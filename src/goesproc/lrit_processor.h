#pragma once

#include <memory>
#include <vector>

#include "handler.h"
#include "log.h"

// Takes a list of paths to LRIT files and/or directories.
//
// This class sorts the files in chronological order and then feeds
// them to the handlers.
//
class LRITProcessor {
public:
  LRITProcessor(
    std::vector<std::unique_ptr<Handler> > handlers,
    const std::shared_ptr<Logger>& logger);

  void run(int argc, char** argv);

protected:
  std::vector<std::unique_ptr<Handler> > handlers_;
  std::shared_ptr<Logger> logger_;
};
