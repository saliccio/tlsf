# TLSF
Implementation of Two-Level Segregated Fit, introduced in paper [TLSF: a New Dynamic Memory Allocator for Real-Time Systems](http://www.gii.upv.es/tlsf/files/papers/ecrts04_tlsf.pdf).

## Overview
TLSF is a dynamic memory allocator designed for real-time systems requiring high performance and predictability. It provides constant-time allocation and deallocation with low fragmentation, making it suitable for systems with stringent real-time constraints.

## Features
- **Constant-Time Operations**: TLSF guarantees O(1) complexity for both allocation and deallocation.

- **Low Fragmentation**: Efficiently manages memory to minimize fragmentation.

- **Real-Time Suitability**: Designed to meet the needs of real-time systems, providing predictable and consistent behavior.

- **Two-Level Segregate Fit**: Combines the benefits of segregated free lists and buddy systems to achieve its performance goals.

## Usage
```c
#include "tlsf.h"

// Initialize a memory pool
int result = tlsf_init(pool_size);

// Allocate memory
void *ptr = tlsf_malloc(size);

// Deallocate memory
tlsf_free(ptr);

// Destroy the memory pool
tlsf_teardown();
```
