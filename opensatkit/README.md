## Hack-A-Sat (HAS) Opensatkit Repository

The Hack-A-Sat Flight Software and Ground System is based on an open source project called OpenSatKit.

OpenSatKit provides a complete desktop solution for learning how to use NASA's open source flight software (FSW) platform called the core Flight System (cFS). The cFS is a reusable FSW architecture that provides a portable and extendable platform with a product line deployment model. 

The cFS deployment has been tailored specifically to the HAS project based on the following changes:

* Addition of LEON3 / RTEMS 5 Support (cpu3)
* Development of a custom Command Ingest / Telemetry Output Application that interfaces to a LEON3 apbuart
* Development of a custom Eyassat IF application that interfaces to the sensors and actuators provided by the EyasSat training satellite
* Development of a custom PL IF application that can be used in conjunction with the RPI Zero from the Cromulence C&DH board to take/downlink pictures 

The Cosmos install provided as part of Opensatkit has been tailored specifically to the HAS project based on the following changes:

* Addition of targets to support HAS custom cFS applications
* Screen updates to support HAS custom cFS applications

### Installation cFS
* Build the RTEMS tool suite based on the readme in the rtems HAS tools repository
* cd cFS
* make docker-build
* cpu1 = x86, cpu2 = i386/RTEMS5, cpu3 = LEON3/RTEMS5

### Installation Cosmos
* $ cd cosmos
* $./setup.sh
* $ rm Gemfile.lock
* $ bundle install