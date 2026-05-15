#!/bin/bash

echo "cmake glew"

cd ../glew-glew-2.3.1/build
cmake ./cmake
echo "make"
make -j4
echo "make install"
sudo make install

echo "-------------------freeglut-------------------"

echo "cmake freeglut"
cd libs/freeglut-3.8.0/
cmake . -DFREEGLUT_COCOA=on
echo "make"
make
echo "make install"
sudo make install