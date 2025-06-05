#ifndef GGML_QUANTS_H
#define GGML_QUANTS_H

#include "ggml.h"

#ifdef __cplusplus
extern "C" {
#endif

// Quantization functions

// Q4_0 quantization (4-bit with FP16 scale)
void quantize_row_q4_0_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q4_0(const float * restrict x, void * restrict y, int k);
void dequantize_row_q4_0(const void * restrict x, float * restrict y, int k);

// Q4_1 quantization (4-bit with FP16 scale and min)
void quantize_row_q4_1_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q4_1(const float * restrict x, void * restrict y, int k);
void dequantize_row_q4_1(const void * restrict x, float * restrict y, int k);

// Q5_0 quantization (5-bit with FP16 scale)
void quantize_row_q5_0_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q5_0(const float * restrict x, void * restrict y, int k);
void dequantize_row_q5_0(const void * restrict x, float * restrict y, int k);

// Q5_1 quantization (5-bit with FP16 scale and min)
void quantize_row_q5_1_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q5_1(const float * restrict x, void * restrict y, int k);
void dequantize_row_q5_1(const void * restrict x, float * restrict y, int k);

// Q8_0 quantization (8-bit with FP32 scale)
void quantize_row_q8_0_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q8_0(const float * restrict x, void * restrict y, int k);
void dequantize_row_q8_0(const void * restrict x, float * restrict y, int k);

// Q8_1 quantization (8-bit with FP32 scale and sum)
void quantize_row_q8_1_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q8_1(const float * restrict x, void * restrict y, int k);
void dequantize_row_q8_1(const void * restrict x, float * restrict y, int k);

// K-quantization methods
void quantize_row_q2_K_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q2_K(const float * restrict x, void * restrict y, int k);
void dequantize_row_q2_K(const void * restrict x, float * restrict y, int k);

void quantize_row_q3_K_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q3_K(const float * restrict x, void * restrict y, int k);
void dequantize_row_q3_K(const void * restrict x, float * restrict y, int k);

void quantize_row_q4_K_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q4_K(const float * restrict x, void * restrict y, int k);
void dequantize_row_q4_K(const void * restrict x, float * restrict y, int k);

void quantize_row_q5_K_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q5_K(const float * restrict x, void * restrict y, int k);
void dequantize_row_q5_K(const void * restrict x, float * restrict y, int k);

void quantize_row_q6_K_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q6_K(const float * restrict x, void * restrict y, int k);
void dequantize_row_q6_K(const void * restrict x, float * restrict y, int k);

void quantize_row_q8_K_reference(const float * restrict x, void * restrict y, int k);
void quantize_row_q8_K(const float * restrict x, void * restrict y, int k);
void dequantize_row_q8_K(const void * restrict x, float * restrict y, int k);

// Vector dot product functions
void ggml_vec_dot_q4_0_q8_0(int n, float * restrict s, const void * restrict vx, const void * restrict vy);
void ggml_vec_dot_q4_1_q8_1(int n, float * restrict s, const void * restrict vx, const void * restrict vy);
void ggml_vec_dot_q5_0_q8_0(int n, float * restrict s, const void * restrict vx, const void * restrict vy);
void ggml_vec_dot_q5_1_q8_1(int n, float * restrict s, const void * restrict vx, const void * restrict vy);
void ggml_vec_dot_q8_0_q8_0(int n, float * restrict s, const void * restrict vx, const void * restrict vy);

// Utility functions
size_t ggml_quantize_q4_0(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q4_1(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q5_0(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q5_1(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q8_0(const float * src, void * dst, int n, int k, int64_t * hist);

size_t ggml_quantize_q2_K(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q3_K(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q4_K(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q5_K(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q6_K(const float * src, void * dst, int n, int k, int64_t * hist);
size_t ggml_quantize_q8_K(const float * src, void * dst, int n, int k, int64_t * hist);

#ifdef __cplusplus
}
#endif

#endif // GGML_QUANTS_H