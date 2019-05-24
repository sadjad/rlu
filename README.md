# A C++17 implementation of Read-Log-Update paper (SOSP'15)

## Build directions

The following programs and libraries are required to build this program:

* `gcc`, `g++` (tested with 8.3.0)
* `automake`
* `pkg-config`
* `libtool`
* `liburcu-dev`

To build, run the following commands:

```
./autogen.sh
./configure
make -j$(nproc)
```
