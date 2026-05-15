#!/bin/bash

cd libs/freeglut-3.8.0/
cmake . -DFREEGLUT_COCOA=on
make
sudo make install

cd ../glew-glew-2.3.1/build
cmake ./cmake
make
sudo make install