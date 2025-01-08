#!/bin/sh
mkdir -p cmake-build
cd cmake-build
cmake .. -G "Unix Makefiles"
make
yes | cp ./BingHomePicDownloader ..
cd ..
rm -rf cmake-build
