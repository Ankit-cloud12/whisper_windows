#include "ggml-alloc.h"
#include "ggml-backend.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Allocator structure
struct ggml_allocr {
    struct ggml_backend_buffer * buffer;
    bool buffer_owned;
    void * data;
    size_t size;
    size_t alignment;
    size_t offset;
    bool measure;
    struct ggml_hash_set hash_set;
    size_t max_size;
};

// Hash set for allocated tensors
struct ggml_hash_set {
    size_t size;
    struct ggml_tensor ** keys;
};

// Alignment helper
static size_t aligned_offset(size_t offset, size_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

// Initialize allocator
ggml_allocr_t ggml_allocr_new(void * data, size_t size, size_t alignment) {
    ggml_allocr_t alloc = (ggml_allocr_t)malloc(sizeof(struct ggml_allocr));
    if (!alloc) return NULL;
    
    *alloc = (struct ggml_allocr) {
        .buffer = NULL,
        .buffer_owned = false,
        .data = data,
        .size = size,
        .alignment = alignment,
        .offset = 0,
        .measure = false,
        .hash_set = {0},
        .max_size = 0,
    };
    
    return alloc;
}

ggml_allocr_t ggml_allocr_new_measure(size_t alignment) {
    ggml_allocr_t alloc = (ggml_allocr_t)malloc(sizeof(struct ggml_allocr));
    if (!alloc) return NULL;
    
    *alloc = (struct ggml_allocr) {
        .buffer = NULL,
        .buffer_owned = false,
        .data = NULL,
        .size = SIZE_MAX,
        .alignment = alignment,
        .offset = 0,
        .measure = true,
        .hash_set = {0},
        .max_size = 0,
    };
    
    return alloc;
}

void ggml_allocr_free(ggml_allocr_t alloc) {
    if (!alloc) return;
    
    if (alloc->buffer_owned && alloc->buffer) {
        ggml_backend_buffer_free(alloc->buffer);
    }
    
    if (alloc->hash_set.keys) {
        free(alloc->hash_set.keys);
    }
    
    free(alloc);
}

bool ggml_allocr_is_measure(ggml_allocr_t alloc) {
    return alloc ? alloc->measure : false;
}

void ggml_allocr_reset(ggml_allocr_t alloc) {
    if (!alloc) return;
    
    alloc->offset = 0;
    if (alloc->measure) {
        alloc->max_size = 0;
    }
}

void ggml_allocr_alloc(ggml_allocr_t alloc, struct ggml_tensor * tensor) {
    if (!alloc || !tensor) return;
    
    size_t size = ggml_nbytes(tensor);
    size_t offset = aligned_offset(alloc->offset, alloc->alignment);
    
    if (alloc->measure) {
        alloc->offset = offset + size;
        if (alloc->offset > alloc->max_size) {
            alloc->max_size = alloc->offset;
        }
        tensor->data = NULL; // measure mode doesn't allocate actual memory
    } else {
        if (offset + size > alloc->size) {
            // Out of memory
            tensor->data = NULL;
            return;
        }
        
        tensor->data = (char *)alloc->data + offset;
        alloc->offset = offset + size;
    }
}

size_t ggml_allocr_get_alloc_size(ggml_allocr_t alloc) {
    return alloc ? (alloc->measure ? alloc->max_size : alloc->offset) : 0;
}

size_t ggml_allocr_alloc_graph(ggml_allocr_t alloc, struct ggml_cgraph * graph) {
    if (!alloc || !graph) return 0;
    
    // Allocate all tensors in the graph
    for (int i = 0; i < graph->n_nodes; i++) {
        ggml_allocr_alloc(alloc, graph->nodes[i]);
    }
    
    for (int i = 0; i < graph->n_leafs; i++) {
        ggml_allocr_alloc(alloc, graph->leafs[i]);
    }
    
    return ggml_allocr_get_alloc_size(alloc);
}

// Backend allocator functions (stubs for now)
ggml_allocr_t ggml_allocr_new_from_buffer(struct ggml_backend_buffer * buffer) {
    if (!buffer) return NULL;
    
    ggml_allocr_t alloc = (ggml_allocr_t)malloc(sizeof(struct ggml_allocr));
    if (!alloc) return NULL;
    
    *alloc = (struct ggml_allocr) {
        .buffer = buffer,
        .buffer_owned = false,
        .data = ggml_backend_buffer_get_base(buffer),
        .size = ggml_backend_buffer_get_size(buffer),
        .alignment = ggml_backend_buffer_get_alignment(buffer),
        .offset = 0,
        .measure = false,
        .hash_set = {0},
        .max_size = 0,
    };
    
    return alloc;
}

ggml_allocr_t ggml_allocr_new_from_backend(struct ggml_backend * backend, size_t size) {
    if (!backend) return NULL;
    
    struct ggml_backend_buffer * buffer = ggml_backend_alloc_buffer(backend, size);
    if (!buffer) return NULL;
    
    ggml_allocr_t alloc = ggml_allocr_new_from_buffer(buffer);
    if (alloc) {
        alloc->buffer_owned = true;
    }
    
    return alloc;
}

ggml_allocr_t ggml_allocr_new_measure_from_backend(struct ggml_backend * backend) {
    if (!backend) return NULL;
    
    size_t alignment = ggml_backend_get_alignment(backend);
    return ggml_allocr_new_measure(alignment);
}

struct ggml_backend_buffer * ggml_allocr_get_buffer(ggml_allocr_t alloc) {
    return alloc ? alloc->buffer : NULL;
}

// Legacy API
void ggml_allocr_set_parse_seq(ggml_allocr_t alloc, const int * list, int n) {
    (void)alloc;
    (void)list;
    (void)n;
    // Legacy function - not implemented in this minimal version
}