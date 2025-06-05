#include "ggml.h"
#include "ggml-impl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

// Context structure
struct ggml_context {
    size_t mem_size;
    void * mem_buffer;
    bool   no_alloc;
    size_t mem_used;
    
    struct ggml_tensor * tensors;
    int n_tensors;
};

// Type information
static const size_t type_sizes[GGML_TYPE_COUNT] = {
    [GGML_TYPE_F32]  = sizeof(float),
    [GGML_TYPE_F16]  = sizeof(uint16_t),
    [GGML_TYPE_Q4_0] = sizeof(uint32_t) + 16*sizeof(uint8_t),
    [GGML_TYPE_Q4_1] = sizeof(float) + sizeof(uint32_t) + 16*sizeof(uint8_t),
    [GGML_TYPE_Q5_0] = sizeof(uint32_t) + sizeof(uint32_t) + 16*sizeof(uint8_t),
    [GGML_TYPE_Q5_1] = sizeof(float) + sizeof(uint32_t) + sizeof(uint32_t) + 16*sizeof(uint8_t),
    [GGML_TYPE_Q8_0] = sizeof(float) + 32*sizeof(uint8_t),
    [GGML_TYPE_Q8_1] = sizeof(float) + sizeof(float) + 32*sizeof(uint8_t),
    [GGML_TYPE_I8]   = sizeof(int8_t),
    [GGML_TYPE_I16]  = sizeof(int16_t),
    [GGML_TYPE_I32]  = sizeof(int32_t),
};

static const char * type_names[GGML_TYPE_COUNT] = {
    [GGML_TYPE_F32]  = "f32",
    [GGML_TYPE_F16]  = "f16",
    [GGML_TYPE_Q4_0] = "q4_0",
    [GGML_TYPE_Q4_1] = "q4_1",
    [GGML_TYPE_Q5_0] = "q5_0",
    [GGML_TYPE_Q5_1] = "q5_1",
    [GGML_TYPE_Q8_0] = "q8_0",
    [GGML_TYPE_Q8_1] = "q8_1",
    [GGML_TYPE_I8]   = "i8",
    [GGML_TYPE_I16]  = "i16",
    [GGML_TYPE_I32]  = "i32",
};

