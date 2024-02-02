#pragma once

#include <stdint.h>
#include <stdbool.h>
#define HANDMADE_MATH_USE_DEGREES
#include "HandmadeMath.h"

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef HMM_Mat4 m4;
typedef HMM_Mat3 m3;
typedef HMM_Vec3 v3;
typedef HMM_Vec2 v2;

#define v3(x, y, z) HMM_V3(x, y, z)
#define m4_diagonal(value) HMM_M4D(value)

#define internal static
#define local_persist static
#define global static

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define terabytes(value) (gigabytes(value)*1024LL)

#define array_count(arr) (sizeof(arr) / sizeof((arr)[0]))

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

	union
	{
		GameButtonState buttons[12];
		struct
		{
			GameButtonState move_up;
			GameButtonState move_down;
			GameButtonState move_left;
			GameButtonState move_right;

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

	// NOTE(Nader): I don't understand why each Input has 5 controllers...
	GameControllerInput controllers[5];
} GameInput;

inline GameControllerInput* get_controller(GameInput* input, u8 index)
{
	// TODO(Nader): Add an assert(index < array_count(input->controllers))

	GameControllerInput* result = &input->controllers[index];
	return(result);
}

