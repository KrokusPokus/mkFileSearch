#!/bin/bash

if [ ! -d "build" ]; then
    mkdir build
fi

cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

if [ -f "./bin/mkFileSearch" ]; then
    strip ./bin/mkFileSearch

    if [ ! -d "$HOME/.local/bin" ]; then
        mkdir "$HOME/.local/bin"
    fi

    cp ./bin/mkFileSearch $HOME/.local/bin/
fi
