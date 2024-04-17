#!/bin/zsh -feu
set -x

cmake . -B build -G Ninja &&
    cmake --build build/ -v &&
    DESTDIR=/tmp/ cmake --install  build -v
