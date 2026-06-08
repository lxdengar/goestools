Airspy R2 Support Note
======================

The current ``goesrecv`` Airspy backend should already work with Airspy R2
devices when ``goesrecv`` is built with ``libairspy`` support. This note records
why the current support applies to the R2 and what future polish would be useful
if users run into device-specific problems.

Current Support
---------------

``src/goesrecv/airspy_source.cc`` uses the generic ``libairspy`` API:

- ``airspy_open`` opens the attached Airspy device.
- ``airspy_get_samplerates`` queries the sample rates supported by the attached
  device.
- ``airspy_set_sample_type`` requests ``AIRSPY_SAMPLE_FLOAT32_IQ``.
- ``airspy_set_samplerate`` sets either the configured sample rate or the lowest
  supported device rate.
- ``airspy_set_freq`` tunes the receiver.
- ``airspy_set_linearity_gain`` applies the configured gain value.
- ``airspy_set_rf_bias`` controls Airspy bias tee power.
- ``airspy_start_rx`` streams complex samples into the existing receiver
  pipeline.

Because sample rates are queried from the device rather than hard-coded, the
same source class can support both Airspy R2 and Airspy Mini. The sample
configuration already notes that the default lowest sample rate is typically
2.5 Msps for Airspy R2 and 3.0 Msps for Airspy Mini.

Recommended R2 Configuration
----------------------------

A conservative Airspy R2 HRIT configuration should start with the existing
``airspy`` source:

.. code-block:: toml

   [demodulator]
   mode = "hrit"
   source = "airspy"

   [airspy]
   frequency = 1694100000
   gain = 18
   bias_tee = false

Leave ``sample_rate`` unset at first so ``goesrecv`` uses a rate reported by
the attached device. For Airspy R2 this should select the lowest supported rate,
commonly 2.5 Msps. If users want to force the R2's higher-rate mode, they can
set:

.. code-block:: toml

   [airspy]
   sample_rate = 10000000

When using 10 Msps, users may need to set ``[demodulator] decimation`` to keep
the downstream clock recovery in a practical samples-per-symbol range.

Known Limitations
-----------------

The current support is likely sufficient for basic R2 operation, but it is not
feature-complete:

- ``Airspy::open(uint32_t index)`` accepts an index argument, but the
  implementation currently calls ``airspy_open`` and does not use that index.
  Systems with more than one Airspy device would benefit from serial-number or
  indexed open support.
- Only Airspy linearity gain is exposed. ``libairspy`` also supports other gain
  styles and individual gain stages. A future config could expose sensitivity
  gain or separate LNA/mixer/VGA controls if GOES reception benefits from them.
- Error handling uses assertions or exits in some paths. User-facing errors
  would be nicer if all Airspy setup failures were converted to
  ``std::runtime_error`` messages, matching the direction planned for new
  source backends.
- Bias tee is exposed as ``bias_tee`` and calls ``airspy_set_rf_bias``. Airspy
  R2 bias tee is intended to power external LNAs or up/down-converters, but it
  should remain disabled by default and documented with voltage/current limits.
- Documentation should explicitly mention ``airspy_info`` or equivalent host
  tools for confirming the R2 is visible to the OS before running
  ``goesrecv``.

Support Plan If R2 Problems Appear
----------------------------------

If live testing shows that Airspy R2 does not behave well with the current
backend, keep changes scoped to the Airspy wrapper and configuration:

1. Add a small Airspy device-info logging path that reports detected serial,
   supported sample rates, and selected sample rate.
2. Add optional ``device_index`` or ``serial`` fields to ``[airspy]`` and use
   the appropriate ``libairspy`` open call.
3. Add validation for Airspy gain and sample-rate values before starting RX.
4. Add optional gain-mode fields only if measurements show that linearity gain
   is not enough for reliable GOES reception.
5. Document R2-specific examples for 2.5 Msps and 10 Msps operation.
6. Run live validation using ``goesrecv -v`` and compare ``omega``, frequency
   correction, Viterbi corrections, Reed-Solomon corrections, packet rate, and
   drops against the current Airspy Mini or RTL-SDR baseline.

No core DSP, demodulator, Viterbi, Reed-Solomon, packet, LRIT, or product
processing changes should be needed for Airspy R2 support unless live receiver
stats show a specific sample-rate or decimation problem.

References
----------

- Airspy R2 product page: https://airspy.com/airspy-r2/
- Airspy quick start: https://airspy.com/quickstart/
- Airspy host tools and ``libairspy``: https://github.com/airspy/airspyone_host
