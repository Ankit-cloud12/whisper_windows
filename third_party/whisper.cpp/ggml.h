#ifndef GGML_H
#define GGML_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// GGML API (minimal implementation for whisper.cpp integration)

// Log levels
enum ggml_log_level {
    GGML_LOG_LEVEL_ERROR = 2,
    GGML_LOG_LEVEL_WARN  = 3,
    GGML_LOG_LEVEL_INFO  = 4,
    GGML_LOG_LEVEL_DEBUG = 5,
};

// Tensor types
enum ggml_type {
    GGML_TYPE_F32  = 0,
    GGML_TYPE_F16  = 1,
    GGML_TYPE_Q4_0 = 2,
    GGML_TYPE_Q4_1 = 3,
    GGML_TYPE_Q5_0 = 6,
    GGML_TYPE_Q5_1 = 7,
    GGML_TYPE_Q8_0 = 8,
    GGML_TYPE_Q8_1 = 9,
    GGML_TYPE_Q2_K = 10,
    GGML_TYPE_Q3_K = 11,
    GGML_TYPE_Q4_K = 12,
    GGML_TYPE_Q5_K = 13,
    GGML_TYPE_Q6_K = 14,
    GGML_TYPE_Q8_K = 15,
    GGML_TYPE_I8,
    GGML_TYPE_I16,
    GGML_TYPE_I32,
    GGML_TYPE_COUNT,
};

// Forward declarations
struct ggml_context;
struct ggml_tensor;

typedef struct ggml_context ggml_context;
typedef struct ggml_tensor  ggml_tensor;

// Context parameters
struct ggml_init_params {
    size_t mem_size;   // bytes
    void * mem_buffer; // if NULL, memory will be allocated internally
    bool   no_alloc;   // don't allocate memory for the tensor data
};

// Tensor struct (simplified)
struct ggml_tensor {
    enum ggml_type type;
    
    int64_t ne[4]; // number of elements
    size_t  nb[4]; // stride in bytes
    
    // compute data
    enum ggml_op op;
    
    // op params - allocated as int32_t for alignment
    int32_t op_params[16];
    
    bool is_param;
    
    struct ggml_tensor * grad;
    struct ggml_tensor * src[2];
    
    // performance
    int     perf_runs;
    int64_t perf_cycles;
    int64_t perf_time_us;
    
    void * data;
    
    char name[64];
    
    void * extra; // extra things e.g. for ggml-cuda.cu
};

// Operations
enum ggml_op {
    GGML_OP_NONE = 0,
    
    GGML_OP_DUP,
    GGML_OP_ADD,
    GGML_OP_ADD1,
    GGML_OP_ACC,
    GGML_OP_SUB,
    GGML_OP_MUL,
    GGML_OP_DIV,
    GGML_OP_SQR,
    GGML_OP_SQRT,
    GGML_OP_LOG,
    GGML_OP_SUM,
    GGML_OP_SUM_ROWS,
    GGML_OP_MEAN,
    GGML_OP_ARGMAX,
    GGML_OP_REPEAT,
    GGML_OP_REPEAT_BACK,
    GGML_OP_CONCAT,
    GGML_OP_SILU_BACK,
    GGML_OP_NORM, // normalize
    GGML_OP_RMS_NORM,
    GGML_OP_RMS_NORM_BACK,
    GGML_OP_GROUP_NORM,
    
    GGML_OP_MUL_MAT,
    GGML_OP_OUT_PROD,
    
