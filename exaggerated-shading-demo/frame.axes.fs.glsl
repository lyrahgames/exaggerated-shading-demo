#version 460

flat in vec4 col;
in float v_lineDist;
// flat in int id;

layout (location = 0) out vec4 frag_color;

void main() {
  // frag_color = col;
  // frag_color = vec4(vec3(0.0), 1.0);

  float d = abs(v_lineDist);

  // 1. Define the zones
  float coreLimit = 1.0;  // The solid part of the line
  float haloLimit = 1.0;  // How far the glow extends

  // 2. Calculate Core Mask (Sharp edge)
  float coreMask = 1.0 - smoothstep(coreLimit - 0.05, coreLimit, d);

  // 3. Calculate Halo Mask (Soft gradient)
  // We use a power function to make the glow look more natural
  float haloMask = 1.0 - smoothstep(coreLimit, haloLimit, d);
  haloMask = pow(haloMask, 2.0); // Adjust exponent for "hardness" of glow

  // 4. Combine colors
  vec4 finalColor = mix(vec4(1.0), col, coreMask);
  float finalAlpha = max(coreMask, haloMask * 0.5); // Halo is semi-transparent

  // if (finalAlpha < 0.01) discard;

  frag_color = vec4(finalColor.rgb, finalColor.a * finalAlpha);
}
