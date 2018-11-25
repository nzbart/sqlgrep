#!/bin/bash
rm -rf sqlgrep_bin
mkdir sqlgrep_bin
cd sqlgrep_bin
cmake ..
cmake --build . --config MinSizeRel
