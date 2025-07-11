# Stage 1: Build environment
FROM ubuntu:noble

# Add GCC toolchain repository and update
RUN apt-get update -y \
    && DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
        software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/ppa -y \
    && apt-get update -y


# Install essential Ubuntu packages
RUN apt-get update -y \
    && DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends --fix-missing \
        git \
        make \
        cmake \
        gcc-14 \
        g++-14 \
        libgtest-dev \
        pkg-config \
        nlohmann-json3-dev \
        libfmt-dev \
        gcovr \
        cppcheck \
        gdb \
        valgrind \
        python3 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 1000 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 1000 \
    && rm -rf /var/lib/apt/lists/*

# Install and build Google Test
RUN cd /usr/src/googletest \
    && cmake -B build -S . \
    && cmake --build build --parallel \
    && cmake --install build \
    && ldconfig

# Install spdlog with system fmt (avoid bundled fmt conflict)
RUN git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && mkdir build && cd build \
    && cmake .. -DSPDLOG_FMT_EXTERNAL=ON && make -j$(nproc) && make install \
    && cd ../.. && rm -rf spdlog
