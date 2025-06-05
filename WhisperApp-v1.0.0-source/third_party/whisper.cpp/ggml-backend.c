#include "ggml-backend.h"
#include "ggml-impl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Helper function to calculate tensor size
size_t ggml_nbytes(const struct ggml_tensor * tensor) {
    if (!tensor) return 0;
    
    size_t nbytes = ggml_type_size(tensor->type);
    for (int i = 0; i < 4; i++) {
        nbytes *= tensor->ne[i];
    }
    return nbytes;
}

size_t ggml_nbytes_pad(const struct ggml_tensor * tensor) {
    return (ggml_nbytes(tensor) + 31) & ~31; // 32-byte alignment
}

// CPU backend buffer implementation
static void cpu_buffer_free(ggml_backend_buffer_t buffer) {
    if (buffer->context) {
        free(buffer->context);
    }
    free(buffer);
}

static void * cpu_buffer_get_base(ggml_backend_buffer_t buffer) {
    return buffer->context;
}

static void cpu_buffer_init_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor) {
    (void)buffer;
    tensor->backend = GGML_BACKEND_CPU;
    tensor->buffer = buffer;
    tensor->data = (char *)buffer->context + tensor->view_offs;
}

static void cpu_buffer_set_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    memcpy((char *)buffer->context + tensor->view_offs + offset, data, size);
}

static void cpu_buffer_get_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    memcpy(data, (const char *)buffer->context + tensor->view_offs + offset, size);
}

static bool cpu_buffer_cpy_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * src, struct ggml_tensor * dst) {
    if (src->backend == GGML_BACKEND_CPU && dst->backend == GGML_BACKEND_CPU) {
        memcpy(dst->data, src->data, ggml_nbytes(src));
        return true;
    }
    return false;
}

static void cpu_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value) {
    memset(buffer->context, value, buffer->size);
}

static void cpu_buffer_reset(ggml_backend_buffer_t buffer) {
    cpu_buffer_clear(buffer, 0);
}

static struct ggml_backend_buffer_i cpu_backend_buffer_i = {
    .free_buffer = cpu_buffer_free,
    .get_base = cpu_buffer_get_base,
    .init_tensor = cpu_buffer_init_tensor,
    .set_tensor = cpu_buffer_set_tensor,
    .get_tensor = cpu_buffer_get_tensor,
    .cpy_tensor = cpu_buffer_cpy_tensor,
    .clear = cpu_buffer_clear,
    .reset = cpu_buffer_reset,
};

// CPU backend buffer type implementation
static ggml_backend_buffer_t cpu_alloc_buffer(ggml_backend_buffer_type_t buft, size_t size) {
    (void)buft;
    
    void * data = malloc(size);
    if (!data) return NULL;
    
    ggml_backend_buffer_t buffer = malloc(sizeof(struct ggml_backend_buffer));
    if (!buffer) {
        free(data);
        return NULL;
    }
    
    buffer->iface = cpu_backend_buffer_i;
    buffer->buft = buft;
    buffer->context = data;
    buffer->size = size;
    buffer->usage = GGML_BACKEND_BUFFER_USAGE_ANY;
    
    return buffer;
}

static size_t cpu_get_alignment(ggml_backend_buffer_type_t buft) {
    (void)buft;
    return 32; // 32-byte alignment for CPU
}

static size_t cpu_get_max_size(ggml_backend_buffer_type_t buft) {
    (void)buft;
    return SIZE_MAX;
}

static size_t cpu_get_alloc_size(ggml_backend_buffer_type_t buft, struct ggml_tensor * tensor) {
    (void)buft;
    return ggml_nbytes_pad(tensor);
}

static bool cpu_supports_backend(ggml_backend_buffer_type_t buft, ggml_backend_t backend) {
    (void)buft;
    return ggml_backend_is_cpu(backend);
}

static bool cpu_is_host(ggml_backend_buffer_type_t buft) {
    (void)buft;
    return true;
}

static struct ggml_backend_buffer_type_i cpu_backend_buffer_type_i = {
    .alloc_buffer = cpu_alloc_buffer,
    .get_alignment = cpu_get_alignment,
    .get_max_size = cpu_get_max_size,
    .get_alloc_size = cpu_get_alloc_size,
    .supports_backend = cpu_supports_backend,
    .is_host = cpu_is_host,
};

static struct ggml_backend_buffer_type cpu_backend_buffer_type = {
    .iface = cpu_backend_buffer_type_i,
    .context = NULL,
};

// CPU backend implementation
static const char * cpu_backend_name(ggml_backend_t backend) {
    (void)backend;
    return "CPU";
}

static void cpu_backend_free(ggml_backend_t backend) {
    free(backend);
}

static ggml_backend_buffer_type_t cpu_backend_get_default_buffer_type(ggml_backend_t backend) {
    (void)backend;
    return &cpu_backend_buffer_type;
}

