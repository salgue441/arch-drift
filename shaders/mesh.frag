#version 450

layout(location = 0) in vec3 vWorldNormal;
layout(location = 1) in vec3 vColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 n = normalize(vWorldNormal);
    vec3 light_dir = normalize(vec3(0.35, 0.90, 0.25));
    float ndotl = max(dot(n, light_dir), 0.0);
    float ambient = 0.28;
    float lit = ambient + (1.0 - ambient) * ndotl;
    outColor = vec4(vColor * lit, 1.0);
}
