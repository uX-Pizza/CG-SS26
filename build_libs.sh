#!/bin/bash

echo "cmake glew"

cd ../glew-glew-2.3.1/auto
make all
cd ..
make

echo "-------------------freeglut-------------------"

echo "cmake freeglut"
cd libs/freeglut-3.8.0/
cmake . -DFREEGLUT_COCOA=on
echo "make"
make
echo "make install"
sudo make install