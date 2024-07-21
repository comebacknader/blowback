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

/*

NOTE(Nader): half_transition refers to a press down or a release up. 
We are recording the half_transition_count, the # of half transitions
for a current frame. I believe we're going to count two half_transitions 
per frame. 

ended_down lets us know if the button is in the ended down state.

*/
typedef struct GameButtonState
{
	int half_transition_count;
	b32 ended_down;
} GameButtonState;

typedef struct GameControllerInput
{
	b32 is_connected;
	b32 is_analog;
	f32 stick_average_x;
	f32 stick_average_y;

	f32 start_x;
	f32 start_y;

	f32 min_x;
	f32 min_y;

	f32 max_x;
	f32 max_y;

	f32 end_x;
	f32 end_y;

	union
	{
		GameButtonState buttons[12];
		struct
		{
			GameButtonState up;
			GameButtonState down;
			GameButtonState left;
			GameButtonState right;

			GameButtonState action_up;
			GameButtonState action_down;
			GameButtonState action_left;
			GameButtonState action_right;


			GameButtonState left_shoulder;
			GameButtonState right_shoulder;

			GameButtonState select;
			GameButtonState start;

			// NOTE(Nader): Need to add lp, mp, hp, lk, mk, hk buttons
			// NOTE(Nader): All buttons must be added above this line. 
			GameButtonState terminator;
		};
	};
} GameControllerInput;

typedef struct GameInput
{
	GameButtonState mouse_buttons[5];
	i32 mouse_x, mouse_y, mouse_z;

	f32 dt_for_frame;

	GameControllerInput controllers[4];
} GameInput;


