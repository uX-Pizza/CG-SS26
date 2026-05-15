#!/bin/bash

echo "-------------------glew-------------------"

cd libs/glew-glew-2.3.1/auto
make all
cd ..
make

echo "-------------------freeglut-------------------"

cd ../freeglut-3.8.0/
cmake . -DFREEGLUT_COCOA=on
make