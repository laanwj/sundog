#version 300 es
/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * SPDX-License-Identifier: MIT
 */
/* ST screen palette mapping, fragment shader.
   hqx4x variant inspired by https://github.com/CrossVR/hqx-shader */
precision highp float;

in vec2 texcoord;
out vec4 frag_color;
uniform lowp usampler2D scr_tex; // lowp is 0..511, enough here
uniform vec4 pal[16];
uniform sampler2D aux_tex;
uniform vec4 tint;

const float WEIGHT_TOTAL = float(0x8) / float(0xf);

void main() {
    ivec2 ipi = ivec2(texcoord);
    vec2 fp = fract(texcoord);

    // Fetch 3x3 neighbourhood.
    // w[0] w[1] w[2]
    // w[3] w[4] w[5]
    // w[6] w[7] w[8]
    lowp uint w[9];
    w[0] = texelFetch(scr_tex, ipi + ivec2(-1, -1), 0).r;
    w[1] = texelFetch(scr_tex, ipi + ivec2( 0, -1), 0).r;
    w[2] = texelFetch(scr_tex, ipi + ivec2( 1, -1), 0).r;
    w[3] = texelFetch(scr_tex, ipi + ivec2(-1,  0), 0).r;
    w[4] = texelFetch(scr_tex, ipi + ivec2( 0,  0), 0).r;
    w[5] = texelFetch(scr_tex, ipi + ivec2( 1,  0), 0).r;
    w[6] = texelFetch(scr_tex, ipi + ivec2(-1,  1), 0).r;
    w[7] = texelFetch(scr_tex, ipi + ivec2( 0,  1), 0).r;
    w[8] = texelFetch(scr_tex, ipi + ivec2( 1,  1), 0).r;

    // Determine which four points to sample (that we need the actual color for),
    // depending on what quadrant we're in.
    ivec2 q = ivec2(fp * 2.0) * ivec2(2, 6);
    mat4x3 pixels = mat4x3(
        pal[w[4]].rgb,
        pal[w[q.x + q.y]].rgb,
        pal[w[3 + q.x]].rgb,
        pal[w[1 + q.y]].rgb);

    // Determine index to fetch in aux texture.
    ivec2 index;
    ivec2 gridpos = ivec2(fp * 4.0);
    index.x = (int(w[4] != w[0]) << 0) |
              (int(w[4] != w[1]) << 1) |
              (int(w[4] != w[2]) << 2) |
              (int(w[4] != w[3]) << 3) |
              (int(w[4] != w[5]) << 4) |
              (int(w[4] != w[6]) << 5) |
              (int(w[4] != w[7]) << 6) |
              (int(w[4] != w[8]) << 7);
    index.y = (int(w[3] != w[1]) << 4) |
              (int(w[1] != w[5]) << 5) |
              (int(w[7] != w[3]) << 6) |
              (int(w[5] != w[7]) << 7) |
              (gridpos.y << 2) |
              (gridpos.x << 0);

    // Fetch weights, and apply them.
    vec4 weights = texelFetch(aux_tex, index, 0);
    vec3 res = pixels * (weights / WEIGHT_TOTAL);

    frag_color = vec4(res * tint.rgb, 1.0);
}
