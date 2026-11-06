#version 330 core

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

uniform sampler2D cube_texture;
uniform vec3 light_position;  // world frame, (0, 5, 5)
uniform vec3 view_position;   // camera eye in world frame
uniform vec3  srcSpec;   // 예: vec3(1.0)
uniform vec3  matSpec;   // 예: vec3(0.5)
uniform float matSh;     // 예: 32.0

in vec2 v_texcoord;
in vec3 v_position;
in vec3 v_normal;

layout(location = 0) out vec4 fragColor;

//! [0]
void main()
{
    vec4 texColor = texture(cube_texture, v_texcoord);

    vec3 N = normalize(v_normal);
    vec3 I = normalize(v_position - light_position );
    vec3 V = normalize(view_position - v_position);
    vec3 R = reflect(I, N);

    // Phong lighting: ambient + diffuse + specular
    float ambient  = 0.2;
    float diffuse  = max(dot(N, -I), 0.0);
    vec3 specular = (diffuse > 0.0) ? pow(max(dot(R, V), 0.0), matSh) * srcSpec * matSpec : vec3(0.0);

    vec3 color = (ambient + diffuse) * texColor.rgb + specular;
    fragColor = vec4(color, texColor.a);
}
//! [0]

