# Dockerfile derived from easy::jit's .travis.yml

FROM ubuntu:latest

LABEL manteiner Juan Manuel Martinez Caamaño jmartinezcaamao@gmail.com

ARG branch=master

# add sources
RUN apt-get update
RUN apt-get install -y software-properties-common
RUN apt-add-repository -y "ppa:ubuntu-toolchain-r/test" 
RUN deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-11 main | tee -a /etc/apt/sources.list > /dev/null 
 
# install apt packages, base first, then travis
RUN apt-get update 
RUN apt-get upgrade -y 

RUN apt-get install wget curl -y

RUN wget https://apt.llvm.org/llvm.sh
RUN chmod +x llvm.sh
RUN ./llvm.sh 11

RUN apt-get install -y ninja-build g++-11 libopencv-dev
RUN apt-get install -y build-essential git unzip cmake 

RUN add-apt-repository universe
RUN apt install python2 -y
RUN curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
RUN python2 get-pip.py 

COPY . ./easy-just-in-time

# install other deps
RUN cd easy-just-in-time && pip install --user --upgrade "pip < 21.0"
RUN cd easy-just-in-time && pip install --user -Iv lit==0.7.0

# compile and test!
RUN cd easy-just-in-time && \
  mkdir _build && cd _build && \ 
  git clone --depth=1 https://github.com/google/benchmark.git && git clone --depth=1 https://github.com/google/googletest.git benchmark/googletest

RUN cd easy-just-in-time/_build && mkdir benchmark/_build && cd benchmark/_build && cmake .. -GNinja -DCMAKE_INSTALL_PREFIX=`pwd`/../_install && ninja && ninja install 
RUN cd easy-just-in-time/_build && cmake -DLLVM_DIR=/usr/lib/llvm-11/cmake -DCMAKE_CXX_COMPILER=clang++-11 -DCMAKE_C_COMPILER=clang-11 -DEASY_JIT_EXAMPLE=ON -DEASY_JIT_BENCHMARK=ON -DBENCHMARK_DIR=`pwd`/benchmark/_install -DCMAKE_INSTALL_PREFIX=`pwd`/../_install .. -G Ninja
# RUN cd easy-just-in-time/_build && ninja && ninja install && ninja check && echo ok!
