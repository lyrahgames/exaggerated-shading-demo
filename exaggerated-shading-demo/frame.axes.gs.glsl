#version 460 core

layout (lines) in;                               // Takes 2 vertices
layout (triangle_strip, max_vertices = 4) out;  // Outputs 4 vertices (1 quad)

uniform float u_thickness = 5.0;   // Thickness in pixels (or small NDC value)
// uniform float u_aspectRatio; // screen_width / screen_height

flat in vec4 color[];

flat out vec4 col;
out float v_lineDist; // -1.0 to 1.0 across the line width

void main() {
    // 1. Get clip space positions
    vec4 p1 = gl_in[0].gl_Position;
    vec4 p2 = gl_in[1].gl_Position;

    // 2. Convert to NDC (divide by W) to do 2D math
    vec2 ndc1 = p1.xy / p1.w;
    vec2 ndc2 = p2.xy / p2.w;

    // 3. Calculate the direction of the line in 2D
    vec2 dir = normalize(ndc2 - ndc1);

    // 4. Calculate the perpendicular (normal) vector
    // We adjust for aspect ratio so the line isn't skewed
    vec2 normal = vec2(-dir.y, dir.x);
    // normal.x /= u_aspectRatio;

    // 5. Calculate the offset based on thickness
    // We divide by 2.0 because we expand in both directions from the center
    vec2 offset = normal * (u_thickness / 1000.0);

    col = color[0];

    // Vertex 1: Bottom Left
    v_lineDist = -1.0;
    gl_Position = vec4((ndc1 - offset) * p1.w, p1.z, p1.w);
    EmitVertex();

    // Vertex 2: Top Left
    v_lineDist = 1.0;
    gl_Position = vec4((ndc1 + offset) * p1.w, p1.z, p1.w);
    EmitVertex();

    // Vertex 3: Bottom Right
    v_lineDist = -1.0;
    gl_Position = vec4((ndc2 - offset) * p2.w, p2.z, p2.w);
    EmitVertex();

    // Vertex 4: Top Right
    v_lineDist = 1.0;
    gl_Position = vec4((ndc2 + offset) * p2.w, p2.z, p2.w);
    EmitVertex();

    EndPrimitive();
}