static const char * op_names[GGML_OP_COUNT] = {
    [GGML_OP_NONE] = "NONE",
    [GGML_OP_DUP] = "DUP",
    [GGML_OP_ADD] = "ADD",
    [GGML_OP_ADD1] = "ADD1",
    [GGML_OP_ACC] = "ACC",
    [GGML_OP_SUB] = "SUB",
    [GGML_OP_MUL] = "MUL",
    [GGML_OP_DIV] = "DIV",
    [GGML_OP_SQR] = "SQR",
    [GGML_OP_SQRT] = "SQRT",
    [GGML_OP_LOG] = "LOG",
    [GGML_OP_SUM] = "SUM",
    [GGML_OP_SUM_ROWS] = "SUM_ROWS",
    [GGML_OP_MEAN] = "MEAN",
    [GGML_OP_ARGMAX] = "ARGMAX",
    [GGML_OP_REPEAT] = "REPEAT",
    [GGML_OP_REPEAT_BACK] = "REPEAT_BACK",
    [GGML_OP_CONCAT] = "CONCAT",
    [GGML_OP_SILU_BACK] = "SILU_BACK",
    [GGML_OP_NORM] = "NORM",
    [GGML_OP_RMS_NORM] = "RMS_NORM",
    [GGML_OP_RMS_NORM_BACK] = "RMS_NORM_BACK",
    [GGML_OP_GROUP_NORM] = "GROUP_NORM",
    [GGML_OP_MUL_MAT] = "MUL_MAT",
    [GGML_OP_OUT_PROD] = "OUT_PROD",
    [GGML_OP_SCALE] = "SCALE",
    [GGML_OP_SET] = "SET",
    [GGML_OP_CPY] = "CPY",
    [GGML_OP_CONT] = "CONT",
    [GGML_OP_RESHAPE] = "RESHAPE",
    [GGML_OP_VIEW] = "VIEW",
    [GGML_OP_PERMUTE] = "PERMUTE",
    [GGML_OP_TRANSPOSE] = "TRANSPOSE",
    [GGML_OP_GET_ROWS] = "GET_ROWS",
    [GGML_OP_GET_ROWS_BACK] = "GET_ROWS_BACK",
    [GGML_OP_DIAG] = "DIAG",
    [GGML_OP_DIAG_MASK_INF] = "DIAG_MASK_INF",
    [GGML_OP_DIAG_MASK_ZERO] = "DIAG_MASK_ZERO",
    [GGML_OP_SOFT_MAX] = "SOFT_MAX",
    [GGML_OP_SOFT_MAX_BACK] = "SOFT_MAX_BACK",
    [GGML_OP_ROPE] = "ROPE",
    [GGML_OP_ROPE_BACK] = "ROPE_BACK",
    [GGML_OP_ALIBI] = "ALIBI",
    [GGML_OP_CLAMP] = "CLAMP",
    [GGML_OP_CONV_1D] = "CONV_1D",
    [GGML_OP_CONV_1D_STAGE_0] = "CONV_1D_STAGE_0",
    [GGML_OP_CONV_1D_STAGE_1] = "CONV_1D_STAGE_1",
    [GGML_OP_CONV_TRANSPOSE_1D] = "CONV_TRANSPOSE_1D",
    [GGML_OP_CONV_2D] = "CONV_2D",
    [GGML_OP_CONV_2D_STAGE_0] = "CONV_2D_STAGE_0",
    [GGML_OP_CONV_2D_STAGE_1] = "CONV_2D_STAGE_1",
    [GGML_OP_CONV_TRANSPOSE_2D] = "CONV_TRANSPOSE_2D",
    [GGML_OP_POOL_1D] = "POOL_1D",
    [GGML_OP_POOL_2D] = "POOL_2D",
    [GGML_OP_UPSCALE] = "UPSCALE",
    [GGML_OP_FLASH_ATTN] = "FLASH_ATTN",
    [GGML_OP_FLASH_FF] = "FLASH_FF",
    [GGML_OP_FLASH_ATTN_BACK] = "FLASH_ATTN_BACK",
    [GGML_OP_WIN_PART] = "WIN_PART",
    [GGML_OP_WIN_UNPART] = "WIN_UNPART",
    [GGML_OP_GET_REL_POS] = "GET_REL_POS",
    [GGML_OP_ADD_REL_POS] = "ADD_REL_POS",
    [GGML_OP_UNARY] = "UNARY",
    [GGML_OP_MAP_UNARY] = "MAP_UNARY",
    [GGML_OP_MAP_BINARY] = "MAP_BINARY",
    [GGML_OP_MAP_CUSTOM1_F32] = "MAP_CUSTOM1_F32",
    [GGML_OP_MAP_CUSTOM2_F32] = "MAP_CUSTOM2_F32",
    [GGML_OP_MAP_CUSTOM3_F32] = "MAP_CUSTOM3_F32",
    [GGML_OP_MAP_CUSTOM1] = "MAP_CUSTOM1",
    [GGML_OP_MAP_CUSTOM2] = "MAP_CUSTOM2",
    [GGML_OP_MAP_CUSTOM3] = "MAP_CUSTOM3",
    [GGML_OP_CROSS_ENTROPY_LOSS] = "CROSS_ENTROPY_LOSS",
    [GGML_OP_CROSS_ENTROPY_LOSS_BACK] = "CROSS_ENTROPY_LOSS_BACK",
};

// Utility functions
size_t ggml_type_size(enum ggml_type type) {
    return type_sizes[type];
}

const char * ggml_type_name(enum ggml_type type) {
    return type_names[type];
}

const char * ggml_op_name(enum ggml_op op) {
    return op_names[op];
}

bool ggml_is_quantized(enum ggml_type type) {
    return type == GGML_TYPE_Q4_0 || type == GGML_TYPE_Q4_1 || 
           type == GGML_TYPE_Q5_0 || type == GGML_TYPE_Q5_1 ||
           type == GGML_TYPE_Q8_0 || type == GGML_TYPE_Q8_1 ||
           type == GGML_TYPE_Q2_K || type == GGML_TYPE_Q3_K ||
           type == GGML_TYPE_Q4_K || type == GGML_TYPE_Q5_K ||
           type == GGML_TYPE_Q6_K || type == GGML_TYPE_Q8_K;
}

