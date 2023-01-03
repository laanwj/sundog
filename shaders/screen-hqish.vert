/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * SPDX-License-Identifier: MIT
 */
/* ST screen palette mapping, vertex shader. */
#version 300 es
precision highp float;

in vec4 vertexCoord;
in vec2 vertexTexCoord;
out vec2 texcoord;

void main()
{
    gl_Position = vertexCoord;
    texcoord = vertexTexCoord;
}
