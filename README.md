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
- [ ] Phoneme and parameter editors
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

DiffScope is licensed under the Apache License, Version 2.0.

This license does not apply to third-party dependencies.

Some built-in plugins are licensed under terms different from those of DiffScope itself. Please refer to the `plugins` directories for the license information applicable to each individual plugin. If a plugin does not explicitly include a separate license, it is licensed under the same terms as DiffScope.
