# finals_cfs_build_example

Contains the cfs directory from an opensatkit repository with the PSP and OSAL required to build RTEMS 5 LEON3 apps.

Does not contain source for all the game apps.
Can be used in conjunction with the provided rtems5 tools teams docker files to develop cFS applications for RTEMS 5 on the LEON3

Build the rtems5:tools docker container using the Dockerfile provided in rtems5_tools_teams repository.

docker build --tag rtems5:teams .

'make docker-build' in the cfs directory to build cfs.  Read the cfs documentation for adding an app to the cmake build environment

