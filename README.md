# Hack-a-Sat 2020 Final #

This repository contains the open source release for the Hack-a-Sat 2020
final event.

Released artifacts include:

* Source code for all challenges
* Source code for all challenge solutions
* Source code for ground station
* Source code for custom hardware board
* Source code for custom cFS and COSMOS deployments

Released artifacts *do not* include:

* Infrastructure used to host and run the game
* Source code for the score board
* Hardware schematics for the modified flatsats


## Repository Structure ##

* `cdh_rev1` - ztex FPGA project
* `finals_cfs_build_example` - Example of how to build cFS
* `flatsat_openmsp430` - OpenMSP430 core used for the ground station radio
* `groundstation_rf` - Ground station radio protocol implementation
* `gs_udp_radio_bridge` - Ground station radio bridge
* `opensatkit` - Flight software and ground system
* `rtems5_tools_teams` - Required files for building cFS/challenge 3
* `finals-challenge-0` - Implementation of challenge 0
* `finals-challenge-3` - Implementation of challenge 3
* `finals-challenge-5` - Implementation of challenge 5
* `solvers` - Solutions for challenges 2-4


## License ##

Files in this repository are provided as-is under the MIT license unless
otherwise stated below or by a license header. See [LICENSE.md](LICENSE.md)
for more details.

Kubos is provided under the Apache v2.0 license.

42 and cFS are provided under the NOSA v1.3 license.

COSMOS is provided under the GPLv3 license.

OSK is provided under the LGPL license.

GRLIB is provided under the GPL license.

openMSP430 is provided under the BSD license.


## Contact ##

Questions, comments, or concerns can be sent to `hackasat[at]cromulence.com`.
