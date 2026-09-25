#!/bin/bash

SDL_VERSION="3.4.16"
SDL_SHA256="7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68"

if [ ! -d "3rdparty/SDL3" ]; then

    if [ ! -f "SDL3-$SDL_VERSION.tar.gz" ]; then
        wget "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL3-$SDL_VERSION.tar.gz"
    fi

    echo "$SDL_SHA256 SDL3-$SDL_VERSION.tar.gz" | sha256sum -c

    rm -rf "3rdparty"
    mkdir -p "3rdparty"

    tar -xf "SDL3-$SDL_VERSION.tar.gz" -C "3rdparty"
    mv "3rdparty/SDL3-$SDL_VERSION" "3rdparty/SDL3"

fi

emcmake cmake -B build
pushd build
emmake make -j`nproc`
popd
