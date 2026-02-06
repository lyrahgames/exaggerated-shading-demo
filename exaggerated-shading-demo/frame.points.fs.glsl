#version 460

layout (location = 0) out vec4 frag_color;

void main() {
    const vec2 p = 2.0 * gl_PointCoord - 1.0;
    if (dot(p, p) > 1.0) discard;

    float dist = length(p);
    float alpha = 1.0 - smoothstep(0.8, 1.0, dist);

    frag_color = vec4(0.0, 0.5, 1.0, alpha);
}
