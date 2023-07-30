#version 330 core

layout (location = 0) in vec2 attrPosition;
layout (location = 1) in vec2 attrTexCoords;
out vec2 texCoords;

uniform mat4 model;
uniform mat4 projection;

void main()
{
	texCoords = attrTexCoords;
	gl_Position = projection * model * vec4(attrPosition, 0.0, 1.0);
}