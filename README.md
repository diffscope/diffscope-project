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
- [ ] Integration of synthesis engine

## Build

1. Install essential tools for C/C++ development and Qt.
2. Clone this repository recursively.

  ```bash
  git clone --recursive https://github.com/diffscope/diffscope-project.git
  ```
   
3. Install dependencies via Vcpkg.

  ```bash
  vcpkg install --x-manifest-root=/path/to/diffscope-project/scripts/vcpkg-manifest --x-install-root=/path/to/vcpkg/installed
  ```
   
4. Build using CMake.

You may refer to CI scripts for more details.

## License

This repository uses multiple licenses.

The DiffScope Project (the combined work comprising the DiffScope Application, its built-in plugins, and its third-party libraries) is licensed under the [GNU General Public License, Version 3](LICENSES/GPL-3.0-only.txt).

The DiffScope Application itself and the Core Plugin are licensed under the [Apache License, Version 2.0](LICENSES/Apache-2.0.txt).

Each built-in plugin is subject to its respective license terms.

Third-party software is subject to its respective license terms.

See [LICENSE](LICENSE) for the component license index and the [LICENSES](LICENSES) directory for the complete license texts.
