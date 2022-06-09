#!/bin/bash

# wsServer dependency
git clone https://github.com/Theldus/wsServer dependencies/wsServer
cd dependencies/wsServer
mkdir build
cd build
cmake ..
make
cd ../../../

# json-c dependency
git clone https://github.com/json-c/json-c dependencies/json-c
cd dependencies/json-c
mkdir build
cd build
cmake -D CMAKE_MACOSX_RPATH=1 ../
make
cd ../../../

# build
cmake ./
make
