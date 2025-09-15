#!/bin/bash
mkdir build_gcc
gcc -O2 -o build_gcc/starbound main.c libbmp.c -lm 
 