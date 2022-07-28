#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set=0, binding = 0) uniform GlobalUniformBufferObject {
	mat4 view;
	mat4 proj;
} gubo;

layout(set=1, binding = 0) uniform UniformBufferObject {
	mat4 model;
} ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 texCoord;

layout(location = 2) out vec2 fragTexCoord;

void main() {
	vec3 newPos = pos;
	if (ubo.model != mat4(1.0)){
		newPos.z = -1.0f;
	}
	gl_Position = vec4(newPos, 1.0);
	fragTexCoord = texCoord;
}