FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    valgrind \
    gdb \
    clang \
    llvm \
    python3 \
    python3-pip \
    git \
    pkg-config \
    libopenblas-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN mkdir -p /app/build && cd /app/build && cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release && ninja
