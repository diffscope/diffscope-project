<!--
SPDX-FileCopyrightText: Team OpenVPI
SPDX-License-Identifier: GFDL-1.3-or-later
-->

# DiffScope

A free, professional singing-voice-synthesis editor powered by [DiffSinger](https://github.com/openvpi/DiffSinger)

> [!NOTE]
> This project is currently under development. Anything related to the project is subject to change.

## Looking for Contributors

This project is currently under active development, and we are looking for contributors to help develop it.

Preferred Skills:
- C++ (front-end core logic)
- Qt QML (front-end GUI)
- Golang (backend)

Additionally, familiarity with singing voice synthesis (SVS), digital audio processing, or related fields is highly appreciated.

If you are interested in contributing or collaborating, please feel free to reach out via Issues.

## Progress

- [x] Application GUI shell
- [x] Arrangement and piano roll editors
- [x] Basic audio processing functionalities
- [x] Phoneme and parameter editors
- [x] Audio playback
- [x] Integration of synthesis engine

## Build

1. Install essential tools for C/C++ development and Qt.
2. Clone this repository recursively.

  ```bash
  git clone --recursive https://github.com/diffscope/diffscope-project.git
  ```
   
3. Install dependencies via Vcpkg. The script checks out vcpkg and the overlays at the commits that `scripts/vcpkg-sources.json` specifies into `deps/`, and installs the dependencies there. It runs on Windows, macOS and Linux.

  ```bash
  cmake -P scripts/vcpkg-workspace.cmake install --qt-dir /path/to/Qt/lib/cmake/Qt6
  ```

  `cmake -P scripts/vcpkg-workspace.cmake help` lists the other commands. `--dry-run` prints the commands without running them.
   
4. Build using CMake with the vcpkg toolchain in `deps/`.

  ```bash
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=deps/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH=/path/to/Qt
  ```

You may refer to CI scripts for more details.

## License

This repository uses multiple licenses. See [LICENSE](LICENSE) for the component license index and the [LICENSES](LICENSES) directory for the complete license texts.
