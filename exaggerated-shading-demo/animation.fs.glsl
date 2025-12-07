#version 460 core

struct material {
  vec4 ambient;
  vec4 diffuse;
  vec4 specular;
  uint albedo_map;
};

layout (std430, binding = 3) readonly
buffer material_data {
  material materials[];
};

uniform sampler2D textures[32];

uniform uint material_index = 0;

in vec2 tuv;
in vec3 nor;
in vec3 vnor;
noperspective in vec3 edge_distance;

layout (location = 0) out vec4 frag_color;

void main() {
  const material mat = materials[material_index];

  // float s = abs(normalize(vnor).z); // vertex normal -> smooth shading
  float s = abs(normalize(nor).z);  // face normal -> flat shading
  // float light = 0.2 + 1.0 * pow(s, 1000) + 0.75 * pow(s, 0.2);
  // Toon Shading
  // if (light <= 0.50) light = 0.20;
  // else if (light <= 0.60) light = 0.40;
  // else if (light <= 0.80) light = 0.60;
  // else if (light <= 0.90) light = 0.80;
  // else if (light <= 1.00) light = 1.00;

  // vec4 light_color = vec4(vec3(light), 1.0);

  vec4 albedo = mat.diffuse;
  if (mat.albedo_map > 0) albedo = texture(textures[mat.albedo_map], tuv);
  // if (mat.albedo_map == 1) albedo = vec4(1,0,0,1);
  // albedo = texture(textures[3], tuv);
  vec4 light_color = mat.ambient + s * albedo + pow(s, 1000) * mat.specular;

  frag_color = light_color; // No Wireframe Shading

  // Wireframe Shading
  // float d = min(edge_distance.x, edge_distance.y);
  // d = min(d, edge_distance.z);
  // float line_width = 0.5;
  // float line_delta = 0.25;
  // float alpha = 1.0;
  // vec4 line_color = vec4(vec3(0.5), alpha);
  // float mix_value =
  //     smoothstep(line_width - line_delta, line_width + line_delta, d);
  // frag_color = mix(line_color, light_color, mix_value);
}
