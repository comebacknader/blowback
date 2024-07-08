// TODO(Nader): Don't pass in shader_program into the function. 
void
game_update_and_render(GameMemory *memory, GameInput *input, u32 shader_program) 
{
    GameState *game_state = (GameState *)memory->permanent_storage;
    // TODO(Nader): Move this initialization into the platform layer
    // TODO(Nader): Pass in the game_state? 
    if (!memory->is_initialized) {
        game_state->camera_position = v3(0.0f, 0.0f, 3.0f);
        game_state->camera_front = v3(0.0f, 0.0f, -1.0f);
        game_state->up = v3(0.0f, 1.0f, 0.0f);

        game_state->camera_target = v3(0.0f, 0.0f, 0.0f);
        game_state->camera_direction = HMM_NormV3(
                                HMM_SubV3(game_state->camera_position, game_state->camera_target));
        game_state->camera_right = HMM_NormV3(HMM_Cross(game_state->up, game_state->camera_direction));
        game_state->camera_up = HMM_Cross(game_state->camera_direction, game_state->camera_right);

        game_state->movement_x = 0.0f;
        game_state->movement_y = 0.0f;
        game_state->position_x = 0.0f;
        game_state->position_y = 0.0f;

        game_state->old_time = 0;
        game_state->new_time = 0;
        game_state->dt = 0;
        game_state->fps = 0.0f;
        game_state->shader_program = shader_program;
        game_state->window_width = 1280.0f;
        game_state->window_height = 720.0f;
        memory->is_initialized = true;
    }

    glUseProgram(game_state->shader_program);
    GameControllerInput keyboard_controller = input->controller;
    m4 view = m4_diagonal(1.0f);
    view = HMM_LookAt_RH(game_state->camera_position, 
                        HMM_AddV3(game_state->camera_position, game_state->camera_front), 
                        game_state->camera_up);

    m4 projection = m4_diagonal(1.0f);
    projection = HMM_Orthographic_RH_NO(0.0f, game_state->window_width, 
                    0.0f, game_state->window_height, 
                    -0.1f, 1000.0f);

    u32 view_location = glGetUniformLocation(game_state->shader_program, "view");
    u32 projection_location = glGetUniformLocation(game_state->shader_program, "projection");

    glUniformMatrix4fv(view_location, 1, GL_FALSE, &view.Elements[0][0]);
    glUniformMatrix4fv(projection_location, 1, GL_FALSE, &projection.Elements[0][0]);

    v3 scale = v3(50.0f, 50.0f, 0.0f);
    // adjusting is having bottom left of image be where it is drawn.
    f32 adjust_x = scale.X;
    f32 adjust_y = scale.Y;
    v3 translation = v3(game_state->position_x + adjust_x, 
                        game_state->position_y + adjust_y, 
                        0.0f);

    if (keyboard_controller.move_right.ended_down) {
        game_state->position_x += 10.0f;
    }

    if (keyboard_controller.move_up.ended_down) {
        game_state->position_y += 10.0f;
    } 

    if (keyboard_controller.move_down.ended_down) {
        game_state->position_y -= 10.0f;
    }
    
    if (keyboard_controller.move_left.ended_down) {
        game_state->position_x -= 10.0f;
    }
    
    m4 model = HMM_M4D(1.0f);

    // Scale
    model = HMM_Scale(scale);

    // Translation
    model.Columns[3].X = translation.X;
    model.Columns[3].Y = translation.Y;
    model.Columns[3].Z = 0.0f;

    u32 model_location = glGetUniformLocation(game_state->shader_program, "model");
    glUniformMatrix4fv(model_location, 1, GL_FALSE, &model.Elements[0][0]);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

}