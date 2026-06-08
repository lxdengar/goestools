HackRF One Support Plan
=======================

This note describes what would be needed to add HackRF One as a supported
``goesrecv`` sample source. It is a planning document only; no decoder, DSP, or
packet-processing changes are required for the first implementation.

.. note::

   HackRF One is not a recommended or expected upgrade over Airspy-class
   hardware for GOES reception. These changes are not implemented yet, and the
   plan is to treat HackRF support as an experimental or secondary receiver
   option.

Relevant HackRF Details
------------------------

HackRF One is a USB SDR that can receive from 1 MHz to 6 GHz, supports
quadrature sample rates from 2 Msps to 20 Msps, and produces 8-bit IQ samples.
For GOES HRIT/LRIT reception, this places it in the same broad role as the
existing Airspy and RTL-SDR source classes: it should deliver complex float
samples into the existing ``Source`` queue and let the current demodulator,
clock recovery, decoder, and packet publisher continue unchanged.

HackRF One should be treated as an experimental or secondary receiver for this
application, not as an expected upgrade over Airspy-class hardware. Its main
practical downside is 8-bit sample depth and generally weaker receive
performance than an Airspy, SDRplay, or a well-supported RTL-SDR with an
appropriate filtered LNA chain. That does not make HackRF unusable, but it
raises the importance of frontend filtering, gain tuning, decimation, and live
quality checks.

Important device-specific controls:

- Sample rate: HackRF documentation supports 2 Msps to 20 Msps, but recommends
  avoiding rates below 8 MHz because of ADC and baseband-filter behavior.
- Baseband filter bandwidth: HackRF exposes configurable baseband filter
  bandwidth and helper functions for choosing a valid hardware value.
- Frequency: GOES HRIT/LRIT examples typically tune near 1694.1 MHz, which is
  within HackRF One's range.
- RX RF amplifier: ``amp`` is an on/off RF amplifier, nominally around 11 dB.
- IF gain: ``lna`` is 0 to 40 dB in 8 dB steps.
- Baseband gain: ``vga`` is 0 to 62 dB in 2 dB steps.
- Antenna port power: HackRF One can provide software-controlled antenna port
  power, up to 50 mA at roughly 3.0 to 3.3 V. This is not the same voltage or
  current budget as many external GOES LNAs expect, so it should be documented
  carefully and default to disabled.

Receiver Caveats
----------------

HackRF's sample-rate guidance matters for GOES reception. The useful GOES
LRIT/HRIT signal bandwidth is far below 8 MHz, and the current receiver can run
with lower-rate sources such as RTL-SDR and Airspy. However, HackRF's own
documentation says sampling below 8 MHz is not recommended because the ADC is
not specified below that rate and the minimum baseband filter bandwidth is too
wide to reject adjacent energy cleanly at low sample rates.

The implementation plan should therefore avoid configuring HackRF directly at a
2 Msps-style rate just because the GOES signal is narrower. A better first
approach is:

- sample from HackRF at 8 Msps or higher;
- use the narrowest suitable HackRF baseband filter;
- decimate inside the existing receiver chain with ``[demodulator] decimation``;
- validate adjacent-channel behavior and packet quality with real captures.

The 8-bit sample depth also leaves less dynamic range than higher-resolution
receivers. In practice this means:

- a filtered LNA near the antenna is likely more important than it is with a
  stronger receiver;
- gain settings need to avoid both burying the signal in noise and clipping or
  distorting strong adjacent signals;
- the RF amp should remain off by default and be enabled only after observing
  receiver stats and spectrum behavior;
- success criteria should be based on packet quality, Viterbi corrections,
  Reed-Solomon corrections, and drop rate, not just whether the device streams.

Build Integration
-----------------

The current source backends are built conditionally:

- ``airspy_source`` is enabled when ``pkg-config`` finds ``libairspy``.
- ``rtlsdr_source`` is enabled when ``pkg-config`` finds ``librtlsdr``.
- ``nanomsg_source`` is always built.

HackRF support should follow the same pattern:

- Add a ``pkg_check_modules(HACKRF libhackrf)`` check in
  ``src/goesrecv/CMakeLists.txt``.
- Add a ``hackrf_source`` library only when ``libhackrf`` is available.
- Link ``goesrecv`` to ``hackrf_source`` only when found.
- Define a compile flag such as ``BUILD_HACKRF`` for ``source.cc``.
- Update install/build docs to mention Ubuntu's ``hackrf`` package and any
  development package needed by the distribution.

This keeps normal builds working on systems without HackRF libraries.

Configuration Shape
-------------------

Add ``source = "hackrf"`` as a valid ``[demodulator]`` source value and add a
new ``[hackrf]`` section. A first-pass configuration could look like:

.. code-block:: toml

   [demodulator]
   mode = "hrit"
   source = "hackrf"
   decimation = 4

   [hackrf]
   frequency = 1694100000
   sample_rate = 8000000
   baseband_filter = 1750000
   amp = false
   lna_gain = 16
   vga_gain = 16
   antenna_power = false
   device_index = 0

Field notes:

- ``sample_rate`` should probably default to 8000000 for HackRF because the
  vendor docs recommend 8 MHz or higher. The GOES signal is narrower than this,
  but HackRF should still sample at or above the recommended floor and let the
  existing demodulator decimate before clock recovery.
