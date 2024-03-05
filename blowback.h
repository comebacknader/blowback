#pragma once
typedef struct GameMemory {
    b32 is_initialized;
    u64 permanent_storage_size;
    void *permanent_storage;
} GameMemory;