// Context management
struct ggml_context * ggml_init(struct ggml_init_params params) {
    struct ggml_context * ctx = (struct ggml_context *)malloc(sizeof(struct ggml_context));
    if (!ctx) {
        return NULL;
    }
    
    ctx->mem_size = params.mem_size;
    ctx->no_alloc = params.no_alloc;
    ctx->mem_used = 0;
    ctx->tensors = NULL;
    ctx->n_tensors = 0;
    
    if (params.mem_buffer) {
        ctx->mem_buffer = params.mem_buffer;
    } else {
        ctx->mem_buffer = malloc(params.mem_size);
        if (!ctx->mem_buffer) {
            free(ctx);
            return NULL;
        }
    }
    
    return ctx;
}

void ggml_free(struct ggml_context * ctx) {
    if (!ctx) return;
    
    // Free tensor data if allocated internally
    if (!ctx->no_alloc && ctx->mem_buffer) {
        free(ctx->mem_buffer);
    }
    
    free(ctx);
}

size_t ggml_used_mem(const struct ggml_context * ctx) {
    return ctx ? ctx->mem_used : 0;
}

// Helper function to calculate tensor size
static size_t ggml_tensor_overhead() {
    return sizeof(struct ggml_tensor);
}

static size_t ggml_calc_tensor_size(enum ggml_type type, int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3) {
    size_t type_size = ggml_type_size(type);
    return ne0 * ne1 * ne2 * ne3 * type_size;
}

// Tensor creation
static struct ggml_tensor * ggml_new_tensor_impl(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int                   n_dims,
        const int64_t       * ne) {
    
    if (!ctx) return NULL;
    
    size_t tensor_size = ggml_tensor_overhead();
    size_t data_size = 1;
    
    for (int i = 0; i < n_dims; i++) {
        data_size *= ne[i];
    }
    data_size *= ggml_type_size(type);
    
    // Check if we have enough memory
    if (ctx->mem_used + tensor_size + data_size > ctx->mem_size) {
        return NULL;
    }
    
    // Allocate tensor struct
    struct ggml_tensor * tensor = (struct ggml_tensor *)((char *)ctx->mem_buffer + ctx->mem_used);
    ctx->mem_used += tensor_size;
    
    // Initialize tensor
    memset(tensor, 0, sizeof(struct ggml_tensor));
    tensor->type = type;
    tensor->op = GGML_OP_NONE;
    tensor->is_param = false;
    tensor->grad = NULL;
    tensor->src[0] = NULL;
    tensor->src[1] = NULL;
    tensor->perf_runs = 0;
    tensor->perf_cycles = 0;
    tensor->perf_time_us = 0;
    tensor->extra = NULL;
    
    // Set dimensions
    for (int i = 0; i < 4; i++) {
        tensor->ne[i] = (i < n_dims) ? ne[i] : 1;
    }
    
    // Calculate strides
    tensor->nb[0] = ggml_type_size(type);
    for (int i = 1; i < 4; i++) {
        tensor->nb[i] = tensor->nb[i-1] * tensor->ne[i-1];
    }
    
    // Allocate data if not no_alloc
    if (!ctx->no_alloc) {
        tensor->data = (char *)ctx->mem_buffer + ctx->mem_used;
        ctx->mem_used += data_size;
        memset(tensor->data, 0, data_size);
    } else {
        tensor->data = NULL;
    }
    
    ctx->n_tensors++;
    
    return tensor;
}

struct ggml_tensor * ggml_new_tensor_1d(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int64_t ne0) {
    int64_t ne[1] = { ne0 };
    return ggml_new_tensor_impl(ctx, type, 1, ne);
}

struct ggml_tensor * ggml_new_tensor_2d(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int64_t ne0,
        int64_t ne1) {
    int64_t ne[2] = { ne0, ne1 };
    return ggml_new_tensor_impl(ctx, type, 2, ne);
}

