ARG UBUNTU_VERSION=26.04
ARG GCC_VERSION=16.2.0

FROM ubuntu:${UBUNTU_VERSION} AS toolchain

ARG BINUTILS_VERSION=2.47
ARG GCC_VERSION

ENV TARGET=i686-elf
ENV PREFIX=/opt/cross

RUN apt update && apt install -y build-essential curl ca-certificates xz-utils texinfo bison flex \
    libgmp-dev libmpfr-dev libmpc-dev

WORKDIR /src

RUN curl -fsSLO https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz \
    && tar xf binutils-${BINUTILS_VERSION}.tar.xz \
    && mkdir build-binutils && cd build-binutils \
    && ../binutils-${BINUTILS_VERSION}/configure --target=${TARGET} --prefix=${PREFIX} \
    --with-sysroot --disable-nls --disable-werror \
    && make -j$(nproc) && make install \
    && cd .. && rm -rf build-binutils binutils-${BINUTILS_VERSION}*

RUN curl -fsSLO https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz \
    && tar xf gcc-${GCC_VERSION}.tar.xz \
    && mkdir build-gcc && cd build-gcc \
    && ../gcc-${GCC_VERSION}/configure --target=${TARGET} --prefix=${PREFIX} \
    --disable-nls --without-headers --enable-languages=c \
    && make -j$(nproc) all-gcc all-target-libgcc \
    && make install-gcc install-target-libgcc \
    && cd .. && rm -rf build-gcc gcc-${GCC_VERSION}*

FROM toolchain AS slim

RUN rm -rf ${PREFIX}/share/man ${PREFIX}/share/info ${PREFIX}/share/locale \
    && find ${PREFIX}/bin ${PREFIX}/libexec ${PREFIX}/${TARGET}/bin -type f -exec strip --strip-unneeded {} + 2>/dev/null; \
    ${PREFIX}/bin/${TARGET}-strip --strip-debug $(find ${PREFIX}/lib -name '*.a' -o -name '*.o')

FROM ubuntu:${UBUNTU_VERSION}

ARG GCC_VERSION

ENV IN_DOCKER=1
ENV PATH=/opt/cross/bin:$PATH

ENV LDFLAGS="-z noexecstack -nostdlib -static -L/opt/cross/lib/gcc/i686-elf/${GCC_VERSION}"

RUN apt update && apt install -y --no-install-recommends make grub2 xorriso mtools clang-format sparse python3 libgmp10 libmpfr6 libmpc3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=slim /opt/cross /opt/cross

RUN useradd -m -s /bin/bash -u 6767 dockeruser
USER 6767

WORKDIR /kfs
ENTRYPOINT [ "make" ]
