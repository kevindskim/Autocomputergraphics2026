#version 330 core

uniform mat4 mvp_matrix;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;

out vec3 v_color;

void main()
{
    // The grid is defined in the world frame, so it has no model transformation
    gl_Position = mvp_matrix * vec4(a_position, 1.0);

    v_color = a_color;
}
