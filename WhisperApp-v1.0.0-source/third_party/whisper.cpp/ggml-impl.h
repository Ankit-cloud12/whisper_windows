#ifndef GGML_IMPL_H
#define GGML_IMPL_H

#include "ggml.h"

#ifdef __cplusplus
extern "C" {
#endif

// Internal implementation details for ggml

// Memory alignment
#define GGML_MEM_ALIGN 16
#define GGML_MAX_DIMS  4
#define GGML_MAX_NODES 16384
#define GGML_MAX_PARAMS 2048
#define GGML_MAX_CONTEXTS 64
#define GGML_MAX_SRC 10
#define GGML_MAX_NAME 64
#define GGML_MAX_OP_PARAMS 64
#define GGML_DEFAULT_N_THREADS 4
#define GGML_DEFAULT_GRAPH_SIZE 2048

// Forward declarations
struct ggml_cgraph;
struct ggml_hash_set;

// Graph structure
struct ggml_cgraph {
    int size;
    int n_nodes;
    int n_leafs;
    
    struct ggml_tensor ** nodes;
    struct ggml_tensor ** grads;
    struct ggml_tensor ** leafs;
    
    struct ggml_hash_set visited_hash_table;
    
    enum ggml_cgraph_eval_order order;
    
    // performance
    int     perf_runs;
    int64_t perf_cycles;
    int64_t perf_time_us;
};

// Graph evaluation order
enum ggml_cgraph_eval_order {
    GGML_CGRAPH_EVAL_ORDER_LEFT_TO_RIGHT = 0,
    GGML_CGRAPH_EVAL_ORDER_RIGHT_TO_LEFT,
    GGML_CGRAPH_EVAL_ORDER_COUNT
};

// Hash set for visited nodes
struct ggml_hash_set {
    size_t size;
    struct ggml_tensor ** keys;
};

// Thread pool
struct ggml_compute_state_shared {
    const struct ggml_cgraph * cgraph;
    const struct ggml_cplan  * cplan;
    
    int64_t perf_node_start_cycles;
    int64_t perf_node_start_time_us;
    
    const int n_threads;
    
    // synchronization primitives
    int n_active; // number of active threads
    int node_n;   // active graph node
    
    bool (*abort_callback)(void * data); // abort ggml_graph_compute when true
    void * abort_callback_data;
};

struct ggml_compute_state {
    struct ggml_compute_state_shared * shared;
    int ith;
};

// Compute plan
struct ggml_cplan {
    size_t    work_size; // size of work buffer, calculated by `ggml_graph_plan()`
    uint8_t * work_data; // work buffer, to be allocated by caller before calling to `ggml_graph_compute()`
    
    int n_threads;
    
    // abort ggml_graph_compute when true
    bool (*abort_callback)(void * data);
    void * abort_callback_data;
};

// Utility functions
static inline size_t ggml_up32(size_t n) {
    return (n + 31) & ~31;
}

static inline size_t ggml_up64(size_t n) {
    return (n + 63) & ~63;
}

static inline size_t ggml_up(size_t n, size_t m) {
    // assert m is a power of 2
    return (n + m - 1) & ~(m - 1);
}

// Type conversion helpers
uint16_t ggml_fp32_to_fp16(float x);
float    ggml_fp16_to_fp32(uint16_t h);

void ggml_fp16_to_fp32_row(const uint16_t * x, float * y, int n);
void ggml_fp32_to_fp16_row(const float * x, uint16_t * y, int n);

// Quantization functions
void quantize_row_q4_0_reference(const float * restrict x, block_q4_0 * restrict y, int k);
void quantize_row_q4_1_reference(const float * restrict x, block_q4_1 * restrict y, int k);
void quantize_row_q5_0_reference(const float * restrict x, block_q5_0 * restrict y, int k);
void quantize_row_q5_1_reference(const float * restrict x, block_q5_1 * restrict y, int k);
void quantize_row_q8_0_reference(const float * restrict x, block_q8_0 * restrict y, int k);
void quantize_row_q8_1_reference(const float * restrict x, block_q8_1 * restrict y, int k);

void dequantize_row_q4_0(const block_q4_0 * restrict x, float * restrict y, int k);
void dequantize_row_q4_1(const block_q4_1 * restrict x, float * restrict y, int k);
void dequantize_row_q5_0(const block_q5_0 * restrict x, float * restrict y, int k);
void dequantize_row_q5_1(const block_q5_1 * restrict x, float * restrict y, int k);
void dequantize_row_q8_0(const block_q8_0 * restrict x, float * restrict y, int k);
void dequantize_row_q8_1(const block_q8_1 * restrict x, float * restrict y, int k);

// Quantization block structures (mock definitions)
typedef struct {
    uint16_t d;          // delta
    uint8_t  qs[16];     // nibbles / quants
} block_q4_0;

typedef struct {
    uint16_t d;          // delta
    uint16_t m;          // min
    uint8_t  qs[16];     // nibbles / quants
} block_q4_1;

typedef struct {
    uint16_t d;          // delta
    uint8_t  qh[4];      // 5-th bit of quants
    uint8_t  qs[16];     // nibbles / quants
} block_q5_0;

typedef struct {
    uint16_t d;          // delta
    uint16_t m;          // min
    uint8_t  qh[4];      // 5-th bit of quants
    uint8_t  qs[16];     // nibbles / quants
} block_q5_1;

typedef struct {
    float    d;          // delta
    int8_t   qs[32];     // quants
} block_q8_0;

typedef struct {
    float    d;          // delta
    float    s;          // d * sum(qs[i]) / 256
    int8_t   qs[32];     // quants
} block_q8_1;

// Graph functions
struct ggml_cgraph * ggml_new_graph(struct ggml_context * ctx);
struct ggml_cgraph * ggml_new_graph_custom(struct ggml_context * ctx, size_t size, bool grads);
struct ggml_cgraph * ggml_graph_dup(struct ggml_context * ctx, struct ggml_cgraph * cgraph);
struct ggml_cgraph   ggml_graph_view(struct ggml_cgraph * cgraph, int i0, int i1);
void ggml_graph_cpy(struct ggml_cgraph * src, struct ggml_cgraph * dst);
void ggml_graph_reset(struct ggml_cgraph * cgraph);
void ggml_graph_clear(struct ggml_cgraph * cgraph);

size_t ggml_graph_overhead(void);
size_t ggml_graph_overhead_custom(size_t size, bool grads);

// Build operations
void ggml_build_forward_expand (struct ggml_cgraph * cgraph, struct ggml_tensor * tensor);
void ggml_build_backward_expand(struct ggml_context * ctx, struct ggml_cgraph * gf, struct ggml_cgraph * gb, bool keep);

// Compute functions
struct ggml_cplan ggml_graph_plan   (struct ggml_cgraph * cgraph, int n_threads);
int ggml_graph_compute(struct ggml_cgraph * cgraph, struct ggml_cplan * cplan);
void ggml_graph_compute_with_ctx(struct ggml_context * ctx, struct ggml_cgraph * cgraph, int n_threads);

// Performance
void ggml_graph_print   (const struct ggml_cgraph * cgraph);
void ggml_graph_dump_dot(const struct ggml_cgraph * gb, const struct ggml_cgraph * gf, const char * filename);

void ggml_set_zero(struct ggml_tensor * tensor);
void ggml_set_i32 (struct ggml_tensor * tensor, int32_t value);
void ggml_set_f32 (struct ggml_tensor * tensor, float value);

int32_t ggml_get_i32_1d(const struct ggml_tensor * tensor, int i);
void    ggml_set_i32_1d(const struct ggml_tensor * tensor, int i, int32_t value);

int32_t ggml_get_i32_nd(const struct ggml_tensor * tensor, int i0, int i1, int i2, int i3);
void    ggml_set_i32_nd(const struct ggml_tensor * tensor, int i0, int i1, int i2, int i3, int32_t value);

float   ggml_get_f32_1d(const struct ggml_tensor * tensor, int i);
void    ggml_set_f32_1d(const struct ggml_tensor * tensor, int i, float value);

float   ggml_get_f32_nd(const struct ggml_tensor * tensor, int i0, int i1, int i2, int i3);
void    ggml_set_f32_nd(const struct ggml_tensor * tensor, int i0, int i1, int i2, int i3, float value);

void *  ggml_get_data    (const struct ggml_tensor * tensor);
float * ggml_get_data_f32(const struct ggml_tensor * tensor);

const char * ggml_get_name(const struct ggml_tensor * tensor);
struct ggml_tensor * ggml_set_name(struct ggml_tensor * tensor, const char * name);

#ifdef __cplusplus
}
#endif

#endif // GGML_IMPL_H