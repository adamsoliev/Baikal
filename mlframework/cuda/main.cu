/*
Google Colab
!nvcc -c main.cu -o main
!./main
*/

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <cublas_v2.h>
#include <library_types.h>
#include <cuda_runtime.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void cudaCheck(cudaError_t error, const char *file, int line) {
  if (error != cudaSuccess) {
    printf("[CUDA ERROR] at file %s:%d:\n%s\n", file, line,
           cudaGetErrorString(error));
    exit(EXIT_FAILURE);
  }
};

#define cudaCheck(err) (cudaCheck(err, __FILE__, __LINE__))

void randomize_matrix(float *mat, int N) {
  // NOTICE: Use gettimeofday instead of srand((unsigned)time(NULL)); the time
  // precision is too low and the same random number is generated.
  struct timeval time {};
  gettimeofday(&time, nullptr);
  srand(time.tv_usec);
  for (int i = 0; i < N; i++) {
    float tmp = (float)(rand() % 5) + 0.01 * (rand() % 5);
    tmp = (rand() % 2 == 0) ? tmp : tmp * (-1.);
    mat[i] = tmp;
  }
}

bool verify_matrix(float *matRef, float *matOut, int N) {
  double diff = 0.0;
  int i;
  for (i = 0; i < N; i++) {
    diff = std::fabs(matRef[i] - matOut[i]);
    if (diff > 0.01) {
      printf("Divergence! Should %5.2f, Is %5.2f (Diff %5.2f) at %d\n",
             matRef[i], matOut[i], diff, i);
      return false;
    }
  }
  return true;
}

void runCublasFP32(cublasHandle_t handle, int M, int N, int K, float alpha,
                   float *A, float *B, float beta, float *C) {
  // cuBLAS uses column-major order. So we change the order of our row-major A &
  // B, since (B^T*A^T)^T = (A*B)
  // This runs cuBLAS in full fp32 mode
  cublasGemmEx(handle, CUBLAS_OP_N, CUBLAS_OP_N, N, M, K, &alpha, B, CUDA_R_32F,
               N, A, CUDA_R_32F, K, &beta, C, CUDA_R_32F, N, CUBLAS_COMPUTE_32F,
               CUBLAS_GEMM_DEFAULT_TENSOR_OP);
}

void run_kernel(int kernel_num, int M, int N, int K, float alpha, float *A,
                float *B, float beta, float *C, cublasHandle_t handle) {
  switch (kernel_num) {
  case 0:
    runCublasFP32(handle, M, N, K, alpha, A, B, beta, C);
    break;
  // case 1:
  //   run_sgemm_naive(M, N, K, alpha, A, B, beta, C);
    // break;
  // case 2:
  //   run_sgemm_coalesce(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 3:
  //   run_sgemm_shared_mem_block(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 4:
  //   runSgemm1DBlocktiling(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 5:
  //   runSgemm2DBlocktiling(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 6:
  //   runSgemmVectorize(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 7:
  //   runSgemmResolveBankConflicts(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 8:
  //   runSgemmResolveBankExtraCol(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 9:
  //   runSgemmAutotuned(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 10:
  //   runSgemmWarptiling(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 11:
  //   runSgemmDoubleBuffering(M, N, K, alpha, A, B, beta, C);
  //   break;
  // case 12:
  //   runSgemmDoubleBuffering2(M, N, K, alpha, A, B, beta, C);
  //   break;
  default:
    throw std::invalid_argument("Unknown kernel number");
  }
}

int main(int argc, char **argv) {
    cublasHandle_t handle;
    if (cublasCreate(&handle)) {
      std::cerr << "Create cublas handle error." << std::endl;
      exit(EXIT_FAILURE);
    };

    float elapsed_time;
    cudaEvent_t beg, end;
    cudaEventCreate(&beg);
    cudaEventCreate(&end);
    
    long m, n, k, max_size;
    max_size = 4096;

    float alpha = 0.5, beta = 3.0; // GEMM input parameters, C=α*AB+β*C

    float *A = nullptr, *B = nullptr, *C = nullptr,
      *C_ref = nullptr; // host matrices
    float *dA = nullptr, *dB = nullptr, *dC = nullptr,
      *dC_ref = nullptr; // device matrices

    A = (float *)malloc(sizeof(float) * max_size * max_size);
    B = (float *)malloc(sizeof(float) * max_size * max_size);
    C = (float *)malloc(sizeof(float) * max_size * max_size);
    C_ref = (float *)malloc(sizeof(float) * max_size * max_size);

    randomize_matrix(A, max_size * max_size);
    randomize_matrix(B, max_size * max_size);
    randomize_matrix(C, max_size * max_size);

    cudaCheck(cudaMalloc((void **)&dA, sizeof(float) * max_size * max_size));
    cudaCheck(cudaMalloc((void **)&dB, sizeof(float) * max_size * max_size));
    cudaCheck(cudaMalloc((void **)&dC, sizeof(float) * max_size * max_size));
    cudaCheck(cudaMalloc((void **)&dC_ref, sizeof(float) * max_size * max_size));
  
    cudaCheck(cudaMemcpy(dA, A, sizeof(float) * max_size * max_size,
                        cudaMemcpyHostToDevice));
    cudaCheck(cudaMemcpy(dB, B, sizeof(float) * max_size * max_size,
                        cudaMemcpyHostToDevice));
    cudaCheck(cudaMemcpy(dC, C, sizeof(float) * max_size * max_size,
                        cudaMemcpyHostToDevice));
    cudaCheck(cudaMemcpy(dC_ref, C, sizeof(float) * max_size * max_size,
                       cudaMemcpyHostToDevice));

    int kernel_num = 0;

    int size = 1024;
    m = n = k = size;
    run_kernel(0, m, n, k, alpha, dA, dB, beta, dC_ref, handle); // cuBLAS
    run_kernel(kernel_num, m, n, k, alpha, dA, dB, beta, dC, handle); // Executes the kernel, modifies the result matrix
    if (!verify_matrix(C_ref, C, m * n)) {
      std::cout << "Failed to pass the correctness verification against NVIDIA cuBLAS." << std::endl;
    }

    cudaEventRecord(beg);
    // run kernel here
    run_kernel(kernel_num, m, n, k, alpha, dA, dB, beta, dC, handle);
    cudaEventRecord(end);
    
    cudaEventSynchronize(beg);
    cudaEventSynchronize(end);
    cudaEventElapsedTime(&elapsed_time, beg, end);
    elapsed_time /= 1000.; // Convert to seconds
    printf("Elapsed time: (%7.9f) s\n", elapsed_time);
}