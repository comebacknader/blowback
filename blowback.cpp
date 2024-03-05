void
game_update_and_render(GameMemory *memory, GameInput *input) 
{
    
    if (!memory->is_initialized) {
        memory->is_initialized = true;
    }

    glUseProgram(shader_program);
    GameControllerInput keyboard_controller = input->controller;
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
    v3 translation = v3(position_x + adjust_x, position_y + adjust_y, 0.0f);

    if (keyboard_controller.move_right.ended_down) {
        position_x += 10.0f;
    }

    if (keyboard_controller.move_up.ended_down) {
        position_y += 10.0f;
    } 

    if (keyboard_controller.move_down.ended_down) {
        position_y -= 10.0f;
    }
    
    if (keyboard_controller.move_left.ended_down) {
        position_x -= 10.0f;
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


}