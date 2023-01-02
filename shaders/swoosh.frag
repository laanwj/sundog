/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * SPDX-License-Identifier: MIT
 */
/* Startup "swoosh" effect, fragment shader. */
varying mediump vec2 texcoord;
uniform sampler2D scr_tex;

void main()
{
    gl_FragColor = texture2D(scr_tex, texcoord);
}
