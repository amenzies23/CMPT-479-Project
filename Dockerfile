# Stage 1: Build environment
FROM ubuntu:noble AS build

# Update and install essential packages and repositories
RUN apt-get update -y \
    && DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
        software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/ppa -y \
    && apt-get update -y

# Install essential Ubuntu packages
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
        apt-utils \
        ca-certificates \
        lsb-release \
        gnupg2 \
        wget \
        git \
        make \
        ninja-build \
        cmake \
        valgrind \
        heaptrack \
        cppcheck \
        libncurses5-dev \
        libsqlite3-dev \
        zlib1g-dev \
        xz-utils \
        bzip2 \
        libgtest-dev \
        pkg-config \
        nlohmann-json3-dev \
        libfmt-dev \
        python3 \
        python3-pip \
        gcc-14 \
        g++-14 \
        gdb \
        gcovr \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 1000 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 1000

# Install and build Google Test properly
RUN cd /usr/src/googletest \
    && cmake -B build -S . \
    && cmake --build build --parallel \
    && cmake --install build \
    && ldconfig

# Install Boost (version 1.83.0)
RUN wget http://downloads.sourceforge.net/project/boost/boost/1.83.0/boost_1_83_0.tar.gz \
    && tar xfz boost_1_83_0.tar.gz \
    && rm boost_1_83_0.tar.gz \
    && cd boost_1_83_0 \
    && ./bootstrap.sh --with-libraries=system,filesystem,container,locale,log,program_options,serialization,stacktrace \
    && ./b2 cxxstd=23 install \
    && cd ../ && rm -rf boost_1_83_0

# Install LLVM and Clang (version 18)
RUN wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh 18 \
    && rm llvm.sh \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 1000 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 1000 \
    && update-alternatives --install /usr/bin/llvm-profdata llvm-profdata /usr/bin/llvm-profdata-18 1000 \
    && update-alternatives --install /usr/bin/llvm-cov llvm-cov /usr/bin/llvm-cov-18 1000

# Install Emscripten
RUN cd /opt \
    && git clone https://github.com/emscripten-core/emsdk.git \
    && cd emsdk \
    && ./emsdk install latest \
    && ./emsdk activate latest \
    && ./emsdk construct_env \
    && echo "/opt/emsdk/emsdk activate latest" >> /etc/profile \
    && echo ". /opt/emsdk/emsdk_env.sh" >> /etc/profile

# Install spdlog with system fmt (avoid bundled fmt conflict)
RUN git clone https://github.com/gabime/spdlog.git \
    && cd spdlog \
    && mkdir build && cd build \
    && cmake .. -DSPDLOG_FMT_EXTERNAL=ON && make -j$(nproc) && make install \
    && cd ../.. && rm -rf spdlog

# Install additional late-stage Ubuntu packages
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
        heaptrack

# Final cleanup to reduce image size
RUN rm -rf /var/lib/apt/lists/*