    GGML_OP_SCALE,
    GGML_OP_SET,
    GGML_OP_CPY,
    GGML_OP_CONT,
    GGML_OP_RESHAPE,
    GGML_OP_VIEW,
    GGML_OP_PERMUTE,
    GGML_OP_TRANSPOSE,
    GGML_OP_GET_ROWS,
    GGML_OP_GET_ROWS_BACK,
    GGML_OP_DIAG,
    GGML_OP_DIAG_MASK_INF,
    GGML_OP_DIAG_MASK_ZERO,
    GGML_OP_SOFT_MAX,
    GGML_OP_SOFT_MAX_BACK,
    GGML_OP_ROPE,
    GGML_OP_ROPE_BACK,
    GGML_OP_ALIBI,
    GGML_OP_CLAMP,
    GGML_OP_CONV_1D,
    GGML_OP_CONV_1D_STAGE_0,
    GGML_OP_CONV_1D_STAGE_1,
    GGML_OP_CONV_TRANSPOSE_1D,
    GGML_OP_CONV_2D,
    GGML_OP_CONV_2D_STAGE_0,
    GGML_OP_CONV_2D_STAGE_1,
    GGML_OP_CONV_TRANSPOSE_2D,
    GGML_OP_POOL_1D,
    GGML_OP_POOL_2D,
    GGML_OP_UPSCALE,
    
    GGML_OP_FLASH_ATTN,
    GGML_OP_FLASH_FF,
    GGML_OP_FLASH_ATTN_BACK,
    GGML_OP_WIN_PART,
    GGML_OP_WIN_UNPART,
    GGML_OP_GET_REL_POS,
    GGML_OP_ADD_REL_POS,
    
    GGML_OP_UNARY,
    
    GGML_OP_MAP_UNARY,
    GGML_OP_MAP_BINARY,
    
    GGML_OP_MAP_CUSTOM1_F32,
    GGML_OP_MAP_CUSTOM2_F32,
    GGML_OP_MAP_CUSTOM3_F32,
    
    GGML_OP_MAP_CUSTOM1,
    GGML_OP_MAP_CUSTOM2,
    GGML_OP_MAP_CUSTOM3,
    
    GGML_OP_CROSS_ENTROPY_LOSS,
    GGML_OP_CROSS_ENTROPY_LOSS_BACK,
    
    GGML_OP_COUNT,
};

// Basic functions
struct ggml_context * ggml_init(struct ggml_init_params params);
void ggml_free(struct ggml_context * ctx);

size_t ggml_used_mem(const struct ggml_context * ctx);

// Tensor creation
struct ggml_tensor * ggml_new_tensor_1d(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int64_t ne0);

struct ggml_tensor * ggml_new_tensor_2d(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int64_t ne0,
        int64_t ne1);

struct ggml_tensor * ggml_new_tensor_3d(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int64_t ne0,
        int64_t ne1,
        int64_t ne2);

struct ggml_tensor * ggml_new_tensor_4d(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int64_t ne0,
        int64_t ne1,
        int64_t ne2,
        int64_t ne3);

// Tensor operations
struct ggml_tensor * ggml_add(
        struct ggml_context * ctx,
        struct ggml_tensor  * a,
        struct ggml_tensor  * b);

struct ggml_tensor * ggml_mul(
        struct ggml_context * ctx,
        struct ggml_tensor  * a,
        struct ggml_tensor  * b);

struct ggml_tensor * ggml_mul_mat(
        struct ggml_context * ctx,
        struct ggml_tensor  * a,
        struct ggml_tensor  * b);

// Computation
void ggml_compute_forward(struct ggml_tensor * tensor, void * ctx);

// Utilities
size_t ggml_type_size(enum ggml_type type);
const char * ggml_type_name(enum ggml_type type);
const char * ggml_op_name(enum ggml_op op);
bool ggml_is_quantized(enum ggml_type type);

// For CUDA support
void ggml_cuda_assign_buffers(struct ggml_tensor * tensor);
void ggml_cuda_assign_buffers_no_scratch(struct ggml_tensor * tensor);
void ggml_cuda_assign_buffers_force_inplace(struct ggml_tensor * tensor);
void ggml_cuda_assign_buffers_no_alloc(struct ggml_tensor * tensor);
void ggml_cuda_assign_scratch_offset(struct ggml_tensor * tensor, size_t offset);
void ggml_cuda_copy_to_device(struct ggml_tensor * tensor);
void ggml_cuda_set_main_device(int main_device);
void ggml_cuda_set_scratch_size(size_t scratch_size);
void ggml_cuda_free_scratch(void);
bool ggml_cuda_compute_forward(struct ggml_tensor * tensor);

#ifdef __cplusplus
}
#endif

#endif // GGML_H