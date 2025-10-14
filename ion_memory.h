/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */
/*
 * Copyright 2023 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef ION_MEMORY_H
#define ION_MEMORY_H

#include "memory.h"
#include "config.h"


struct perftest_parameters;

bool neuron_memory_supported();

bool neuron_memory_dmabuf_supported();

struct memory_ctx *ion_memory_create(struct perftest_parameters *params);