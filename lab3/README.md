
## Synopsis
This program implements [Archimedes' mirror legend](http://www.unmuseum.org/burning_mirror.htm) in terms of computer networks.
The process is implemented as an interacting system of four types of agents:
* **WorkerNode** agent represents a soldier with a mirror. It is able to rotate the mirror
 and transmit reflected rays to **FocusNode**, retrieving a feedback: did the intensity
 accumulated on focus raise or not. After focusing worker invokes the next worker to repeat
 process. The last worker in a row invokes the previous invoker, thereby the focusing
 process is circular.
* **SunNode** agent is basically the Sun. Its only function is to change its coordinates
with the given speed and report it to workers.
* **ControlNode** is a commander for workers. It determines workers network topology,
verifies that it is a line and then invokes the last worker to start focusing process.
* **FocusNode** represents a ship to be burnt. It receives information about reflected rays
from workers, checks if ray intersects with the focus or not and gives a feedback in the
shape of intensity value.

<img src="http://www.unmuseum.org/burning_mirror_demo.jpg" alt=""/>

## Launching
1. Download Boost 1.71.0 for your OS from [here](https://www.boost.org/users/history/version_1_71_0.html)
2. Unpack its headers to `libs/boost_1_71_0/include/`
3. Build project with CMake
4. Copy all built executables to `presets/` folder
5. Run `presets/run.sh`

## Notes
* Tested only with GCC 5.4 on Ubunu 16.04
