### docker build --ssh default -t atlantis:latest . ###

# Build Atlantis in an isolated stage.
FROM minizinc/mznc2026:latest AS builder

# Install compiler toolchain
RUN apt-get update -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    locales \
    locales-all \
    gcc-14 \
    g++-14 \
    python3

ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US.UTF-8

# Clone the Atlantis git repository.
RUN ssh-keyscan github.com > /etc/ssh/ssh_known_hosts
RUN --mount=type=ssh \
    git clone https://www.github.com/astra-uu-se/atlantis /src

# Change directory to /src.
WORKDIR /src

# clone Gecode
RUN --mount=type=ssh \
    mkdir -p .cpm-cache && \
    mkdir -p .cpm-cache/gecode-6.3.0 && \
    git clone --depth 1 \
        --branch release/6.3.0 \
        https://github.com/Gecode/gecode.git .cpm-cache/gecode-6.3.0

# build Gecode
RUN --mount=type=ssh \
    mkdir -p .cpm-cache/gecode-6.3.0/build && \
    cmake -DCMAKE_C_COMPILER=gcc-14 \
          -DCMAKE_CXX_COMPILER=g++-14 \
          -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
          -DCMAKE_BUILD_TYPE=Release \
          -B .cpm-cache/gecode-6.3.0/build \
          -S .cpm-cache/gecode-6.3.0 && \
    cmake --build .cpm-cache/gecode-6.3.0/build \
          --config Release \
          -j 8 && \
    mkdir -p .cpm-cache/gecode && \
    cmake --install .cpm-cache/gecode-6.3.0/build \
          --config Release \
          --prefix .cpm-cache/gecode && \
    rm -rf .cpm-cache/gecode-6.3.0

# Config Atlantis
RUN --mount=type=ssh \
    echo "test" \
    mkdir -p /install && \
    mkdir -p build && \
    cd build && \
    cmake -DCMAKE_C_COMPILER=gcc-14 \
          -DCMAKE_CXX_COMPILER=g++-14 \
          -DCMAKE_BUILD_TYPE=Release \
          -DMORE_STATS:BOOL=OFF \
          -DBUILD_TESTS:BOOL=OFF \
          -DBUILD_BENCHMARKS:BOOL=OFF \
          -DCPM_SOURCE_CACHE=/src/.cpm-cache \
          -DCMAKE_INSTALL_PREFIX=/install  \
          ..

RUN --mount=type=ssh \
    cd build && \
    make -j 8 && \
    cmake --build build --config Release --target install

# Create our final image using this base.
FROM minizinc/mznc2026:latest

# Copy the Atlantis installation to /atlantis in the final image.
COPY --from=builder /install /atlantis
COPY atlantis.mpc /minizinc/base.mpc

# Add Atlanstis to the MiniZinc search path and set it as the default solver
RUN echo '{"mzn_solver_path": ["/atlantis/share/minizinc/solvers"],' > $HOME/.minizinc/Preferences.json && \
    echo '"tagDefaults": [["", "se.uu.it.atlantis"]]}'              >> $HOME/.minizinc/Preferences.json
