### docker build --ssh default -t atlantis:latest . ###

# Build Atlantis in an isolated stage.
FROM minizinc/mznc2025:latest AS builder

# Install compiler toolchain
RUN apt-get update -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    locales \
    locales-all

ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US.UTF-8

# Clone the Atlantis git repository.
RUN ssh-keyscan github.com > /etc/ssh/ssh_known_hosts
RUN --mount=type=ssh \
    git clone https://www.github.com/astra-uu-se/atlantis /src

# Change directory to /src.
WORKDIR /src

# Build Atlantis and install it into /install.
RUN --mount=type=ssh \
    mkdir /install && mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/install .. && \
    cmake --build . --config Release && \
    cmake --build . --config Release --target install

# Create our final image using this base.
FROM minizinc/mznc2025:latest

# Copy the Atlantis installation to /atlantis in the final image.
COPY --from=builder /install /atlantis
COPY atlantis.mpc /minizinc/base.mpc

# atlantis uses Gecode in order to compute the bounds of variables.
# This 'hack' creates a bash file that prepends the '--use-gecode' flag
# to the minzinc command:
RUN MZN_PATH=`which minizinc` && \
    mv ${MZN_PATH} ${MZN_PATH}_orig && \
    echo '#!'"`which bash`" > ${MZN_PATH} && \
    echo "${MZN_PATH}_orig --use-gecode "'${@}' >> ${MZN_PATH} && \
    chmod a+x ${MZN_PATH}

# Add Atlanstis to the MiniZinc search path and set it as the default solver
RUN echo '{"mzn_solver_path": ["/atlantis/share/minizinc/solvers"],' > $HOME/.minizinc/Preferences.json && \
    echo '"tagDefaults": [["", "se.uu.it.atlantis"]]}'              >> $HOME/.minizinc/Preferences.json
