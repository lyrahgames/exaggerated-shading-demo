#version 460

uniform float halo = 1.0;
uniform float outer = 1.0;
uniform float inner = 0.6;
uniform float softness = 0.2;

in vec4 color;
flat in int id;
flat in int negative;

layout (location = 0) out vec4 frag_color;


float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

// Letter X: Two crossing diagonals
float sdX(vec2 p, float size) {
    float d1 = sdSegment(p, vec2(-size), vec2(size));
    float d2 = sdSegment(p, vec2(-size, size), vec2(size, -size));
    return min(d1, d2);
}

// Letter Y: A vertical stem and two branches
float sdY(vec2 p, float size) {
    float stem = sdSegment(p, vec2(0.0, -size), vec2(0.0, 0.0));
    float branchL = sdSegment(p, vec2(0.0, 0.0), vec2(-size, size));
    float branchR = sdSegment(p, vec2(0.0, 0.0), vec2(size, size));
    return min(stem, min(branchL, branchR));
}

// Letter Z: Top, bottom, and diagonal
float sdZ(vec2 p, float size) {
    float top = sdSegment(p, vec2(-size, size), vec2(size, size));
    float bot = sdSegment(p, vec2(-size, -size), vec2(size, -size));
    float diag = sdSegment(p, vec2(size, size), vec2(-size, -size));
    return min(top, min(bot, diag));
}

void main() {
    vec2 p = 2.0 * gl_PointCoord - 1.0;
    p.y = -p.y;
    if (dot(p, p) > outer) discard;

    float d = 1.0;

    if (id == 1) d = sdX(p, 0.4);
    else if (id == 3) d = sdY(p, 0.4);
    else if (id == 5) d = sdZ(p, 0.4);

    float boldness = 0.15;
    float sharpness = 0.05;
    float letter_mask = 1.0 - smoothstep(boldness - sharpness, boldness, d);
    vec4 letter_color = vec4(vec3(1.0), letter_mask);

    const float r = length(p);
    float alpha = 1.0 - smoothstep(outer - softness, outer, r);
    frag_color = vec4(color.rgb, color.a * alpha);
    if (bool(negative))
        frag_color = mix(vec4(1.0), frag_color, smoothstep(inner - softness, inner, r));

    frag_color = mix(frag_color, letter_color, letter_color.a);
}