static void cpu_backend_set_tensor_async(ggml_backend_t backend, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    (void)backend;
    memcpy((char *)tensor->data + offset, data, size);
}

static void cpu_backend_get_tensor_async(ggml_backend_t backend, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    (void)backend;
    memcpy(data, (const char *)tensor->data + offset, size);
}

static bool cpu_backend_cpy_tensor_async(ggml_backend_t backend, const struct ggml_tensor * src, struct ggml_tensor * dst) {
    (void)backend;
    memcpy(dst->data, src->data, ggml_nbytes(src));
    return true;
}

static void cpu_backend_synchronize(ggml_backend_t backend) {
    (void)backend;
    // CPU is always synchronized
}

static bool cpu_backend_graph_plan_compute(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    (void)backend;
    (void)cgraph;
    return true; // CPU can compute any graph
}

static bool cpu_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    (void)backend;
    
    for (int i = 0; i < cgraph->n_nodes; i++) {
        struct ggml_tensor * node = cgraph->nodes[i];
        ggml_compute_forward(node, NULL);
    }
    
    return true;
}

static bool cpu_backend_supports_op(ggml_backend_t backend, const struct ggml_tensor * op) {
    (void)backend;
    (void)op;
    return true; // CPU supports all operations
}

static struct ggml_backend_i cpu_backend_i = {
    .get_name = cpu_backend_name,
    .free = cpu_backend_free,
    .get_default_buffer_type = cpu_backend_get_default_buffer_type,
    .set_tensor_async = cpu_backend_set_tensor_async,
    .get_tensor_async = cpu_backend_get_tensor_async,
    .cpy_tensor_async = cpu_backend_cpy_tensor_async,
    .synchronize = cpu_backend_synchronize,
    .graph_plan_compute = cpu_backend_graph_plan_compute,
    .graph_compute = cpu_backend_graph_compute,
    .supports_op = cpu_backend_supports_op,
};

// Public API implementations

// Buffer functions
void ggml_backend_buffer_free(ggml_backend_buffer_t buffer) {
    if (buffer && buffer->iface.free_buffer) {
        buffer->iface.free_buffer(buffer);
    }
}

void * ggml_backend_buffer_get_base(ggml_backend_buffer_t buffer) {
    return buffer ? buffer->iface.get_base(buffer) : NULL;
}

size_t ggml_backend_buffer_get_size(ggml_backend_buffer_t buffer) {
    return buffer ? buffer->size : 0;
}

size_t ggml_backend_buffer_get_alignment(ggml_backend_buffer_t buffer) {
    return buffer ? cpu_get_alignment(buffer->buft) : 0;
}

void ggml_backend_buffer_init_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor) {
    if (buffer && buffer->iface.init_tensor) {
        buffer->iface.init_tensor(buffer, tensor);
    }
}

void ggml_backend_buffer_set_tensor(ggml_backend_buffer_t buffer, struct ggml_tensor * tensor, const void * data, size_t offset, size_t size) {
    if (buffer && buffer->iface.set_tensor) {
        buffer->iface.set_tensor(buffer, tensor, data, offset, size);
    }
}

void ggml_backend_buffer_get_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * tensor, void * data, size_t offset, size_t size) {
    if (buffer && buffer->iface.get_tensor) {
        buffer->iface.get_tensor(buffer, tensor, data, offset, size);
    }
}

bool ggml_backend_buffer_cpy_tensor(ggml_backend_buffer_t buffer, const struct ggml_tensor * src, struct ggml_tensor * dst) {
    return buffer && buffer->iface.cpy_tensor ? buffer->iface.cpy_tensor(buffer, src, dst) : false;
}

void ggml_backend_buffer_clear(ggml_backend_buffer_t buffer, uint8_t value) {
    if (buffer && buffer->iface.clear) {
        buffer->iface.clear(buffer, value);
    }
}

bool ggml_backend_buffer_is_host(ggml_backend_buffer_t buffer) {
    return buffer ? cpu_is_host(buffer->buft) : false;
}

void ggml_backend_buffer_set_usage(ggml_backend_buffer_t buffer, enum ggml_backend_buffer_usage usage) {
    if (buffer) {
        buffer->usage = usage;
    }
}

// Backend functions
const char * ggml_backend_name(ggml_backend_t backend) {
    return backend ? backend->iface.get_name(backend) : NULL;
}

void ggml_backend_free(ggml_backend_t backend) {
    if (backend && backend->iface.free) {
        backend->iface.free(backend);
    }
}

ggml_backend_buffer_type_t ggml_backend_get_default_buffer_type(ggml_backend_t backend) {
    return backend ? backend->iface.get_default_buffer_type(backend) : NULL;
}

ggml_backend_buffer_t ggml_backend_alloc_buffer(ggml_backend_t backend, size_t size) {
    if (!backend) return NULL;
    ggml_backend_buffer_type_t buft = ggml_backend_get_default_buffer_type(backend);
    return buft ? cpu_alloc_buffer(buft, size) : NULL;
}

