#!/bin/bash

cd libs/freeglut-3.8.0/
cmake . -DFREEGLUT_COCOA=on
make
make install

cd ../glew-glew-2.3.1/
mkdir build
cd build
cmake .
make
make install