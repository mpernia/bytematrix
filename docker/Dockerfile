# syntax=docker/dockerfile:1
#
# Isolated build environment for the bytematrix monorepo.
#
# Toolchain: Debian (trixie) + GCC 14 (C++20) + CMake + GoogleTest + gdb/valgrind.
# The image is only a toolchain: the sources are bind-mounted at /workspace, so
# nothing of the project is compiled into the image.
FROM debian:trixie-slim

ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    CC=gcc-14 \
    CXX=g++-14

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        ca-certificates \
        git \
        build-essential \
        make \
        cmake \
        pkg-config \
        gcc-14 \
        g++-14 \
        gdb \
        valgrind \
        libgtest-dev \
        libgmock-dev \
 && rm -rf /var/lib/apt/lists/*

# Debian sometimes ships GoogleTest as sources only. Provide the static archives
# so `find_package(GTest)` resolves without network access inside the container.
RUN set -eux; \
    if ! ls /usr/lib/*/libgtest.a >/dev/null 2>&1; then \
        cmake -S /usr/src/googletest -B /tmp/googletest -DCMAKE_BUILD_TYPE=Release >/dev/null; \
        cmake --build /tmp/googletest -j "$(nproc)" >/dev/null; \
        cmake --install /tmp/googletest --prefix /usr/local >/dev/null; \
        rm -rf /tmp/googletest; \
    fi

# Non-root user matching the host UID/GID, so bind-mounted build artifacts are
# not owned by root.
RUN set -eux; \
    groupadd --gid "${USER_GID}" "${USERNAME}"; \
    useradd --uid "${USER_UID}" --gid "${USER_GID}" --create-home --shell /bin/bash "${USERNAME}"

USER ${USERNAME}
WORKDIR /workspace

CMD ["bash"]
