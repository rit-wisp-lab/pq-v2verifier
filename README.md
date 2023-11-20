# PQ-V2Verifier (Artifact Version)

This is a preliminary version of PQ-V2Verifier, an extension of [V2Verifier](https://github.com/twardokus/v2verifier) 
that enables experimentation with post-quantum (PQ) authentication algorithms and protocols for vehicle-to-vehicle (V2V)
communication. This repository is made available as part of an artifact submission to the 2024 Network and Distributed 
System Security Symposium (NDSS), associated with the following paper:

Geoff Twardokus, Nina Bindel, Hanif Rahbari, and Sarah McCarthy, "When Cryptography Needs a Hand: Practical Post-Quantum Authentication for V2V Communications," _Network and Distributed System Security Symposium (NDSS 2024)_, San Diego, CA, Feb. 2024.

Bibtex:

    @inproceedings{twardokus2024when,
        author = "Geoff Twardokus and Nina Bindel and Hanif Rahbari and Sarah McCarthy",
        title = "When Cryptography Needs a Hand: Practical Post-Quantum Authentication for {V2V} Communications",
        booktitle = "Proc. Network and Distributed System Security Symposium (NDSS)",
        month = feb,
        year = "2024",
        address = "San Diego, CA",
        pages = ""
    }

## Repository Structure
This repository is organized in several directories, as follows:

- `cert_keys` - ten ECDSA keypairs to be used for signing BSMs
- `falcon_keys` - ten Falcon-512 keypairs to be used for signing certificates (in the context of the paper,
these are the keys a CA would use to sign its certificates and guarantee their integrity under the _Partially Hybrid_
protocol for V2V)
- `include` - C++ header (.h) library files
- `keys` - ten ECDSA keypairs to be used for signing certificates (to be used for guaranteeing certificate integrity
under both classical and _Partially Hybrid_ designs)
- `scripts` - a Python script to generate keypairs for ECDSA and various post-quantum algorithms
- `src` - C++ source files (.cpp) for PQ-V2Verifier
- `trace_files` - sample coordinate trace files for vehicles to use for location data in simulated BSMs

### Hardware
No hardware is required beyond a "commodity PC." As defined by the
[NDSS 2024 Call for Artifacts](https://web.archive.org/web/20230930035437/https://secartifacts.github.io/ndss2024/call),
"[a] commodity desktop machine is defined as one with an x86-64 CPU with 8 cores and 16 GB of RAM running a recent Linux
or Windows operating system."

### Software
An Ubuntu 22.04 virtual machine containing the pre-configured artifact is available from [](). This virtual machine was
created and evaluated using Virtualbox version 7.0.8 r156879 (Qt5.15.2), which is freely available from Oracle at
https://www.virtualbox.org. We recommend users and evaluators use the same version to ensure consistency.

## Installation

*For artifact evaluation at NDSS 2024, a pre-configured virtual machine is available [here](https://bit.ly/3ruvzsH)*.

It is recommended to begin with a fresh installation of Ubuntu 22.04.

### Setup
Begin by updating all Ubuntu packages to their latest versions:

    sudo apt update
    sudo apt upgrade -y

Next, install dependencies that are required for *PQ-V2Verifier*:

    sudo apt install -y libboost-dev git
    sudo apt install -y astyle cmake gcc ninja-build libssl-dev python3-pytest python3-pytest-xdist unzip xsltproc doxygen graphviz python3-yaml valgrind

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

## Running *PQ-V2Verifier* in test mode 
*These instructions are specifically for artifact evaluation or other scenarios where no software-defined radio are available 
for over-the-air experiments*

*PQ-V2Verifier* is released to the community in open source in order to support study of the integration of post-quantum cryptography into V2V
environments that use the IEEE 1609.2 standard protocols for security. As such, the testbed code does not come with pre-built post-quantum protocols;
rather, we supply the starter code needed to rapidly prototype and implement post-quantum V2V authentication protocols of indefinite design using
NIST-approved post-quantum algorithms. We include starter code to support signing and verifying messages using the Falcon-512
algorithm (see `src/v2vcrypto.cpp`), as this algorithm is very likely the only NIST-approved post-quantum algorithm that can feasibly 
be used in real-world V2V applications (see our NDSS 2024 paper, to be linked here upon publication in Feburary 2024, for details).

*PQ-V2Verifier* comes set up to run with the current IEEE 1609.2 protocol, which uses ECDSA P.256 digital signatures for message signing and
verification. A functional example can be run as follows:

Open two terminal windows (in simulation with no radio devices, one will play the role of a vehicle transmitting signed messages and the other 
will play the role of receiving and verifying those messages).

In the first terminal, start the receiver:

    cd $HOME/pq-v2verifier
    ./build/pq-v2verifier dsrc receiver nogui --test

The last command will start a *PQ-V2Verifier* instance configured to use the DSRC communication protocol (ignored in this simulation mode), play the role of the receiver, avoid using any graphical user interface (such interfaces are inherited from *V2Verifier* and not yet supported in *PQ-V2Verifier*), and run in test mode (i.e., with no radio devices).

In the second terminal, start the transmitter:

    cd $HOME/pq-v2verifier
    ./build/pq-v2verifier dsrc transmitter nogui --test

This will start a second *PQ-V2Verifier* instance that again uses DSRC, this time plays the role of the transmitter, does not use a graphical interface, and runs in test mode.

In the transmitter terminal, you should immediately see output indicating that messages are being transmitted. In an experimental setup with radio devices, this would send the V2V messages to a radio for over-the-air transmission; here, they are simply sent through a network pipe to the receiving terminal.

In the receiver terminal window, you should see output indicating that messages are being received, the contents of those messages (e.g., the stated heading of the transmitting "vehicle"), and an indication of whether or not the digital signature on each received message has been successfully verified.

If all has gone as described above, the availability and functionality of *PQ-V2Verifier* has been demonstrated. 
