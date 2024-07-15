#pragma once

internal void game_update_and_render();

typedef struct GameMemory 
{
    b32 is_initialized;
    u64 permanent_storage_size;
    void *permanent_storage;
    u64 transient_storage_size;
    void *transient_storage;
} GameMemory;

typedef struct GameState 
{
    u32 shader_program;

    v3 camera_position;
    v3 camera_front;
    v3 up;

    v3 camera_target;
    v3 camera_direction;
    v3 camera_right;
    v3 camera_up;

    f32 movement_x;
    f32 movement_y;

    u32 old_time;
    u32 new_time;
    u32 dt;
    f32 fps;

    f32 position_x;
    f32 position_y;

    f32 window_width;
    f32 window_height;
} GameState;