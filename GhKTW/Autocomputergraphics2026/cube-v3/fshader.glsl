#version 330 core

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// Must match the values of RenderMode in mainwidget.h
const int MODE_WIREFRAME = 0;
const int MODE_SOLID = 1;

uniform sampler2D cube_texture;
uniform int render_mode;

in vec2 v_texcoord;
in vec3 v_normal;

layout(location = 0) out vec4 fragColor;

//! [0]
void main()
{
    if (render_mode == MODE_WIREFRAME) {
        // Constant line color
        fragColor = vec4(0.9, 0.9, 0.9, 1.0);
    } else if (render_mode == MODE_SOLID) {
        // Lambert shading of a grey material. The light is fixed in the view frame,
        // so it stays at the same place on the screen when the object rotates.
        const vec3 lightDirection = normalize(vec3(0.4, 0.6, 1.0));
        const vec3 baseColor = vec3(0.7);
        const float ambient = 0.2;

        float diffuse = max(dot(normalize(v_normal), lightDirection), 0.0);
        fragColor = vec4(baseColor * (ambient + (1.0 - ambient) * diffuse), 1.0);
    } else {
        // Set fragment color from texture
        fragColor = texture(cube_texture, v_texcoord);
    }
}
//! [0]

