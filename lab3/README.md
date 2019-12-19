
## Synopsis
Not that simple program intended to demonstrate OSPF
(Open Shortest Path First) protocol in action. Computer
network is modeled by a set of processes called Common
Routers and one Designated Router, which synchronizes
LSA (Link-State Advertisement) between common routers.
## Launching
1. Download Boost 1.71.0 for your OS from [here](https://www.boost.org/users/history/version_1_71_0.html)
2. Unpack its headers to `libs/boost_1_71_0/include/`
3. Build project with CMake
4. Run one `router` executable as designated router and a set of `router` executables as common routers. Designated router should be launched first.

Out-of-box presets can be found at `presets/` folder. It contains three simple topology cases: cross, circle and full-connected (actually not full, but I don't know how to call it). For launching you should copy 
`router` and `clean_channels` executables to the folder with scripts (you can
just modify scripts or use only config files the way you like).

Designated router executable produces file called `topology.dot` at the end of execution. This is the final network topology in [DOT](https://en.wikipedia.org/wiki/DOT_(graph_description_language)) format, which can be visualized with GrpahViz for instance (see a special tool for this in `tools/` folder).

## Notes
* Tested only with GCC 5.4 on Ubunu 16.04
