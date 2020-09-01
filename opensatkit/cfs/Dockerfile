FROM rtems5:tools AS builder

WORKDIR /src

RUN apt update ; \
    apt install -y cmake curl default-jre libssl-dev libyaml-dev libffi-dev libreadline6-dev zlib1g-dev python-dev xxd
    #apt install -y cmake curl default-jre xfce4-terminal gcc libssl-dev libyaml-dev libffi-dev \
    #libreadline6-dev zlib1g-dev  \
    #freeglut3 freeglut3-dev qt4-default qt4-dev-tools libqt4-dev libsmokeqt4-dev libcanberra-gtk* dos2unix python-dev
    #libgdbm3 libgdbm-dev libncurses5-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev
ADD cfs.tar /src/


cmd ["/bin/bash", "/src/build.sh"]
