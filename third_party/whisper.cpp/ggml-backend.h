#ifndef GGML_BACKEND_H
#define GGML_BACKEND_H

#include "ggml.h"
#include "ggml-alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

// Backend buffer
struct ggml_backend_buffer;
typedef struct ggml_backend_buffer * ggml_backend_buffer_t;

// Backend
struct ggml_backend;
typedef struct ggml_backend * ggml_backend_t;

// Backend buffer interface
typedef struct ggml_backend_buffer_i {
    void (*free_buffer)(ggml_backend_buffer_t buffer);
    void * (*get_base)(ggml_backend_buffer_t buffer);
    void (*init_tensor)(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor);
    void (*set_tensor)(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size);
    void (*get_tensor)(ggml_backend_buffer_t buffer, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size);
    bool (*cpy_tensor)(ggml_backend_buffer_t buffer, const struct ggml_tensor * src, struct ggml_tensor * dst);
    void (*clear)(ggml_backend_buffer_t buffer, uint8_t value);
    void (*reset)(ggml_backend_buffer_t buffer);
} ggml_backend_buffer_i;

// Backend buffer type
typedef struct ggml_backend_buffer_type_i {
    ggml_backend_buffer_t (*alloc_buffer)(ggml_backend_buffer_type_t buft, size_t size);
    size_t (*get_alignment)(ggml_backend_buffer_type_t buft);
    size_t (*get_max_size)(ggml_backend_buffer_type_t buft);
    size_t (*get_alloc_size)(ggml_backend_buffer_type_t buft, struct ggml_tensor * tensor);
    bool (*supports_backend)(ggml_backend_buffer_type_t buft, ggml_backend_t backend);
    bool (*is_host)(ggml_backend_buffer_type_t buft);
} ggml_backend_buffer_type_i;

typedef struct ggml_backend_buffer_type {
    struct ggml_backend_buffer_type_i iface;
    void * context;
} ggml_backend_buffer_type;

typedef ggml_backend_buffer_type * ggml_backend_buffer_type_t;

// Backend buffer
struct ggml_backend_buffer {
    struct ggml_backend_buffer_i iface;
    ggml_backend_buffer_type_t buft;
    void * context;
    size_t size;
    enum ggml_backend_buffer_usage usage;
};

enum ggml_backend_buffer_usage {
    GGML_BACKEND_BUFFER_USAGE_ANY = 0,
    GGML_BACKEND_BUFFER_USAGE_WEIGHTS = 1,
    GGML_BACKEND_BUFFER_USAGE_COMPUTE = 2,
};

// Backend interface
typedef struct ggml_backend_i {
    const char * (*get_name)(ggml_backend_t backend);
    
    void (*free)(ggml_backend_t backend);
    
    ggml_backend_buffer_type_t (*get_default_buffer_type)(ggml_backend_t backend);
    
    void (*set_tensor_async)(ggml_backend_t backend, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size);
    void (*get_tensor_async)(ggml_backend_t backend, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size);
    bool (*cpy_tensor_async)(ggml_backend_t backend, const struct ggml_tensor * src, struct ggml_tensor * dst);
    
    void (*synchronize)(ggml_backend_t backend);
    
    bool (*graph_plan_compute)(ggml_backend_t backend, struct ggml_cgraph * cgraph);
    bool (*graph_compute)(ggml_backend_t backend, struct ggml_cgraph * cgraph);
    bool (*supports_op)(ggml_backend_t backend, const struct ggml_tensor * op);
} ggml_backend_i;

struct ggml_backend {
    struct ggml_backend_i iface;
    void * context;
};

// Backend buffer functions
GGML_API void ggml_backend_buffer_free(ggml_backend_buffer_t buffer);
GGML_API void * ggml_backend_buffer_get_base(ggml_backend_buffer_t buffer);
GGML_API size_t ggml_backend_buffer_get_size(ggml_backend_buffer_t buffer);
GGML_API size_t ggml_backend_buffer_get_alignment(ggml_backend_buffer_t buffer);
GGML_API void ggml_backend_buffer_init_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor);
GGML_API void ggml_backend_buffer_set_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size);
GGML_API void ggml_backend_buffer_get_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size);
GGML_API bool ggml_backend_buffer_cpy_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * src, struct ggml_tensor * dst);
GGML_API void ggml_backend_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value);
GGML_API bool ggml_backend_buffer_is_host(ggml_backend_buffer_t buffer);
GGML_API void ggml_backend_buffer_set_usage(ggml_backend_buffer_t buffer, enum ggml_backend_buffer_usage usage);

