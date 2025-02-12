#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=1, binding = 1) uniform sampler2D texSampler;

layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec4 Tx = texture(texSampler, fragTexCoord);
	outColor = Tx;
}