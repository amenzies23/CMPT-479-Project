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
        gcovr \
        cppcheck \
        gdb \
        valgrind \
        python3 \
        curl \
        unzip \
        zip \
        tar \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 1000 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 1000 \
    && rm -rf /var/lib/apt/lists/*

# Install and build Google Test
RUN cd /usr/src/googletest \
    && cmake -B build -S . \
    && cmake --build build --parallel \
    && cmake --install build \
    && ldconfig

# Install vcpkg
RUN git clone https://github.com/Microsoft/vcpkg.git /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh \
    && /opt/vcpkg/vcpkg integrate install

# Set environment variables for vcpkg
ENV VCPKG_ROOT=/opt/vcpkg
ENV CMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake

# Set working directory
WORKDIR /workspace

# Copy source code
COPY . .

# Install project dependencies via vcpkg (manifest mode)
RUN /opt/vcpkg/vcpkg install --triplet=x64-linux

# Clone tree-sitter-cpp grammar
RUN git clone --depth 1 --branch v0.20.0 https://github.com/tree-sitter/tree-sitter-cpp.git /opt/tree-sitter-cpp-grammar
