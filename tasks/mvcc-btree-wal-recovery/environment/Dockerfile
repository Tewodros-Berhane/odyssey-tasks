FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-13 \
    g++-13 \
    cmake \
    ninja-build \
    valgrind \
    gdb \
    clang-16 \
    clang-tools-16 \
    llvm-16 \
    python3 \
    python3-pip \
    git \
    pkg-config \
    libpthread-stubs0-dev \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN mkdir -p /app/build && cd /app/build && cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release && ninja
