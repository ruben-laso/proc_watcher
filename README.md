# `proc_watcher`
This is the repo of `proc_watcher`, a header-only C++ library for monitoring processes on Linux.

## Description
`proc_watcher` is a header-only C++ library for monitoring processes on Linux.
It is based on the `procfs` filesystem, which is a virtual filesystem that provides an interface to kernel data structures.
`procfs` is typically mounted at `/proc` and is used to obtain information about the system and running processes.
`proc_watcher` parses the `/proc` filesystem to obtain information about the processes running on the system, so the user can access this information programmatically.

## Installation
`proc_watcher` is a header-only library, so installing should be fairly easy.
You can simply copy the header files to your project directory and include them in your source code.

Or, you can install it system-wide by running the following commands:
```bash
mkdir build
cd build
cmake ..
sudo make install
```

## Usage
Wiki is WIP. For now, you can check the [demo](demo) directory for examples.

## Contributing
Anyone is welcome to contribute to this project, just behave yourself :smiley:.

Please use the `clang-format` and `clang-tidy` tools before committing.

## License
This project is licensed under the GNU GPLv3 license. See [LICENSE](LICENSE) for more details.

## Contact
If you have any questions, feel send me an [email](mailto:laso@par.tuwien.ac.at).