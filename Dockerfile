FROM debian:bullseye

RUN apt-get update && apt-get -y install gnupg wget
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|apt-key add -
RUN echo "deb http://apt.llvm.org/bullseye/ llvm-toolchain-bullseye-14 main" > /etc/apt/sources.list.d/clang.list

# Install dependencies
RUN apt-get -qq update && \
    apt-get install -qqy --no-install-recommends \
        clang-14 lldb-14 lld-14 ca-certificates \
        autoconf automake cmake dpkg-dev file git make patch \
        libc-dev libc++-dev libgcc-10-dev libstdc++-10-dev  \
        dirmngr gnupg2 lbzip2 wget xz-utils libtinfo5 && \
    rm -rf /var/lib/apt/lists/*

ADD . /typerunner
WORKDIR /typerunner
RUN mkdir build
RUN cd build && cmake -DCMAKE_CXX_COMPILER=clang++-14 -DCMAKE_C_COMPILER=clang-14 -DCMAKE_BUILD_TYPE=Release ..
RUN cd build && make bench typescript_main -j 8
RUN ./build/bench tests/objectLiterals1.ts
RUN ./build/typescript_main tests/objectLiterals1.ts