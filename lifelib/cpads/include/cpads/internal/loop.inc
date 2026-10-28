#ifdef __CUDA_ARCH__
#pragma unroll
for (size_t i = 0; i < N; i++)
#else
for (size_t i = 0; i < N; i++)
#endif
