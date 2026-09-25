#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(set = 0, binding = 0) uniform ObjectUBO {
    mat4 mvp;
    mat4 model;
    vec4 color_mul;
} ubo;

layout(location = 0) out vec3 vWorldNormal;
layout(location = 1) out vec3 vColor;

void main() {
    vWorldNormal = mat3(ubo.model) * inNormal;
    vColor = inColor * ubo.color_mul.rgb;
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
}
