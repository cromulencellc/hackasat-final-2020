# finals_challenge_2_adcs_tables

It looks like the adversary has corrupted the adcs table. The wheel is spinning all the time, and the gyro and calibration constants have been changed.

The adversary has also disabled the commands associated with the adcs pointing modes and calibration constant updates inside their implant, so we need another way to control the satellite.

To solve the challenge teams must upload new adcs tables to restore the nominal configuration and also use these tables to point the solar array at the sun.

The repo contains example tables that can be use for the following:
* Restoring the nominal configuration (es_adcs_tbl_teamX_.json - contains correct cal values for each team)
* Pointing solar array at the sun (es_adcs_tbl_sasp.json)
* Pointing camera(-X) at earth (es_adcs_tbl_cam.json)
* Ruby scripts that can be executed from Cosmos that transfer and load these tables

## License ##
Solvers in this repository are provided as-is under the MIT license.
See [LICENSE.md](LICENSE.md) for more details.
