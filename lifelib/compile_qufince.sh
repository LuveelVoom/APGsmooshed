#!/bin/bash
set -e
nvcc -O3 -lineinfo -Xptxas=-v -c cuda2/qufince_kernel.cu -o qufince_kernel.o
g++ -O3 -c cuda2/qufince_main.cpp -o qufince_main.o
nvcc qufince_main.o qufince_kernel.o -o qufince