struct ggml_tensor * ggml_new_tensor_3d(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int64_t ne0,
        int64_t ne1,
        int64_t ne2) {
    int64_t ne[3] = { ne0, ne1, ne2 };
    return ggml_new_tensor_impl(ctx, type, 3, ne);
}

struct ggml_tensor * ggml_new_tensor_4d(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int64_t ne0,
        int64_t ne1,
        int64_t ne2,
        int64_t ne3) {
    int64_t ne[4] = { ne0, ne1, ne2, ne3 };
    return ggml_new_tensor_impl(ctx, type, 4, ne);
}

// Tensor operations (mock implementations)
struct ggml_tensor * ggml_add(
        struct ggml_context * ctx,
        struct ggml_tensor  * a,
        struct ggml_tensor  * b) {
    
    if (!ctx || !a || !b) return NULL;
    
    // Create result tensor with same shape as a
    struct ggml_tensor * result = ggml_new_tensor_impl(ctx, a->type, 4, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_ADD;
    result->src[0] = a;
    result->src[1] = b;
    
    return result;
}

struct ggml_tensor * ggml_mul(
        struct ggml_context * ctx,
        struct ggml_tensor  * a,
        struct ggml_tensor  * b) {
    
    if (!ctx || !a || !b) return NULL;
    
    // Create result tensor with same shape as a
    struct ggml_tensor * result = ggml_new_tensor_impl(ctx, a->type, 4, a->ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_MUL;
    result->src[0] = a;
    result->src[1] = b;
    
    return result;
}

struct ggml_tensor * ggml_mul_mat(
        struct ggml_context * ctx,
        struct ggml_tensor  * a,
        struct ggml_tensor  * b) {
    
    if (!ctx || !a || !b) return NULL;
    
    // Result dimensions: [ne00, ne11, ne12, ne13]
    int64_t ne[4] = { a->ne[0], b->ne[1], b->ne[2], b->ne[3] };
    struct ggml_tensor * result = ggml_new_tensor_impl(ctx, a->type, 4, ne);
    if (!result) return NULL;
    
    result->op = GGML_OP_MUL_MAT;
    result->src[0] = a;
    result->src[1] = b;
    
    return result;
}

// Computation (mock implementation)
void ggml_compute_forward(struct ggml_tensor * tensor, void * ctx) {
    (void)ctx; // unused
    
    if (!tensor || !tensor->data) return;
    
    // Mock computation - just zero the output for now
    size_t total_size = tensor->ne[0] * tensor->ne[1] * tensor->ne[2] * tensor->ne[3] * ggml_type_size(tensor->type);
    memset(tensor->data, 0, total_size);
    
    // In a real implementation, this would perform the actual computation
    // based on tensor->op and tensor->src[0], tensor->src[1]
}

// CUDA support stubs
void ggml_cuda_assign_buffers(struct ggml_tensor * tensor) {
    (void)tensor; // Not implemented in mock version
}

void ggml_cuda_assign_buffers_no_scratch(struct ggml_tensor * tensor) {
    (void)tensor; // Not implemented in mock version
}

void ggml_cuda_assign_buffers_force_inplace(struct ggml_tensor * tensor) {
    (void)tensor; // Not implemented in mock version
}

void ggml_cuda_assign_buffers_no_alloc(struct ggml_tensor * tensor) {
    (void)tensor; // Not implemented in mock version
}

void ggml_cuda_assign_scratch_offset(struct ggml_tensor * tensor, size_t offset) {
    (void)tensor;
    (void)offset; // Not implemented in mock version
}

void ggml_cuda_copy_to_device(struct ggml_tensor * tensor) {
    (void)tensor; // Not implemented in mock version
}

void ggml_cuda_set_main_device(int main_device) {
    (void)main_device; // Not implemented in mock version
}

void ggml_cuda_set_scratch_size(size_t scratch_size) {
    (void)scratch_size; // Not implemented in mock version
}

void ggml_cuda_free_scratch(void) {
    // Not implemented in mock version
}

bool ggml_cuda_compute_forward(struct ggml_tensor * tensor) {
    (void)tensor;
    return false; // Not implemented in mock version
}