#version 450

layout(set=0, binding = 0) uniform GlobalUniformBufferObject {
	mat4 view;
	mat4 proj;
} gubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 colour;
layout(location = 4) in vec3 tangent;

layout(location = 2) out vec2 fragTexCoord;

void main() {
	gl_Position = gubo.proj * gubo.view  * vec4(pos, 1.0);
	fragTexCoord = pos;
}