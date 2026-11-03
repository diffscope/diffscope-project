<!--
SPDX-FileCopyrightText: Team OpenVPI
SPDX-License-Identifier: GFDL-1.3-or-later
-->



# ![DiffScope](docs/banner.png)

[![Dev Build](https://github.com/diffscope/diffscope-project/actions/workflows/dev-build.yml/badge.svg)](https://github.com/diffscope/diffscope-project/actions/workflows/dev-build.yml)
![GitHub Release](https://img.shields.io/github/v/release/diffscope/diffscope-project?include_prereleases)


A free, professional singing-voice-synthesis editor powered by [DiffSinger](https://github.com/openvpi/DiffSinger)

Visit <https://diffscope.org/> for more details.

> [!NOTE]
> This project is currently under development. Anything related to the project is subject to change.

## Download

Nightly builds are available to download, but are unstable and do not yet include all features.

Visit <https://diffscope.org/downloads/#diffscope> to download.

## Features

- **Singing synthesis:** DiffSinger synthesis through a local [DiffSinger Synthesis Program (DSSP)](https://github.com/diffscope/diffsinger-synthesis-program) service or a remote DiffScope Synthesis Platform (DSSP) API, with voice package installation and per-stage resynthesis.
- **Note editing:** A piano roll for notes, lyrics, phonemes, synthesis parameters, and pitch curves, including freehand and line pitch drawing, pitch fixing, transform, and erasing.
- **Arrangement:** Multi-track timeline with singing clips, audio clips, labels, and variable tempo/time signature/key signature events, plus loop playback, metronome, and MIDI recording.
- **Mixed singers:** Multiple singers per singing clip with weighted ratios, time-varying voice blending, and reusable presets.
- **Mixing and effects:** Per-track volume, pan, mute, and solo controls, effect chains on tracks and the master bus, offline time stretching and pitch shifting, and mixed or stem export in multiple audio formats.
- **Audio extraction:** Derives notes, pitch curves, and tempo from audio clips and separates clips into stems through DSSP services.
- **Interchange:** Imports and exports MIDI and multiple formats via [LibreSVIP](https://github.com/SoulMelody/LibreSVIP), and exports lyrics as plain text or timed text.

## Progress

- [x] Basic SVS editor experience
- [ ] Scripting and MCP
- [ ] VST bridge
- [ ] Plugin SDK

## Build

1. Install essential tools for C/C++ development and Qt.
2. Clone this repository recursively.

  ```bash
  git clone --recursive https://github.com/diffscope/diffscope-project.git
  ```
   
3. Install dependencies via Vcpkg with [vcpkg-workspace.cmake](https://github.com/stdware/dev-scripts/blob/main/docs/about-scripts/vcpkg-workspace.md), downloaded into `scripts/`. The script checks out vcpkg and the overlays that `scripts/vcpkg-sources.json` specifies into `deps/`, and installs the dependencies there. It runs on Windows, macOS and Linux.

  ```bash
  curl -fsSL -o scripts/vcpkg-workspace.cmake https://raw.githubusercontent.com/stdware/dev-scripts/main/scripts/vcpkg-workspace.cmake
  cmake -P scripts/vcpkg-workspace.cmake install --qt-dir /path/to/Qt/lib/cmake/Qt6
  ```
  
  Or with PowerShell:  

  ```powershell
  Invoke-WebRequest -OutFile scripts/vcpkg-workspace.cmake https://raw.githubusercontent.com/stdware/dev-scripts/main/scripts/vcpkg-workspace.cmake
  cmake -P scripts/vcpkg-workspace.cmake install --qt-dir /path/to/Qt/lib/cmake/Qt6
  ```

  `cmake -P scripts/vcpkg-workspace.cmake help` lists the other commands. `--dry-run` prints the commands without running them. `scripts/vcpkg-sources.local.json` takes precedence over `scripts/vcpkg-sources.json`, for example to use a local overlay.
   
4. Build using CMake with the vcpkg toolchain in `deps/`.

  ```bash
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=deps/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=/path/to/Qt
  ```

You may refer to CI scripts for more details.

## License

This repository uses multiple licenses. See [LICENSE](LICENSE) for the component license index and the [LICENSES](LICENSES) directory for the complete license texts.
