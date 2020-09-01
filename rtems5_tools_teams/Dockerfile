FROM ubuntu:18.04 as builder

WORKDIR /src
ENV PATH="/opt/rtems/5/bin:${PATH}"
RUN apt update ; \
    apt upgrade ; \
    apt install -y gcc-multilib autoconf automake bison diffutils flex gawk git gperf help2man make patch texinfo wget \
      'g++' python-dev xz-utils libtool-bin libncurses-dev zip unzip bzip2 zlib1g-dev 
RUN git clone git://git.rtems.org/rtems-source-builder.git rsb ;\
    cd rsb ;\
    git checkout -b has d48abf49ab0a 

RUN git clone git://git.rtems.org/rtems.git;\
    cd rtems ;\
    git checkout -b has 362cf319d44e

RUN cd /src/rsb/rtems ;\
    /src/rsb/source-builder/sb-set-builder --prefix=/opt/rtems/5 5/rtems-sparc ;\
    /src/rsb/source-builder/sb-set-builder --prefix=/opt/rtems/5 5/rtems-i386

COPY *.patch /src/rtems
RUN cd /src/rtems; \ 
    git apply *.patch ;\
    ./bootstrap -c ;\ 
    /src/rsb/source-builder/sb-bootstrap

ENV RTEMS_MAKEFILE_PATH=/opt/rtems/5/sparc-rtems5/leon3
COPY leon3_custom.cfg /src/rtems/bsps/sparc/leon3/config/leon3.cfg
RUN mkdir -p /src/kernel/leon3 ;\
    cd /src/kernel/leon3 ;\
    /src/rtems/configure --prefix=/opt/rtems/5 --target=sparc-rtems5 --enable-rtemsbsp=leon3 \
        --enable-posix --enable-networking --enable-drvmgr ;\
    make -j4; \
    make install
RUN mkdir -p /src/kernel/i386 ;\
    cd /src/kernel/i386 ;\
    /src/rtems/configure --prefix=/opt/rtems/5 --target=i386-rtems5 --enable-rtemsbsp=pc686 \
        --enable-posix --enable-networking CLOCK_DRIVER_USE_TSC=1;\
    make -j4; \
    make install


FROM ubuntu:18.04 AS runtime

RUN apt update ; \
    apt install -y gcc-multilib autoconf automake bison diffutils flex gawk git gperf help2man make patch texinfo wget \
          'g++' python-dev xz-utils libtool-bin libncurses-dev zip unzip bzip2 zlib1g-dev libncurses5 


WORKDIR /src
COPY --from=builder /opt /opt

ENV PATH="/opt/rtems/5/bin:${PATH}"
ENV RTEMS_MAKEFILE_PATH=/opt/rtems/5/sparc-rtems5/leon3

cmd ["/usr/bin/make"]

