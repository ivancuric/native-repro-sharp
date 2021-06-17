# Compilation instructions

In this section you will find how to compile the module. The actual compilation
is built on top of CMake and its JS-counterpart `cmake-js` which embeds the
compilation of the module into the actual electron-project.

## MacOS

First you need to install some external dependencies, namely:

- `CMake` (get it from here https://cmake.org/download/)
- Some MacOS build tools (just run `xcode-tools --install` with `sudo` if needed)
- `yarn` (see https://classic.yarnpkg.com/en/docs/install/#mac-stable)
- `Node.js` (needs to support `N-API` version 6, see https://nodejs.org/api/n-api.html#n_api_n_api_version_matrix)
