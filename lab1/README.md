
## Synopsis
A simple program intended to demonstrate ARQ protocols (Go-Back-N and Selective 
Repeat) in action.
## Launching
1. Download Boost 1.71.0 for your OS from [here](https://www.boost.org/users/history/version_1_71_0.html)
2. Unpack its headers to `libs/boost_1_71_0/include/`
3. Build project with CMake
4. Run two `agent` executables with path to config files (see example configs in `cfg/`). Receiver agent should be launched before sender one.

## Notes
* Tested only with GCC 5.4 on Ubunu 16.04
