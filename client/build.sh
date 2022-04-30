#!/bin/bash

# make sure that g++ compiler is install
sudo apt-get install g++ -y

# install cmake
sudo apt-get install cmake -y

# build the logger application
mkdir src/build   # (PiCAN/client/src/build)
cd src/build
cmake ..
make










