# PQ-V2Verifier (Artifact Version)

This is a preliminary version of PQ-V2Verifier, an extension of [V2Verifier](https://github.com/twardokus/v2verifier) 
that enables experimentation with post-quantum (PQ) authentication algorithms and protocols for vehicle-to-vehicle (V2V)
communication.

## Installation

*For artifact evaluation at NDSS 2024, a pre-configured virtual machine is available at [url](url_here)*.

It is recommended to begin with a fresh installation of Ubuntu 22.04.

### Setup
Begin by updating all Ubuntu packages to their latest versions:

  sudo apt update
  sudo apt upgrade -y

Next, install dependencies that are required for *PQ-V2Verifier*:

  sudo apt install -y libboost-dev git
  sudo apt install -7 astyle cmake gcc ninja-build libssl-dev python3-pytest python3-pytest-xdist unzip xsltproc doxygen graphviz python3-yaml valgrind

### Installing liboqs

Start by creating a placeholder directory for liboqs to be installed into:

    mkdir liboqs-x86

Now, obtain the liboqs source code from GitHub:

    cd $HOME
    git clone -b main https://github.com/open-quantum-safe/liboqs.git

Now, build liboqs:

    cd $HOME
    git clone -b main https://github.com/open-quantum-safe/liboqs.git
    mv liboqs liboqs-intel
    cd liboqs-intel
    mkdir build && cd build
    cmake -GNinja -DOQS_USE_OPENSSL=OFF -DCMAKE_INSTALL_PREFIX=$HOME/liboqs-x86 ..
    ninja
    ninja install

### Install *PQ-V2Verifier*

Obtain the *PQ-V2Verifier* source code:

    cd $HOME
    git clone https://github.com/twardokus/pq-v2verifier.git

  Build *PQ-V2Verifier*:

    cd pq-v2verifier
    mkdir build && cd build
    cmake ../
    make

  ## Running *PQ-V2Verifier*