size_t ggml_backend_get_alignment(ggml_backend_t backend) {
    if (!backend) return 0;
    ggml_backend_buffer_type_t buft = ggml_backend_get_default_buffer_type(backend);
    return buft ? cpu_get_alignment(buft) : 0;
}

size_t ggml_backend_get_max_size(ggml_backend_t backend) {
    if (!backend) return 0;
    ggml_backend_buffer_type_t buft = ggml_backend_get_default_buffer_type(backend);
    return buft ? cpu_get_max_size(buft) : 0;
}

bool ggml_backend_graph_compute(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    return backend && backend->iface.graph_compute ? backend->iface.graph_compute(backend, cgraph) : false;
}

bool ggml_backend_supports_op(ggml_backend_t backend, const struct ggml_tensor * op) {
    return backend && backend->iface.supports_op ? backend->iface.supports_op(backend, op) : false;
}

// CPU backend specific functions
ggml_backend_t ggml_backend_cpu_init(void) {
    ggml_backend_t backend = malloc(sizeof(struct ggml_backend));
    if (!backend) return NULL;
    
    backend->iface = cpu_backend_i;
    backend->context = NULL;
    
    return backend;
}

bool ggml_backend_is_cpu(ggml_backend_t backend) {
    return backend && backend->iface.get_name == cpu_backend_name;
}

void ggml_backend_cpu_set_n_threads(ggml_backend_t backend_cpu, int n_threads) {
    (void)backend_cpu;
    (void)n_threads;
    // Not implemented in this minimal version
}

ggml_backend_buffer_type_t ggml_backend_cpu_buffer_type(void) {
    return &cpu_backend_buffer_type;
}

ggml_backend_buffer_t ggml_backend_cpu_buffer_from_ptr(void * ptr, size_t size) {
    ggml_backend_buffer_t buffer = malloc(sizeof(struct ggml_backend_buffer));
    if (!buffer) return NULL;
    
    buffer->iface = cpu_backend_buffer_i;
    buffer->buft = &cpu_backend_buffer_type;
    buffer->context = ptr;
    buffer->size = size;
    buffer->usage = GGML_BACKEND_BUFFER_USAGE_ANY;
    
    return buffer;
}

// Stub implementations for optional backends
bool ggml_backend_is_metal(ggml_backend_t backend) { (void)backend; return false; }
void ggml_backend_metal_set_n_cb(ggml_backend_t backend, int n_cb) { (void)backend; (void)n_cb; }
bool ggml_backend_is_cuda(ggml_backend_t backend) { (void)backend; return false; }
int ggml_backend_cuda_get_device_count(void) { return 0; }
void ggml_backend_cuda_get_device_description(int device, char * description, size_t description_size) { (void)device; (void)description; (void)description_size; }
void ggml_backend_cuda_get_device_memory(int device, size_t * free, size_t * total) { (void)device; (void)free; (void)total; }

// Registry stubs
ggml_backend_t ggml_backend_reg_get_backend(size_t i) { (void)i; return NULL; }
char * ggml_backend_reg_get_name(size_t i) { (void)i; return NULL; }
size_t ggml_backend_reg_get_count(void) { return 0; }
ggml_backend_t ggml_backend_reg_get_backend_from_str(const char * name) { (void)name; return NULL; }
ggml_backend_buffer_type_t ggml_backend_reg_get_buffer_type(size_t i) { (void)i; return NULL; }
char * ggml_backend_reg_get_buffer_type_name(size_t i) { (void)i; return NULL; }
size_t ggml_backend_reg_get_buffer_type_count(void) { return 0; }
ggml_backend_buffer_type_t ggml_backend_reg_get_buffer_type_from_str(const char * name) { (void)name; return NULL; }

// Scheduling stubs
ggml_backend_sched_t ggml_backend_sched_new(ggml_backend_t * backends, ggml_backend_buffer_type_t * bufts, int n_backends, size_t graph_size) { (void)backends; (void)bufts; (void)n_backends; (void)graph_size; return NULL; }
void ggml_backend_sched_free(ggml_backend_sched_t sched) { (void)sched; }
bool ggml_backend_sched_reserve(ggml_backend_sched_t sched, struct ggml_cgraph * measure_graph) { (void)sched; (void)measure_graph; return false; }
bool ggml_backend_sched_alloc_graph(ggml_backend_sched_t sched, struct ggml_cgraph * graph) { (void)sched; (void)graph; return false; }
bool ggml_backend_sched_compute_graph(ggml_backend_sched_t sched, struct ggml_cgraph * graph) { (void)sched; (void)graph; return false; }
void ggml_backend_sched_reset(ggml_backend_sched_t sched) { (void)sched; }
void ggml_backend_sched_set_eval_callback(ggml_backend_sched_t sched, ggml_backend_sched_eval_callback callback, void * user_data) { (void)sched; (void)callback; (void)user_data; }