#include "packet_processor.h"

#include <iomanip>

#include "lrit/file.h"

PacketProcessor::PacketProcessor(
    std::vector<std::unique_ptr<Handler> > handlers,
    const std::shared_ptr<Logger>& logger)
  : handlers_(std::move(handlers)),
    logger_(logger),
    assembler_([logger](int vcid, unsigned lost, unsigned previous, unsigned current) {
      nlohmann::json fields;
      fields["vcid"] = vcid;
      fields["lost"] = lost;
      fields["previous"] = previous;
      fields["current"] = current;
      logger->increment("vcdu_lost", lost);
      logger->eventRateLimited(
        LogLevel::WARNING,
        "vcdu_gap",
        "vcdu_gap:" + std::to_string(vcid),
        fields);
    }) {
}

void PacketProcessor::run(std::unique_ptr<PacketReader>& reader, bool verbose) {
  if (verbose) {
    std::cout
      << "Waiting for first packet..."
      // Carriage return to return to beginning of line
      << "\r"
      // Flush buffers
      << std::flush
      // Erase in line (expected to be buffered)
      << "\033[K";
  }

  std::array<uint8_t, 892> buf;
  bool firstPacket = true;
  bool progressDrawn = verbose;
  while (reader->nextPacket(buf)) {
    if (progressDrawn) {
      std::cout << "\r\033[K" << std::flush;
      progressDrawn = false;
    }
    logger_->increment("vcdu");
    if (firstPacket) {
      logger_->event(LogLevel::INFO, "input_active");
      firstPacket = false;
    }
    auto spdus = assembler_.process(buf);
    for (auto& spdu : spdus) {
      logger_->increment("lrit");
      auto file = std::make_shared<lrit::File>(spdu->get());
      for (auto& handler : handlers_) {
        handler->handle(file);
      }
    }
    logger_->tick();

    if (verbose) {
      VCDU vcdu(buf);
      std::cout
        << "Packet:"
        << " SCID="
        << std::setw(1) << vcdu.getSCID()
        << " VCID="
        << std::setw(2) << vcdu.getVCID()
        << " counter="
        << std::setw(8) << std::setfill('0') << vcdu.getCounter()
        << "\r"
        << std::flush
        << "\033[K";
      progressDrawn = true;
    }
  }
  if (progressDrawn) {
    std::cout << "\r\033[K" << std::flush;
  }
  logger_->summary(true);
}
