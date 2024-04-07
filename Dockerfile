# syntax=docker/dockerfile:1
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

SHELL [ "/bin/bash", "-c" ]
ENV SHELL=/bin/bash

#Update repositories
RUN apt-get update

#Install build dependencies
RUN apt-get install -y git cmake ninja-build gperf \
  ccache dfu-util device-tree-compiler wget \
  python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
  make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1

#Install zephyr-sdk. Don't include all tools
RUN cd ~ && \
    wget --progress=bar:force:noscroll https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.5-1/zephyr-sdk-0.16.5-1_linux-x86_64_minimal.tar.xz && \
    tar xvf zephyr-sdk-0.16.5-1_linux-x86_64_minimal.tar.xz && \
    rm zephyr-sdk-0.16.5-1_linux-x86_64_minimal.tar.xz && \
    cd zephyr-sdk-0.16.5-1 && \
    ./setup.sh -t arm-zephyr-eabi -t xtensa-espressif_esp32_zephyr-elf -t riscv64-zephyr-elf -h && \
    ln -s ~/zephyr-sdk-0.16.5-1/ ~/zephyr-sdk

#Install west
RUN pip3 install west

#Install zephyr
RUN west init ~/zephyr && \
    cd ~/zephyr && \
    west update && \
    west -z ~/zephyr zephyr-export && \
    pip3 install -r ~/zephyr/zephyr/scripts/requirements.txt && \
	  west blobs fetch hal_espressif

# Always use bash as the shell
RUN echo "dash dash/sh boolean false" | debconf-set-selections
RUN DEBIAN_FRONTEND=noninteractive dpkg-reconfigure dash
RUN ln -sf /bin/bash /bin/sh

# Set zephyr's Environment Variables
RUN echo source /root/zephyr/zephyr/zephyr-env.sh >> ~/.bashrc