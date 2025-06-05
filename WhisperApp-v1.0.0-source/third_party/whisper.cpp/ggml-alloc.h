#ifndef GGML_ALLOC_H
#define GGML_ALLOC_H

#include "ggml.h"

#ifdef __cplusplus
extern "C" {
#endif

// Memory allocator for ggml tensors

struct ggml_allocr;
typedef struct ggml_allocr * ggml_allocr_t;

// Initialize allocator for use with CPU backend
GGML_API ggml_allocr_t ggml_allocr_new(void * data, size_t size, size_t alignment);
GGML_API ggml_allocr_t ggml_allocr_new_measure(size_t alignment);

// Free allocator
GGML_API void ggml_allocr_free(ggml_allocr_t alloc);

// Check if allocator is measure only
GGML_API bool ggml_allocr_is_measure(ggml_allocr_t alloc);

// Reset allocator
GGML_API void ggml_allocr_reset(ggml_allocr_t alloc);

// Allocate tensor
GGML_API void ggml_allocr_alloc(ggml_allocr_t alloc, struct ggml_tensor * tensor);

// Get size of allocator
GGML_API size_t ggml_allocr_get_alloc_size(ggml_allocr_t alloc);

// Allocate graph
GGML_API size_t ggml_allocr_alloc_graph(ggml_allocr_t alloc, struct ggml_cgraph * graph);

// Backend allocator
struct ggml_backend_buffer;
struct ggml_backend;

GGML_API ggml_allocr_t ggml_allocr_new_from_buffer(struct ggml_backend_buffer * buffer);
GGML_API ggml_allocr_t ggml_allocr_new_from_backend(struct ggml_backend * backend, size_t size);
GGML_API ggml_allocr_t ggml_allocr_new_measure_from_backend(struct ggml_backend * backend);

GGML_API struct ggml_backend_buffer * ggml_allocr_get_buffer(ggml_allocr_t alloc);

// Legacy API
GGML_API void ggml_allocr_set_parse_seq(ggml_allocr_t alloc, const int * list, int n);

#ifdef __cplusplus
}
#endif

#endif // GGML_ALLOC_H