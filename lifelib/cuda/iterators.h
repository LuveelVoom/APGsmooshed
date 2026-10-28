#pragma once

#include "basics.h"

/*
* Takes a 64x64 universe stored across register 'a' of a block of
* 64 CUDA threads, iterates it one generation (with toroidal wrap),
* and stores the result in register 'b'. The pointers 'tmp' and 'dst'
* must refer to disjoint 512-byte blocks of shared memory.
*/
#include "../avxlife/lifelogic/iterators_gpu.h"
