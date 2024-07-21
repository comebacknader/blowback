#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <windows.h>
#include <xinput.h>

#include "platform.h"
#include "blowback.h"
#define GL_LITE_IMPLEMENTATION
#include "gl_lite.h"
#include "win32_blowback.h"

#include "shader.c"
#include "blowback.c"

/*

	TODO(Nader): Things that need to be done for shippable platform layer: 

	- Saved game locations
	- Getting a handle to our own executable ifle
	- Asset loading path
	- Threading (launch a thread)
	- Raw Input (support for multiple keyboards) (?)
	- ClipCursor() (for multimonitor support)
	- Fullscreen support
	- WM_SETCURSOR (control cursor visibility)
	- QueryCancelAutoplay
	- WM_ACTIVATEAPP (for when we are not the active application)
	- Blit speed improvements (BitBlt)
	- Hardware acceleration (OpenGL or Direct3D or BOTH?)
	- GetKeyboardLayout (for French keyboards, international WASD support)

*/

/*

NOTE(Nader): WORD is Windows for 16-bit unsigned integer 
NOTE(Nader): DWORD is Windows for 32-bit value unsigned integer
NOTE(Nader): SHORT is Windows 16-bit signed value

TODO(Nader): Render a tile map
TODO(Nader): Have the camera follow the player as he travels between different tilemaps
	- Then I'll have an understanding of rendering offscreen items and coordinate systems 
TODO(Nader): Abstract away the renderer
TODO(Nader): Implement Hot Reloading
TODO(Nader): Have camera track player movement
TODO(Nader): Implement playback for debugging and for replays

*/

const f32 WINDOW_WIDTH = 1280.0f;
const f32 WINDOW_HEIGHT = 720.0f;
global HGLRC rendering_context;
global b32 game_loop;
static i64 global_performance_counter_frequency; 

/*

The reason for using these typedef functions is that we don't want to link 
directly to Xinput.lib, because if it can't find one of these DLL's (Xinput1_4.dll, Xinput9_1_0.dll) on the 
system, then the game won't load, so we want to load the Windows functions ourselves, so that
if they DON'T load, we can stub out the functions and the game won't crash. 

So we want to get the code into our executable for a Windows binding, and looking up the 
function pointer so we can call into it without using an import library. 

With these typedefs, we want to do something like 

x_input_get_state *Foo; // A function pointer

We could end it with just doing:
global x_input_get_state *DynamicXInputGetState;

But Casey wants to be clever, and allow us to use the same name XInputGetState, so
he does:

global x_input_get_state *XInputGetState_;

And then does a pound define: 

#define XInputGetState XInputGetState_

Which allows the user to just call XInputGetState instead of DynamicXInputGetState. That is 
what the reasoning is behind having the code below:

typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_STATE *pVibration);
global x_input_get_state *XInputGetState_;
global x_input_set_state *XInputSetState_;
#define XInputGetState XInputGetState_
#define XInputGetState XInputSetState_

But when you do this, and the XInput library isn't there, then the code will fail, and we 
don't want to fail, because the player can just use the keyboard if there's no controller. 

So, we create stubs in that case, leading to the code below.

*/
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState) // function prototype
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration) // function prototype
// NOTE(Nader): Here we define a type for that function prototype so we can use it as a pointer
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

// NOTE(Nader): These are stubs we define so that our function pointers point to a default function 
// right off the bat, and the app doesn't crash. 
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(0);
}

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(0);
}

// Setting the pointer to the stub as a default value.
global x_input_get_state *XInputGetState_ = XInputGetStateStub;
global x_input_set_state *XInputSetState_ = XInputSetStateStub;
// A #define in order to be able to use the XInputGetState rather than have to use XInputGetState_ in the code.
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

// This will essentially do the steps that the Windows loader does when it loads our program.
internal void
win32_load_xinput(void)
{
	// NOTE(Nader): Maybe xinput1_3.dll would support older systems? 
	HMODULE x_input_library = LoadLibraryA("xinput1_4.dll");	
	if (x_input_library)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(x_input_library, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(x_input_library, "XInputSetState");
	}
}

internal f32 
win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
	f32 result = (((f32)(end.QuadPart - start.QuadPart)) / ((f32)global_performance_counter_frequency)); 
	return(result);
}

internal LRESULT CALLBACK
win32_main_window_callback(HWND window, UINT message, WPARAM wparam, LPARAM lparam) 
{
	LRESULT result = 0;
	switch (message)
	{
	case WM_CLOSE:
	{
		game_loop = false;
	} break;
	case WM_ACTIVATEAPP:
	{

	} break;
	case WM_DESTROY:
	{

	} break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{} break;
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC device_context = BeginPaint(window, &paint);
		EndPaint(window, &paint);
	} break;
	default:
	{
		result = DefWindowProcA(window, message, wparam, lparam);
	} break;
	}
	return(result);
}

