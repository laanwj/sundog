#version 300 es
/*
 * Copyright (c) 2023 Wladimir J. van der Laan
 * SPDX-License-Identifier: MIT
 */
/* ST screen palette mapping, fragment shader.
   hqx-ish variant inspired by https://godotshaders.com/shader/hq4x-shader-like-in-emulators/ */
precision highp float;

in vec2 texcoord;
out vec4 frag_color;
uniform lowp usampler2D scr_tex; // lowp is 0..511, enough here
uniform vec4 pal[16];
uniform sampler2D pal_tex;
uniform vec4 tint;

// Line offsets. Together these two determine the shape of the pixels.
//
// Line offset for 1 by 1 sloped lines
// With a line offset of 0 we won't see anything: the line connecting the
// diagonal pixel centers will be entirely outside the pixel.
// A line offset of sqrt(0.5) would make the line run through the pixel center.
const float LINE_OFFSET_A = 0.7071 / 2.0;
// Line offset for 2 by 1 sloped lines
// A line offset of 1/sqrt(5) would make the line run through the pixel center.
const float LINE_OFFSET_B = 0.4472 / 2.0;

// Anti aliasing scaling, smaller value make lines more blurry. This doesn't
// affect the position of the line.
const float AA_SCALE = 10.0;

uint fetch_idx(ivec2 uv) {
    return texelFetch(scr_tex, uv, 0).x;
}

// Draw diagonal line (a plane clipped to the square pixel- more like a triangle) connecting 2 pixels if they're the same color
bool diag(inout vec4 sum, vec2 sub, vec2 p1, vec2 p2, float line_offset, lowp uint v1, lowp uint v2, vec4 rgb) {
    if (v1 == v2) { // Consider different palette indices far apart enough
        vec2 dir = p2 - p1, lp = sub - p1;
        vec2 ortho = normalize(vec2(dir.y, -dir.x));
        float l = clamp((line_offset - dot(lp, ortho)) * AA_SCALE, 0., 1.);

        sum = mix(sum, rgb, l);
        return true;
    }
    return false;
}

void main() {
    ivec2 ipi = ivec2(texcoord);
    vec2 sub = fract(texcoord) - .5;

    // Start with center pixel as 'background'
    //vec4 s = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 s = pal[fetch_idx(ipi)];

    // Fetch all neighbours
    lowp uint v0 = fetch_idx(ipi + ivec2(-1, -1));
    lowp uint v1 = fetch_idx(ipi + ivec2( 0, -1));
    lowp uint v2 = fetch_idx(ipi + ivec2( 1, -1));
    lowp uint v3 = fetch_idx(ipi + ivec2(-1,  0));
    lowp uint v4 = fetch_idx(ipi + ivec2( 1,  0));
    lowp uint v5 = fetch_idx(ipi + ivec2(-1,  1));
    lowp uint v6 = fetch_idx(ipi + ivec2( 0,  1));
    lowp uint v7 = fetch_idx(ipi + ivec2( 1,  1));

    // But we only need RGB values for +-shaped neighbourhood, the rest will be matched against this.
    vec4 rgb1 = pal[v1];
    vec4 rgb3 = pal[v3];
    vec4 rgb6 = pal[v6];
    vec4 rgb4 = pal[v4];

    // Draw anti-aliased diagonal lines of surrounding pixels as 'foreground'
    // The inner diagonals provide "rounding" in case further pixels match.
    if (diag(s, sub, vec2(-1,  0), vec2( 0,  1), LINE_OFFSET_A, v3, v6, rgb3)) {
        diag(s, sub, vec2(-1,  0), vec2( 1,  1), LINE_OFFSET_B, v3, v7, rgb3);
        diag(s, sub, vec2(-1, -1), vec2( 0,  1), LINE_OFFSET_B, v0, v6, rgb3);
    }
    if (diag(s, sub, vec2( 0,  1), vec2( 1,  0), LINE_OFFSET_A, v6, v4, rgb6)) {
        diag(s, sub, vec2( 0,  1), vec2( 1, -1), LINE_OFFSET_B, v6, v2, rgb6);
        diag(s, sub, vec2(-1,  1), vec2( 1,  0), LINE_OFFSET_B, v5, v4, rgb6);
    }
    if (diag(s, sub, vec2( 1,  0), vec2( 0, -1), LINE_OFFSET_A, v4, v1, rgb4)) {
        diag(s, sub, vec2( 1,  0), vec2(-1, -1), LINE_OFFSET_B, v4, v0, rgb4);
        diag(s, sub, vec2( 1,  1), vec2( 0, -1), LINE_OFFSET_B, v7, v1, rgb4);
    }
    if (diag(s, sub, vec2( 0, -1), vec2(-1,  0), LINE_OFFSET_A, v1, v3, rgb1)) {
        diag(s, sub, vec2( 0, -1), vec2(-1,  1), LINE_OFFSET_B, v1, v5, rgb1);
        diag(s, sub, vec2( 1, -1), vec2(-1,  0), LINE_OFFSET_B, v2, v3, rgb1);
    }

    frag_color = s * tint;
}
