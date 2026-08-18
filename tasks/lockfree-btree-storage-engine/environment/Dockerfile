FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y gcc-13 g++-13 clang-18 cmake ninja-build python3 libomp-dev
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100
COPY . /app
WORKDIR /app
