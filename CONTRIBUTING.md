# Contributing to Drummr

Thanks for your interest! Drummr is a hobby project, but contributions are welcome.

## Building locally

Drummr is a JUCE 8 C++20 app built with CMake. macOS is the only tested platform.

```sh
cmake -B build -S .
cmake --build build
open build/Drummr_artefacts/Drummr.app
```

JUCE is fetched automatically via CMake `FetchContent`, so no manual setup is needed.

## Reporting bugs and requesting features

Open a GitHub issue. Please include:

- Your macOS version and Drummr version
- Steps to reproduce (for bugs)
- A MIDI file that triggers the issue, if relevant

## Submitting a pull request

1. Fork the repo and create a feature branch.
2. Keep the change focused — one logical change per PR.
3. Match the existing code style (JUCE conventions, 4-space indent, `.cpp`/`.h` pairs under `Source/`).
4. Test the change by running the app with a real MIDI file before opening the PR.
5. Write a clear PR description: what changed, why, and how you tested it.

## Licensing and contributor agreement

Drummr is licensed under **GPLv3** (see `LICENSE`). By submitting a contribution, you agree that:

1. Your contribution is your own original work (or you have the right to submit it).
2. Your contribution is licensed under GPLv3, same as the rest of the project.
3. You grant the project maintainer (angihe93) a perpetual, irrevocable license to also distribute your contribution under other licenses, including proprietary commercial licenses.

Point 3 lets the project dual-license in the future (for example, offering a paid commercial edition) without needing to track down every past contributor. If you're not comfortable with this, please don't submit a PR — open an issue to discuss instead.

## Questions

Open a GitHub discussion or issue. No formal support channel — this is a side project.
