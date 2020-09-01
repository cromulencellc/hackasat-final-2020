FROM debian:stable

ENV DEBIAN_FRONTEND noninteractive

RUN dpkg --add-architecture i386

RUN apt-get update -y -qq
RUN apt-get install -y -qq --no-install-recommends \
    build-essential cmake libc6:i386 gcc-multilib \
    bc ca-certificates file locales rsync \
    cvs bzr git mercurial subversion wget \
    cpio unzip \
    libncurses5-dev \
    python-nose2 python-pexpect qemu-system-arm qemu-system-x86\
    git curl libssl-dev pkg-config
RUN apt-get -q -y autoremove
RUN apt-get -q -y clean

WORKDIR /root/
COPY build-kubos/rpi0_toolchain.tar.gz /root/
RUN tar -xf ./rpi0_toolchain.tar.gz -C /usr/bin && rm ./rpi0_toolchain.tar.gz

ENV PATH "$PATH:/root/.cargo/bin"
RUN curl https://sh.rustup.rs -sSf | sh -s -- -y && rustup toolchain uninstall stable-x86_64-unknown-linux-gnu
RUN rustup default 1.39.0 && rm -rf /root/.rustup/toolchains/*/share/doc
RUN rustup target install arm-unknown-linux-gnueabihf
RUN rustup target install armv5te-unknown-linux-gnueabi
RUN rustup target add arm-unknown-linux-gnueabi
RUN rustup component add clippy
RUN rustup component add rustfmt
RUN /root/.cargo/bin/cargo install --git https://github.com/kubos/cargo-kubos
COPY build-kubos/cargo_config /root/.cargo/config

# To be able to generate a toolchain with locales, enable one UTF-8 locale
RUN sed -i 's/# \(en_US.UTF-8\)/\1/' /etc/locale.gen
RUN /usr/sbin/locale-gen

ENV FORCE_UNSAFE_CONFIGURE=1
ENV PATH "$PATH:/usr/bin/rpi0_toolchain/usr/bin"

COPY build-kubos/onentry.sh /root

CMD /root/onentry.sh && cd /kubos-linux/ && /bin/bash