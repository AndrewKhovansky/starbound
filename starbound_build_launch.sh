#!/bin/bash

gcc -O2 -o starbound main.c libbmp.c -lm 
 
./starbound sky.bmp
./starbound sky2.bmp
./starbound sky3.bmp