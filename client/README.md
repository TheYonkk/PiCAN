# Telemetry Client

<!-- @import "[TOC]" {cmd="toc" depthFrom=1 depthTo=6 orderedList=false} -->

<!-- code_chunk_output -->

- [logger](#logger)
- [Automatic Build](#automatic-build)
- [Manual Build](#manual-build)
  - [Dependencies](#dependencies)
    - [Manually installing dbcppp](#manually-installing-dbcppp)
  - [CMake](#cmake)
  - [Build binaries](#build-binaries)
- [Configuration](#configuration)
- [Run](#run)
  - [Command line options](#command-line-options)
    - [`-f`](#-f)

<!-- /code_chunk_output -->

# Automatic Build

To use the telemetry client, you must build it first. To make things nice and easy, I've created a `./build` script that will download, build, and install the dependencies for the client, as well as build the client application once dependencies are met. To run this, simply run `sudo ./build` from within the `client` directory.


# Manual Build

Manual build is accomplished using [CMake](https://cmake.org). If CMake is not installed, you must install it, either by downloading the source and building, or simply running `sudo apt-get install cmake` (easiest). Downloading via the package manager may not install the latest version, so I reccomend building by source. Instructions for doing so can be found [here](https://cmake.org/install/).

## Dependencies

* [yaml-cpp](https://github.com/jbeder/yaml-cpp) is used for YAML file parsing, but is included as a submodule in this repository to make for easy building. If the yaml-cpp files do not appear in the `client/src/external/yaml-cpp` directory after cloning, cd into that directory and run `git submodule init`, followed by `git submodule update`.

* [json](https://github.com/nlohmann/json) is used to package CAN data before sending the JSON string over to the server. If the json module files do not appear in the `client/src/external/json` directory after cloning, cd into that directory and run `git submodule init`, followed by `git submodule update`.


## CMake
CMake creates the Makefiles necessary for UNIX building. To run CMake:
1. `cd client/src`
2. `mkdir build`
3. `cd build`
4. `cmake ..`

The Makefiles should now be present if all prerequisits were satisfied.

## Build binaries

After running CMake, you're all set. Simply run the command `make` from within the `build` directory.

# Configuration

After building, all of the logger configuration is located in the `src/build/client.conf` file.

Be sure that you have full paths to the DBC files, not relative paths.

If you examine the logger's output and the HTTP write response is null but there's no data in the database, it's likely a permissions issue. Try changing the Influx token.

# Run

To manually run the logger, simply run `./logger` after building

## Command line options

### `-f`
The default configuration file is `logger.conf`, located in the build directory after building. If you'd like to use a different configuration file, use the `-f` flag. For example: `./logger -f /path/to/my.config`

# Install as a service

