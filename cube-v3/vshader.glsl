#version 330 core

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

uniform mat4 mvp_matrix;
uniform mat4 model_matrix;   // object frame -> world frame
uniform mat3 normal_matrix;  // inverse-transpose of model_matrix (upper 3x3)

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texcoord;
layout(location = 2) in vec3 a_normal;

out vec2 v_texcoord;
out vec3 v_position;  // world-space position
out vec3 v_normal;    // world-space normal

//! [0]
void main()
{
    // Calculate vertex position in screen space
    gl_Position = mvp_matrix * a_position;

    // Pass texture coordinate to fragment shader
    // Value will be automatically interpolated to fragments inside polygon faces
    v_texcoord = a_texcoord;

    // World-space position and normal for lighting
    v_position = vec3(model_matrix * a_position);
    v_normal = normal_matrix * a_normal;
}
//! [0]