internal void
win32_init_opengl(HWND window)
{
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,
		8,
		0,
		PFD_MAIN_PLANE,
		0, 0, 0
	};

	HDC window_dc = GetDC(window);
	int pixel_format = ChoosePixelFormat(window_dc, &pfd);
	SetPixelFormat(window_dc, pixel_format, &pfd);
	rendering_context = wglCreateContext(window_dc);
	if (wglMakeCurrent(window_dc, rendering_context))
	{
		// TODO(Nader): Log
	}
	else
	{
		OutputDebugStringA("Failed to make current openGL rendering context \n");
	}
	b32 initialized_opengl = gl_lite_init();
	if (!initialized_opengl)
	{
		OutputDebugStringA("Failed to dynamically load function pointers \n");
	}
	else
	{
		OutputDebugStringA("Successfully loaded function pointers \n");
	}
	ReleaseDC(window, window_dc);
}

inline u32
truncate_u64(u64 value)
{
	u32 result = (u32)value;
	return(result);
}

internal void
free_file_memory(void *file_memory)
{
	if (file_memory)
	{
		VirtualFree(file_memory, 0, MEM_RELEASE);
	}
}

typedef struct FileReadResults
{
	u32 contents_size;
	void *contents;
} FileReadResults;

internal FileReadResults
read_file_to_memory(char *filepath)
{
	FileReadResults result = { 0 };
	HANDLE file_handle = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file_handle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size;
		if (GetFileSizeEx(file_handle, &file_size))
		{
			u32 file_size_32 = truncate_u64(file_size.QuadPart);
			result.contents = VirtualAlloc(0, file_size_32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.contents)
			{
				DWORD bytes_read;
				if (ReadFile(file_handle, result.contents, file_size_32, &bytes_read, 0)
					&& (file_size_32 == bytes_read))
				{
					OutputDebugStringA("File Read successfully \n");
					result.contents_size = file_size_32;
				}
				else
				{
					if (result.contents)
					{
						free_file_memory(result.contents);
					}
				}
			}
		}
		else
		{
			// TODO(Nader): Logging 
		}
		CloseHandle(file_handle);
	}
	return(result);
}

internal void
win32_process_pending_messages(GameControllerInput *keyboard_controller)
{
	MSG message = { 0 };
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		switch (message.message)
		{
		case WM_QUIT:
		{
			game_loop = false;
			wglDeleteContext(rendering_context);
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		{ 
			// NOTE(Nader): This should be using keyboard_controller to store movement
			// NOTE(Nader): A vk_code tells you which key got pressed
			u32 vk_code = (u32)message.wParam;
			b32 was_down = ((message.lParam & (1 << 30)) != 0);
			b32 is_down = ((message.lParam & (1 << 31)) == 0);

			if (was_down != is_down)
			{
				if (vk_code == VK_ESCAPE)
				{
					game_loop = false;
				}
				else if (vk_code == VK_UP || vk_code == 'W')
				{
					keyboard_controller->up.ended_down = true;
				}
				else if (vk_code == VK_DOWN || vk_code == 'S')
				{
					keyboard_controller->down.ended_down = true;
				}
				else if (vk_code == VK_LEFT || vk_code == 'A')
				{
					keyboard_controller->left.ended_down = true;
				}
				else if (vk_code == VK_RIGHT || vk_code == 'D')
				{
					keyboard_controller->right.ended_down = true;
				}
			}
		} break;	
		case WM_KEYUP:
		{
			// This should be using keyboard_controller to store movement
			u32 vk_code = (u32)message.wParam;
			b32 was_down = ((message.lParam & (1 << 30)) != 0);
			b32 is_down = ((message.lParam & (1 << 31)) == 0);

			if (was_down != is_down)
			{
				if (vk_code == VK_ESCAPE)
				{
					game_loop = false;
				}
				else if (vk_code == VK_UP || vk_code == 'W')
				{
					keyboard_controller->up.ended_down = false;
				}
				else if (vk_code == VK_DOWN || vk_code == 'S')
				{
					keyboard_controller->down.ended_down = false;
				}
				else if (vk_code == VK_LEFT || vk_code == 'A')
				{
					keyboard_controller->left.ended_down = false;
				}
				else if (vk_code == VK_RIGHT || vk_code == 'D')
				{
					keyboard_controller->right.ended_down = false;
				}
			}
		} break;
		default:
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		} break;
		}
	}
}

internal LARGE_INTEGER
win32_get_wall_clock()
{
	LARGE_INTEGER wall_clock;
	QueryPerformanceCounter(&wall_clock);
	return(wall_clock);
};