- ``baseband_filter`` should be optional. If omitted, the source can compute a
  valid filter bandwidth from the sample rate or use a conservative default.
- ``amp`` should default to ``false``. It can overload the frontend in strong
  RF environments.
- ``lna_gain`` and ``vga_gain`` should default to 16 dB, matching HackRF's
  suggested starting point for RX gain tuning.
- ``antenna_power`` should default to ``false`` and docs should warn that it is
  limited to HackRF's antenna-port power specification.
- ``device_index`` or ``serial`` should be considered for systems with more
  than one HackRF. Serial selection is more stable than index selection.

Source Class Work
-----------------

Add ``src/goesrecv/hackrf_source.h`` and ``src/goesrecv/hackrf_source.cc`` with
the same responsibilities as the current Airspy and RTL-SDR source classes:

- Initialize and close ``libhackrf`` cleanly.
- Open a HackRF device by index or serial.
- Set frequency, sample rate, baseband filter bandwidth, gains, RF amplifier,
  and antenna port power.
- Start RX with ``hackrf_start_rx``.
- Stop RX with ``hackrf_stop_rx`` and release the device.
- Convert callback buffers into ``Samples`` and push them into the existing
  queue.
- Publish raw samples through ``SamplePublisher`` if configured.
- Convert HackRF errors into useful ``std::runtime_error`` messages.

Sample Conversion
-----------------

The existing source interface expects ``Samples`` as complex floats. HackRF RX
callbacks provide interleaved signed 8-bit IQ samples. The conversion should be
similar to the existing ``nanomsg`` source:

- Treat each pair as I/Q.
- Convert each signed byte to float in approximately ``[-1.0, 1.0]``.
- Preserve sample ordering.
- Avoid allocating per sample; allocate one queue buffer per transfer block.

The implementation should verify whether ``libhackrf`` callback bytes are
already signed ``int8_t`` or require conversion from offset unsigned bytes for
the library version in use, and document that decision near the conversion.

Demodulator Considerations
--------------------------

HackRF support should avoid changing the decoder pipeline. The primary
receiver-specific issue is sample rate:

- GOES HRIT has a higher symbol rate than legacy LRIT.
- The current demodulator derives samples-per-symbol from source sample rate,
  mode, and decimation.
- An 8 Msps HackRF input with ``decimation = 4`` may be a practical starting
  point, because HackRF documentation recommends using 8 MHz input and
  decimating downstream when a narrower effective bandwidth is desired.
- Lower direct sample rates may appear tempting for GOES, but they should not be
  the default plan for HackRF because they sit below the vendor-recommended
  operating floor.

Initial validation should compare lock behavior, frequency correction, Viterbi
corrections, Reed-Solomon corrections, packet rate, and dropped packets against
known-working Airspy or RTL-SDR configurations.

User Setup Documentation
------------------------

Documentation should include:

- Ubuntu package installation for HackRF tools and headers.
- ``hackrf_info`` or equivalent detection checks.
- USB permissions and WSL USB/IP notes where applicable.
- A warning about maximum input power and frontend protection.
- A warning that HackRF One uses 8-bit samples and may be less forgiving than
  Airspy, SDRplay, or RTL-SDR plus a filtered LNA.
- A warning that GOES reception should start from an 8 Msps-or-higher HackRF
  sample rate with receiver-side decimation, even though the useful signal is
  narrower than 8 MHz.
- A warning that antenna port power is low-voltage, low-current, and disabled
  by default.
- Example ``goesrecv.conf`` blocks for HackRF HRIT reception.
- Tuning notes for ``amp``, ``lna_gain``, ``vga_gain``, sample rate, and
  baseband filter bandwidth.

Testing Plan
------------

Recommended test sequence:

1. Build without ``libhackrf`` installed and confirm existing sources still
   build.
2. Build with ``libhackrf`` installed and confirm ``goesrecv`` links HackRF
   support.
3. Run a config parse test for ``source = "hackrf"`` and invalid HackRF keys.
4. Run device-open smoke tests with no HackRF attached and verify clear errors.
5. Run live RX smoke tests with HackRF attached.
6. Validate that ``goesrecv -v`` reports stable gain/frequency/omega fields.
7. Validate packet publication into ``goesproc``.
8. Compare sustained packet quality against an existing supported receiver.

Open Questions
--------------

- Should the source select devices by ``device_index``, serial number, or both?
- Should ``sample_rate`` default to 8 MHz with ``decimation = 4``, or should the
  config require both fields explicitly for HackRF?
- Should ``baseband_filter`` be specified directly, computed automatically, or
  both?
- Should the config call the RF amplifier ``amp`` to match HackRF tools, or use
  a more descriptive name such as ``rf_amp``?
- Should antenna port power be named ``antenna_power`` or ``bias_tee``? The
  hardware behavior is similar in purpose but different enough that a distinct
  name is clearer.

References
----------

- HackRF One documentation: https://hackrf.readthedocs.io/en/latest/hackrf_one.html
- HackRF software installation: https://hackrf.readthedocs.io/en/latest/installing_hackrf_software.html
- HackRF sampling rate and baseband filter notes: https://hackrf.readthedocs.io/en/latest/sampling_rate.html
- HackRF RX gain controls: https://hackrf.readthedocs.io/en/latest/setting_gain.html
- HackRF tools options: https://hackrf.readthedocs.io/en/latest/hackrf_tools.html
