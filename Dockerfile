FROM ubuntu:latest AS toolchain

ARG BINUTILS_VERSION=2.47
ARG GCC_VERSION=16.2.0

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
        --disable-nls --enable-languages=c --without-headers \
    && make -j$(nproc) all-gcc all-target-libgcc \
    && make install-gcc install-target-libgcc \
    && cd .. && rm -rf build-gcc gcc-${GCC_VERSION}*

FROM toolchain AS slim

# Drop what a build never uses: docs, and symbols of the host binaries
RUN rm -rf ${PREFIX}/share/man ${PREFIX}/share/info ${PREFIX}/share/locale \
    && find ${PREFIX}/bin ${PREFIX}/libexec ${PREFIX}/${TARGET}/bin -type f -exec strip --strip-unneeded {} + 2>/dev/null; \
    ${PREFIX}/bin/${TARGET}-strip --strip-debug $(find ${PREFIX}/lib -name '*.a' -o -name '*.o')

FROM ubuntu:latest

ENV IN_DOCKER=1
ENV PATH=/opt/cross/bin:$PATH

# `-T linker.ld` drops ld's default search dirs, so `libgcc.a` has to be given with `-L`
ENV LDFLAGS="-z noexecstack -nostdlib -static -L/opt/cross/lib/gcc/i686-elf/16.2.0"

RUN apt update && apt install -y --no-install-recommends make grub2 xorriso mtools clang-format python3 libgmp10 libmpfr6 libmpc3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=slim /opt/cross /opt/cross

RUN useradd -m -s /bin/bash dockeruser
USER dockeruser

WORKDIR /kfs
ENTRYPOINT [ "make" ]