internal void
win32_process_xinput_digital_button(DWORD x_input_button_state, GameButtonState *old_state, 
									GameButtonState *new_state, DWORD button_bit)
{
	// NOTE(Nader): need to check if the button was ended_down before, if it was, 
	// then there was a half transition, if it wasn't ended_down before, then there 
	// wasn't a half_transition.
	new_state->ended_down = ((x_input_button_state & button_bit)) == button_bit; 
	new_state->half_transition_count = (old_state->ended_down != new_state->ended_down) ? 1 : 0;


}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE previous_instance,
        LPSTR command_line, int show_code) 
{
	win32_load_xinput();
	Win32State win32_state = { 0 };

	WNDCLASSA window_class = { 0 };
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = win32_main_window_callback;
	window_class.hInstance = instance;
	window_class.lpszClassName = "BlowbackWindowClass";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

	// TODO(Nader): How do we reliably query this on Windows?
	LARGE_INTEGER performance_counter_frequency_result;
	QueryPerformanceFrequency(&performance_counter_frequency_result);
	global_performance_counter_frequency = performance_counter_frequency_result.QuadPart;

    UINT desired_scheduler_ms = 1;
    b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
 
	int monitor_refresh_rate_hz = 60;
	int game_update_hz = monitor_refresh_rate_hz;
	f32 target_seconds_elapsed_per_frame = 1.0f / (f32)(monitor_refresh_rate_hz);

    if (RegisterClassA(&window_class))
    {
        stbi_set_flip_vertically_on_load(true);

        HWND window = CreateWindowExA(
            0, window_class.lpszClassName, "Blowback",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
            WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, instance, 0);
        if (window)
        {
            win32_init_opengl(window);
            game_loop = true;
            b32 full_screen = false;

			// ALLOCATE GAME MEMORY
			LPVOID base_address = 0;
			GameMemory game_memory = { 0 };
			game_memory.permanent_storage_size = megabytes(64);
			win32_state.total_size =  game_memory.permanent_storage_size + game_memory.transient_storage_size;
			win32_state.game_memory_block = VirtualAlloc(base_address, game_memory.permanent_storage_size,
											MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			game_memory.permanent_storage = win32_state.game_memory_block;

			// Ephemeral storage
			game_memory.transient_storage_size = gigabytes(1);
			game_memory.transient_storage = ((u8 *)game_memory.permanent_storage +
											game_memory.permanent_storage_size);

            
			// RENDERER SETUP
            char* sprite_vertex_filepath = "D:\\work\\blowback\\vertex_shader.vert";
            char* sprite_fragment_filepath = "D:\\work\\blowback\\fragment_shader.frag";
            FileReadResults sprite_vertex_file = read_file_to_memory(sprite_vertex_filepath);
            FileReadResults sprite_fragment_file = read_file_to_memory(sprite_fragment_filepath);

            char *vertex_shader_source = (char *)sprite_vertex_file.contents;
            char *fragment_shader_source = (char *)sprite_fragment_file.contents; 

            u32 vertex_shader, fragment_shader; 

            vertex_shader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
            glCompileShader(vertex_shader);

            fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
            glCompileShader(fragment_shader);

            check_shader_errors("VERTEX", vertex_shader);
            check_shader_errors("FRAGMENT", fragment_shader);

            u32 shader_program;
            shader_program = glCreateProgram();
            glAttachShader(shader_program, vertex_shader);
            glAttachShader(shader_program, fragment_shader);
            glLinkProgram(shader_program);

            check_shader_errors("PROGRAM", shader_program);

            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);

            f32 vertices[] = {
                1.0f, 1.0f, 0.0f,   // top right
                1.0f, -1.0f, 0.0f,   // bottom right 
                -1.0f, -1.0f, 0.0f,   // bottom left
                -1.0f, 1.0f, 0.0f,   // top left 
            };

            u32 indices[] = {
                0, 1, 3,    // first triangle
                1, 2, 3     // second triangle
            };

            u32 vao, vbo, ebo;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

			// INPUT SETUP
			GameInput input[2] = {0};
			GameInput *new_input = &input[0];
			GameInput *old_input = &input[1];

			// START GAME LOOP TIMING  
			LARGE_INTEGER last_counter;
			QueryPerformanceCounter(&last_counter);
			u64 last_cycle_count = __rdtsc();

			// GAME LOOP
            while (game_loop) 
			{
				HDC window_device_context = GetDC(window);

				// TODO(Nader): Should we poll this more frequently? 
				DWORD max_controller_count = XUSER_MAX_COUNT;
				// NOTE(Nader): This just makes sure that if XUSER_MAX_COUNT is more than 4, 
				// we don't start filling out information past input.controllers[4] which would
				// override memory past the 4th controller value into unknown memory. 
				if (max_controller_count > array_count(new_input->controllers))
				{
					max_controller_count = array_count(new_input->controllers);
				}
				for (DWORD controller_index = 0;
					controller_index < max_controller_count;
					++controller_index)
				{
					GameControllerInput *old_controller = &old_input->controllers[controller_index];
					GameControllerInput *new_controller = &new_input->controllers[controller_index];

					XINPUT_STATE controller_state;
					if (XInputGetState(controller_index, &controller_state) == ERROR_SUCCESS)
						{
						// NOTE(Nader): This controller is plugged in
						// TODO(Nader): See if controller_state.dwPacketNumber increments too rapidly
						XINPUT_GAMEPAD *pad = &controller_state.Gamepad;

						bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

						win32_process_xinput_digital_button(pad->wButtons, &old_controller->down, 
															&new_controller->down, XINPUT_GAMEPAD_DPAD_DOWN);
						win32_process_xinput_digital_button(pad->wButtons, &old_controller->right, 
															&new_controller->right, XINPUT_GAMEPAD_DPAD_RIGHT);
						win32_process_xinput_digital_button(pad->wButtons, &old_controller->left, 
															&new_controller->left, XINPUT_GAMEPAD_DPAD_LEFT);
						win32_process_xinput_digital_button(pad->wButtons, &old_controller->up, 
															&new_controller->up, XINPUT_GAMEPAD_DPAD_UP);
						win32_process_xinput_digital_button(pad->wButtons, &old_controller->left_shoulder, 
															&new_controller->left_shoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
						win32_process_xinput_digital_button(pad->wButtons, &old_controller->right_shoulder, 
															&new_controller->right_shoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);

						// bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
						// bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool left_shoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool right_shoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool a_button = (pad->wButtons & XINPUT_GAMEPAD_A);
						bool b_button = (pad->wButtons & XINPUT_GAMEPAD_B);
						bool x_button = (pad->wButtons & XINPUT_GAMEPAD_X);
						bool y_button = (pad->wButtons & XINPUT_GAMEPAD_Y);

						i16 stick_x = pad->sThumbLX;
						i16 stick_y = pad->sThumbLY;
						if (a_button)
						{
							OutputDebugStringA("A button was pressed \n");
						}
					}
					else
					{
						// NOTE(Nader): The controller is not available
					}
				}

				win32_process_pending_messages(&new_input->controllers[0]);

                glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
                glClearColor(0.8f, 0.2f, 0.5f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

				// UPDATE & RENDER
				game_update_and_render(&game_memory, new_input, shader_program);

				SwapBuffers(window_device_context);
				ReleaseDC(window, window_device_context);

				// -- END GAME LOOP TIMING --

				u64 end_cycle_count = __rdtsc();				

				LARGE_INTEGER end_counter = win32_get_wall_clock();

				u64 cycles_elapsed = end_cycle_count - last_cycle_count;
				i64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
				f32 work_seconds_elapsed = win32_get_seconds_elapsed(last_counter, end_counter);
				f32 seconds_elapsed_for_frame = work_seconds_elapsed;

				if(seconds_elapsed_for_frame < target_seconds_elapsed_per_frame)
				{
					if(sleep_is_granular)
					{
						DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_elapsed_per_frame - seconds_elapsed_for_frame));
						if(sleep_ms > 0)
						{
							Sleep(sleep_ms);
						}
					}

					f32 test_seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter,
																					win32_get_wall_clock());
				
				 	// TODO(Nader): This assert statement needs to be handled correctly.
					//asserts(test_seconds_elapsed_for_frame < target_seconds_elapsed_per_frame);

					while(seconds_elapsed_for_frame < target_seconds_elapsed_per_frame)
					{
						seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
					}
				}
				else 
				{
					// TODO(Nader): Missed frame rate
					// TODO(Nader): Logging
				}

				end_counter = win32_get_wall_clock();
				counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
				f64 ms_per_frame = 1000.0f*win32_get_seconds_elapsed(last_counter, end_counter);
				f64 fps = ((f64)global_performance_counter_frequency / (f64)counter_elapsed);

				last_counter = end_counter;
				char metrics_text[256];
				sprintf_s(metrics_text, sizeof(metrics_text), 
					"counter_elapsed: %I64d  | qlobal_peformance_counter_frequency: %I64d \n", 
						counter_elapsed,
						global_performance_counter_frequency);
				OutputDebugStringA(metrics_text);			

				// char metrics_text[256];
				// sprintf_s(
				// 	metrics_text, 
				// 	sizeof(metrics_text),
				// 	"ms/f: %.02f | fps: %.02f \n", 
				// 	ms_per_frame, 
				// 	fps);
				// OutputDebugStringA(metrics_text);
            }

			GameInput *temp = new_input;
			new_input = old_input;
			old_input = temp;

			// END GAME LOOP

        }
        else
        {
            //TODO(Nader): Logging
        }
    }
    else
    {
        // TODO(Nader): Logging
    }

}