// Backend buffer type functions
GGML_API ggml_backend_buffer_t ggml_backend_buft_alloc_buffer(ggml_backend_buffer_type_t buft, size_t size);
GGML_API size_t ggml_backend_buft_get_alignment(ggml_backend_buffer_type_t buft);
GGML_API size_t ggml_backend_buft_get_max_size(ggml_backend_buffer_type_t buft);
GGML_API size_t ggml_backend_buft_get_alloc_size(ggml_backend_buffer_type_t buft, struct ggml_tensor * tensor);
GGML_API bool ggml_backend_buft_supports_backend(ggml_backend_buffer_type_t buft, ggml_backend_t backend);
GGML_API bool ggml_backend_buft_is_host(ggml_backend_buffer_type_t buft);

// Backend functions
GGML_API const char * ggml_backend_name(ggml_backend_t backend);
GGML_API void ggml_backend_free(ggml_backend_t backend);
GGML_API ggml_backend_buffer_type_t ggml_backend_get_default_buffer_type(ggml_backend_t backend);
GGML_API ggml_backend_buffer_t ggml_backend_alloc_buffer(ggml_backend_t backend, size_t size);
GGML_API size_t ggml_backend_get_alignment(ggml_backend_t backend);
GGML_API size_t ggml_backend_get_max_size(ggml_backend_t backend);

GGML_API void ggml_backend_set_tensor_async(ggml_backend_t backend, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size);
GGML_API void ggml_backend_get_tensor_async(ggml_backend_t backend, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size);
GGML_API bool ggml_backend_cpy_tensor_async(ggml_backend_t backend, const struct ggml_tensor * src, struct ggml_tensor * dst);

GGML_API void ggml_backend_synchronize(ggml_backend_t backend);

GGML_API bool ggml_backend_graph_plan_compute(ggml_backend_t backend, struct ggml_cgraph * cgraph);
GGML_API bool ggml_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph * cgraph);
GGML_API bool ggml_backend_supports_op(ggml_backend_t backend, const struct ggml_tensor * op);

// CPU backend
GGML_API ggml_backend_t ggml_backend_cpu_init(void);
GGML_API bool ggml_backend_is_cpu(ggml_backend_t backend);
GGML_API void ggml_backend_cpu_set_n_threads(ggml_backend_t backend_cpu, int n_threads);

GGML_API ggml_backend_buffer_type_t ggml_backend_cpu_buffer_type(void);
GGML_API ggml_backend_buffer_t ggml_backend_cpu_buffer_from_ptr(void * ptr, size_t size);

// Registry
GGML_API ggml_backend_t ggml_backend_reg_get_backend(size_t i);
GGML_API char * ggml_backend_reg_get_name(size_t i);
GGML_API size_t ggml_backend_reg_get_count(void);
GGML_API ggml_backend_t ggml_backend_reg_get_backend_from_str(const char * name);
GGML_API ggml_backend_buffer_type_t ggml_backend_reg_get_buffer_type(size_t i);
GGML_API char * ggml_backend_reg_get_buffer_type_name(size_t i);
GGML_API size_t ggml_backend_reg_get_buffer_type_count(void);
GGML_API ggml_backend_buffer_type_t ggml_backend_reg_get_buffer_type_from_str(const char * name);

// Backend scheduling
struct ggml_backend_sched;
typedef struct ggml_backend_sched * ggml_backend_sched_t;

typedef bool (*ggml_backend_sched_eval_callback)(struct ggml_tensor * t, bool ask, void * user_data);

GGML_API ggml_backend_sched_t ggml_backend_sched_new(ggml_backend_t * backends, ggml_backend_buffer_type_t * bufts, int n_backends, size_t graph_size);
GGML_API void ggml_backend_sched_free(ggml_backend_sched_t sched);
GGML_API bool ggml_backend_sched_reserve(ggml_backend_sched_t sched, struct ggml_cgraph * measure_graph);
GGML_API bool ggml_backend_sched_alloc_graph(ggml_backend_sched_t sched, struct ggml_cgraph * graph);
GGML_API bool ggml_backend_sched_compute_graph(ggml_backend_sched_t sched, struct ggml_cgraph * graph);
GGML_API void ggml_backend_sched_reset(ggml_backend_sched_t sched);
GGML_API void ggml_backend_sched_set_eval_callback(ggml_backend_sched_t sched, ggml_backend_sched_eval_callback callback, void * user_data);

// Utility functions
GGML_API bool ggml_backend_is_metal(ggml_backend_t backend);
GGML_API void ggml_backend_metal_set_n_cb(ggml_backend_t backend, int n_cb);

GGML_API bool ggml_backend_is_cuda(ggml_backend_t backend);
GGML_API int ggml_backend_cuda_get_device_count(void);
GGML_API void ggml_backend_cuda_get_device_description(int device, char * description, size_t description_size);
GGML_API void ggml_backend_cuda_get_device_memory(int device, size_t * free, size_t * total);

// Helper functions
GGML_API size_t ggml_nbytes(const struct ggml_tensor * tensor);
GGML_API size_t ggml_nbytes_pad(const struct ggml_tensor * tensor);

#ifdef __cplusplus
}
#endif

#endif // GGML_BACKEND_H