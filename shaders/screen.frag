/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * SPDX-License-Identifier: MIT
 */
/* ST screen palette mapping, fragment shader. */
varying mediump vec2 texcoord;
uniform sampler2D scr_tex;
uniform sampler2D pal_tex;
uniform mediump vec4 tint;

void main()
{
    mediump vec2 idx = vec2(texture2D(scr_tex, texcoord).x, 0.0);
    gl_FragColor = texture2D(pal_tex, idx) * tint;
}
