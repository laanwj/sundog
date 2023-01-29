#version 300 es
/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * SPDX-License-Identifier: MIT
 */
/* ST screen palette mapping, vertex shader. */
precision highp float;

in vec4 vertexCoord;
in vec2 vertexTexCoord;
out vec2 texcoord;

void main()
{
    gl_Position = vertexCoord;
    texcoord = vertexTexCoord;
}
