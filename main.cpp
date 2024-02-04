#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <SDL.h>
#undef main

#include "platform.h"
#define GL_LITE_IMPLEMENTATION
#include "gl_lite.h"

#include "shader.cpp"
#include "blowback.cpp"

/*

TODO(Nader): Get a better understanding of the world. 
    - Have a camera that can move throughout a level
    - A level consists of a 2D array of 1's/0's
TODO(Nader): Separate platform layer from gameplay code
    - Pass in Input
TODO(Nader): Implement Hot Reloading


*/

const f32 WINDOW_WIDTH = 1280.0f;
const f32 WINDOW_HEIGHT = 720.0f;

char *
read_file(char *filename) {
    char *buffer;
    i32 length;
    FILE *file = fopen(filename, "r");

    if (file) {
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer = (char *) malloc(length);
        if (buffer) {
            fread(buffer, 1, length, file);
        }
        fclose(file);
    }
    printf("Successfully loaded %s \n", filename);
    return buffer;
} 

int main(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "SDL_Init Error: %s \n", SDL_GetError());
        return EXIT_FAILURE;
    }

    u32 window_flags = SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_SHOWN;
    SDL_Window *window = SDL_CreateWindow("Hello World!", 100, 100, 1280, 720, window_flags);

    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow Error: %s \n", SDL_GetError());
    }
    
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "SDL_CreateContext Error: %s \n", SDL_GetError());
    }

    stbi_set_flip_vertically_on_load(true);

    int initialize_opengl = gl_lite_init();
    if (!initialize_opengl) {
        printf("Failed to dynamically load function pointers \n");
    } else {
        printf("Successfully loaded function pointers \n");
    }

    b32 game_loop = true;
    b32 full_screen = false;

    //printf("vertex_shader_source: %s \n", vertex_shader_source);
    char *vertex_shader_source = read_file("vertex_shader.vert");
    char *fragment_shader_source = read_file("fragment_shader.frag");
    
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

    v3 camera_position = v3(0.0f, 0.0f, 3.0f);
    v3 camera_front = v3(0.0f, 0.0f, -1.0f);
    v3 up = v3(0.0f, 1.0f, 0.0f);

    v3 camera_target = v3(0.0f, 0.0f, 0.0f);
    v3 camera_direction = HMM_NormV3(HMM_SubV3(camera_position, camera_target));
    v3 camera_right = HMM_NormV3(HMM_Cross(up, camera_direction));
    v3 camera_up = HMM_Cross(camera_direction, camera_right);

    f32 movement_x = 0.0f;
    f32 movement_y = 0.0f;

    b32 right_pressed = false;
    b32 left_pressed = false;
    b32 up_pressed = false;
    b32 down_pressed = false;

    u32 old_time = 0;
    u32 new_time = 0;
    u32 dt = 0;
    f32 fps = 0.0f;
    while (game_loop) {
        old_time = new_time;
        new_time = SDL_GetTicks();
        dt = new_time - old_time;
        fps = 1.0f/((f32)dt/1000.0f);
        printf("%0.0ffps | %dms/f \n", fps, dt);
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        game_loop = false;
                        break;
                    case 'f':
                        full_screen = !full_screen;
                        if (full_screen) {
                            SDL_SetWindowFullscreen(window, window_flags | SDL_WINDOW_FULLSCREEN_DESKTOP);
                        } else {
                            SDL_SetWindowFullscreen(window, window_flags);
                        }
                        break;
                    case SDLK_RIGHT:
                        right_pressed = true;
                        break;
                    case SDLK_LEFT:
                        left_pressed = true;
                        break;
                    case SDLK_UP:
                        up_pressed = true;
                        break;
                    case SDLK_DOWN:
                        down_pressed = true;
                        break;
                    default:
                        break;
                }
            } else if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT:
                        right_pressed = false;
                        break;
                    case SDLK_LEFT:
                        left_pressed = false;
                        break;
                    case SDLK_UP:
                        up_pressed = false;
                        break;
                    case SDLK_DOWN:
                        down_pressed = false;
                        break;
                    default: 
                        break;
                }
            } else if (event.type == SDL_QUIT) {
                game_loop = false;
            }
        }

        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);

        m4 view = m4_diagonal(1.0f);
        view = HMM_LookAt_RH(camera_position, HMM_AddV3(camera_position, camera_front), camera_up);

        m4 projection = m4_diagonal(1.0f);
        projection = HMM_Orthographic_RH_NO(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, -0.1f, 1000.0f);

        u32 view_location = glGetUniformLocation(shader_program, "view");
        u32 projection_location = glGetUniformLocation(shader_program, "projection");

        glUniformMatrix4fv(view_location, 1, GL_FALSE, &view.Elements[0][0]);
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection.Elements[0][0]);

        v3 scale = v3(50.0f, 50.0f, 0.0f);
        // adjusting is having bottom left of image be where it is drawn.
        f32 adjust_x = scale.X;
        f32 adjust_y = scale.Y;
        v3 translation = v3(movement_x + adjust_x, movement_y + adjust_y, 0.0f);

        if (right_pressed) {
            movement_x += 10.0f;
        }

        if (up_pressed) {
            movement_y += 10.0f;
        } 

        if (down_pressed) {
            movement_y -= 10.0f;
        }
        
        if (left_pressed) {
            movement_x -= 10.0f;
        }
        
        m4 model = HMM_M4D(1.0f);

        // Scale
        model = HMM_Scale(scale);

        // Translation
        model.Columns[3].X = translation.X;
        model.Columns[3].Y = translation.Y;
        model.Columns[3].Z = 0.0f;

        u32 model_location = glGetUniformLocation(shader_program, "model");
        glUniformMatrix4fv(model_location, 1, GL_FALSE, &model.Elements[0][0]);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        SDL_GL_SwapWindow(window);
    }
    free(vertex_shader_source);
    free(fragment_shader_source);
}