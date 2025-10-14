/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2023 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "ion.h"
#include "perftest_parameters.h"

#define ACCEL_PAGE_SIZE (4 * 1024)


struct ion_memory_ctx {
        struct memory_ctx base;
        bool use_dmabuf;
        int fd;
};


int ion_memory_init(struct memory_ctx *ctx) {
        int fd;
        struct ion_memory_ctx *ion_ctx = container_of(ctx, struct ion_memory_ctx, base);
        fd = open("/dev/ion", O_RDWR);
        if (fd < 0) {
                printf("open /dev/ion failed, %s\n", strerror(errno));
                return FAILURE;
        }
        ion_ctx->fd = fd;
        return SUCCESS;
}

int ion_memory_destroy(struct memory_ctx *ctx) {
        struct ion_memory_ctx *ion_ctx = container_of(ctx, struct ion_memory_ctx, base);
        if(ion_ctx->fd)
                close(ion_ctx->fd);
        return SUCCESS;
}

int ion_memory_allocate_buffer(struct memory_ctx *ctx, int alignment, uint64_t size, int *dmabuf_fd,
                                  uint64_t *dmabuf_offset, void **addr, bool *can_init) {
        struct ion_memory_ctx *ion_ctx = container_of(ctx, struct ion_memory_ctx, base);
        struct ion_allocation_data alloc_data;
        char *str;

        size_t buf_size = (size + (ACCEL_PAGE_SIZE - 1)) & ~(ACCEL_PAGE_SIZE - 1);

        alloc_data.len = buf_size;

        if (ioctl(ion_ctx->fd, ION_IOC_ALLOC, &alloc_data)) {
                printf("ion ioctl failed, %s\n", strerror(errno));
                close(ion_ctx->fd);
                return FAILURE;
        }

        printf("ion alloc success: size = %lu, dmabuf_fd = %u\n",
                alloc_data.len, alloc_data.fd);

        str = mmap(NULL, alloc_data.len , PROT_READ | PROT_WRITE, MAP_SHARED, alloc_data.fd, 0);
        if (str == MAP_FAILED) {
                printf("mmap dmabuf failed: %s\n", strerror(errno));
                return FAILURE;
        }


        *dmabuf_fd = alloc_data.fd;
        *dmabuf_offset = 0;
        *addr = str;
        *can_init = false;
#if 0
        void *d_A = NULL;
        NRT_STATUS result;
        size_t buf_size = (size + ACCEL_PAGE_SIZE - 1) & ~(ACCEL_PAGE_SIZE - 1);
        int tensor_index = ion_ctx->num_of_tensors;

        if (tensor_index >= ion_ctx->max_tensors)
        {
                printf("Can't allocate Neuron memory, max tensors reached\n");
                return FAILURE;
        }

        result = nrt_tensor_allocate(NRT_TENSOR_PLACEMENT_DEVICE, ion_ctx->core_id, buf_size, NULL, &ion_ctx->tensors[tensor_index]);
        if (result != NRT_SUCCESS) {
                ion_ctx->tensors[tensor_index] = NULL;
                printf("nrt_tensor_allocate_error =%d\n", (int)result);
                return FAILURE;
        }

        d_A = nrt_tensor_get_va(ion_ctx->tensors[tensor_index]);
        if (d_A == NULL) {
                nrt_tensor_free(&ion_ctx->tensors[tensor_index]);
                ion_ctx->tensors[tensor_index] = NULL;
                printf("Failed to get va for the allocated tensor\n");
                return FAILURE;
        }

        if (ion_ctx->use_dmabuf) {
                result = nrt_get_dmabuf_fd((uint64_t)d_A, (uint64_t)buf_size, dmabuf_fd);
                if (result != NRT_SUCCESS) {
                        nrt_tensor_free(&ion_ctx->tensors[tensor_index]);
                        ion_ctx->tensors[tensor_index] = NULL;
                        *dmabuf_fd = 0;
                        printf("Unable to retrieve dmabuf fd of Neuron device buffer\n");
                        return FAILURE;
                }

                *dmabuf_offset = 0;
        }

        ion_ctx->num_of_tensors++;
        *addr = d_A;
        *can_init = false;
#endif
        return SUCCESS;
}

int ion_memory_free_buffer(struct memory_ctx *ctx, int dmabuf_fd, void *addr, uint64_t size) {
        return SUCCESS;
}

bool ion_memory_supported() {
        return true;
}

bool ion_memory_dmabuf_supported() {
        return true;
}

struct memory_ctx *ion_memory_create(struct perftest_parameters *params) {
        struct ion_memory_ctx *ctx;

        printf("inside ion_memory_create\n");
        ALLOCATE(ctx, struct ion_memory_ctx, 1);
        ctx->base.init = ion_memory_init;
        ctx->base.destroy = ion_memory_destroy;
        ctx->base.allocate_buffer = ion_memory_allocate_buffer;
        ctx->base.free_buffer = ion_memory_free_buffer;
        ctx->base.copy_host_to_buffer = memcpy;
        ctx->base.copy_buffer_to_host = memcpy;
        ctx->base.copy_buffer_to_buffer = memcpy;
        ctx->use_dmabuf = params->use_ion_dmabuf;
        return &ctx->base;
}