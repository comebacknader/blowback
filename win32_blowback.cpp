#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <windows.h>

#include "platform.h"
#define GL_LITE_IMPLEMENTATION
#include "gl_lite.h"
#include "blowback.h"
#include "win32_blowback.h"

#include "shader.cpp"
#include "blowback.cpp"

/*

TODO(Nader): Separate platform layer from gameplay code
    - Pass in Input
	- Pass in Memory - this saves state, so my variables outside of 
	- game loop can come in.
	Gameplay code just wants to draw an object with
	- certain dimensions
	- DrawRect(v2{width, height}, v2{positionX, positionY}, color v4{rgba})
	- The platform layer implement drawing this, and the gameplay code
	just calls this global function
TODO(Nader): Implement Hot Reloading
TODO(Nader): Have camera track player movement
TODO(Nader): Implement playback for debugging and for replays
TODO(Nader): Implement fixed point deterministic physics


*/

const f32 WINDOW_WIDTH = 1280.0f;
const f32 WINDOW_HEIGHT = 720.0f;
global HGLRC rendering_context;
global b32 game_loop;

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
	int initialized_opengl = gl_lite_init();
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
				else if (vk_code == VK_UP)
				{
					keyboard_controller->move_up.ended_down = true;
				}
				else if (vk_code == VK_DOWN)
				{
					keyboard_controller->move_down.ended_down = true;
				}
				else if (vk_code == VK_LEFT)
				{
					keyboard_controller->move_left.ended_down = true;
				}
				else if (vk_code == VK_RIGHT)
				{
					keyboard_controller->move_right.ended_down = true;
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
				else if (vk_code == VK_UP)
				{
					keyboard_controller->move_up.ended_down = false;
				}
				else if (vk_code == VK_DOWN)
				{
					keyboard_controller->move_down.ended_down = false;
				}
				else if (vk_code == VK_LEFT)
				{
					keyboard_controller->move_left.ended_down = false;
				}
				else if (vk_code == VK_RIGHT)
				{
					keyboard_controller->move_right.ended_down = false;
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

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE previous_instance,
        LPSTR command_line, int show_code) 
{

	Win32State win32_state = {};

	WNDCLASSA window_class = { 0 };
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = win32_main_window_callback;
	window_class.hInstance = instance;
	window_class.lpszClassName = "BlowbackWindowClass";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

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

			LPVOID base_address = 0;
			GameMemory game_memory = {};
			game_memory.permanent_storage_size = megabytes(64);
			game_memory.transient_storage_size = gigabytes(1);
			win32_state.total_size =  game_memory.permanent_storage_size + game_memory.transient_storage_size;
			win32_state.game_memory_block = VirtualAlloc(base_address, game_memory.permanent_storage_size,
											MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			game_memory.permanent_storage = win32_state.game_memory_block;
			game_memory.transient_storage = ((u8 *)game_memory.permanent_storage +
											game_memory.permanent_storage_size);

            char* sprite_vertex_filepath = "G:\\work\\blowback\\vertex_shader.vert";
            char* sprite_fragment_filepath = "G:\\work\\blowback\\fragment_shader.frag";
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

			GameInput input = { 0 };
			GameControllerInput keyboard_controller = {0};
			keyboard_controller.is_connected = true;
			input.controller = keyboard_controller; 
            while (game_loop) 
			{
                //old_time = new_time;
                //new_time = SDL_GetTicks();
                //dt = new_time - old_time;
                //fps = 1.0f/((f32)dt/1000.0f);
        #if 0
                printf("%0.0ffps | %dms/f \n", fps, dt);
        #endif
				HDC window_device_context = GetDC(window);
				RECT client_rect;
				GetClientRect(window, &client_rect);
				int window_width = client_rect.right - client_rect.left;
				int window_height = client_rect.bottom - client_rect.top;

				win32_process_pending_messages(&keyboard_controller);

                //OutputDebugStringA("GOT HERE \n");
                glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
                glClearColor(0.8f, 0.2f, 0.5f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

				game_update_and_render(&game_memory, &input, shader_program);

				SwapBuffers(window_device_context);
				ReleaseDC(window, window_device_context);
            }

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