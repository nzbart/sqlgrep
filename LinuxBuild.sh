#!/bin/bash
rm -rf sqlgrep_bin
mkdir sqlgrep_bin
cd sqlgrep_bin
cmake .. -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build .
