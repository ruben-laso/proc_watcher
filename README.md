# `prox`
This is the repo of `prox`, a header-only C++ library for monitoring processes on Linux.

## Description
`prox` is a header-only C++ library for monitoring processes on Linux.
It is based on the `procfs` filesystem, which is a virtual filesystem that provides an interface to kernel data structures.
`procfs` is typically mounted at `/proc` and is used to obtain information about the system and running processes.
`prox` parses the `/proc` filesystem to obtain information about the processes running on the system, so the user can access this information programmatically.

## Installation
`prox` is a header-only library, so installing should be fairly easy (make sure you have the dependencies installed).
You can simply copy the header files to your project directory and include them in your source code.

Or, you can install it system-wide by running the following commands:
```bash
mkdir build
cd build
cmake ..
sudo cmake --build . --target install
```

### Dependencies
`prox` depends on the following libraries:
- [range-v3](https://github.com/ericniebler/range-v3)
- [fmt](https://github.com/fmtlib/fmt)
- libnuma

Additionally, `prox` example depends on the following libraries:
- [spdlog](https://github.com/gabime/spdlog)
- [CLI11](https://github.com/CLIUtils/CLI11)

For testing, `prox` depends on the following libraries:
- [Google Test](https://github.com/google/googletest)

## Usage
Wiki is WIP. For now, you can check the [example](example) directory for examples.

## Contributing
Anyone is welcome to contribute to this project, just behave yourself :smiley:.

Please use the `clang-format` and `clang-tidy` tools before committing.

## License
This project is licensed under the GNU GPLv3 license. See [LICENSE](LICENSE) for more details.

## Contact
If you have any questions, feel free to send me an [email](mailto:laso@par.tuwien.ac.at).