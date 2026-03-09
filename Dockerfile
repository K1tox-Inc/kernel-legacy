FROM ubuntu:latest

ENV IN_DOCKER=1

RUN apt update && apt install -y binutils-i686-linux-gnu gcc-i686-linux-gnu g++-i686-linux-gnu make grub2 xorriso clang-format python

RUN useradd -m -s /bin/bash dockeruser
USER dockeruser

WORKDIR /kfs
ENTRYPOINT [ "make" ]
