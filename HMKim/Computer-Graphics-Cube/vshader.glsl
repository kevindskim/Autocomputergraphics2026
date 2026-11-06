#version 330 core

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

uniform mat4 mvp_matrix;
uniform mat4 model_matrix;    // object frame -> world frame
uniform mat3 normal_matrix;   // (Mo^-1)^T, upper-left 3x3

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texcoord;
layout(location = 2) in vec3 a_normal;

out vec2 v_texcoord;
out vec3 v_worldPos;
out vec3 v_worldNormal;

//! [0]
void main()
{
    // Calculate vertex position in screen space
    gl_Position = mvp_matrix * a_position;

    // Position and normal in the world frame, used for lighting
    v_worldPos = vec3(model_matrix * a_position);
    v_worldNormal = normal_matrix * a_normal;

    // Pass texture coordinate to fragment shader
    // Value will be automatically interpolated to fragments inside polygon faces
    v_texcoord = a_texcoord;
}
//! [0]
