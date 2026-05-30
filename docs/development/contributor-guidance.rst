Contributor Guidance
====================

Repository guidance is captured in ``AGENTS.md``.

Important project rules:

- Preserve existing receiver and decoder behavior.
- Prefer incremental, reviewable changes.
- Prioritize build, packaging, documentation, wrapper, dashboard, service, and
  operator tooling work before core refactors.
- Do not rewrite DSP, demodulation, Viterbi, Reed-Solomon, packet processing,
  LRIT, EMWIN, or DCS internals unless explicitly requested.

When core-path changes are unavoidable, keep them minimal and verify behavior
with focused tests or representative builds.
