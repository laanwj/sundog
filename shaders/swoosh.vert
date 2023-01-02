/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * SPDX-License-Identifier: MIT
 */
/* Startup "swoosh" effect, vertex shader. */
attribute mediump vec4 vertexCoord;
attribute mediump vec2 vertexTexCoord;
varying mediump vec2 texcoord;

void main()
{
    gl_Position = vertexCoord;
    texcoord = vertexTexCoord;
}
