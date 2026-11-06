#version 330 core

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

uniform sampler2D cube_texture;

// Lighting parameters, all in the world frame
uniform int  lighting_enabled;
uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 view_pos;        // camera eye position

in vec2 v_texcoord;
in vec3 v_worldPos;
in vec3 v_worldNormal;

layout(location = 0) out vec4 fragColor;

//! [0]
void main()
{
    vec4 texColor = texture(cube_texture, v_texcoord);

    if (lighting_enabled == 0) {
        // Same as cube-v2: color from texture only
        fragColor = texColor;
        return;
    }

    // Phong reflection model: ambient + diffuse + specular
    vec3 N = normalize(v_worldNormal);
    vec3 L = normalize(light_pos - v_worldPos);
    vec3 V = normalize(view_pos - v_worldPos);
    vec3 R = reflect(-L, N);

    const float ambientStrength = 0.25;
    const float specularStrength = 0.5;
    const float shininess = 32.0;

    vec3 ambient  = ambientStrength * light_color;
    vec3 diffuse  = max(dot(N, L), 0.0) * light_color;
    vec3 specular = specularStrength * pow(max(dot(V, R), 0.0), shininess) * light_color;

    fragColor = vec4((ambient + diffuse) * texColor.rgb + specular, texColor.a);
}
//! [0]
