#include <iostream>

#include <glad/gl.h>
#include "Texture2D.h"



Texture2D::Texture2D()
	: Width(0), Height(0), Internal_Format(GL_RGB), Image_Format(GL_RGB), Wrap_S(GL_REPEAT), Wrap_T(GL_REPEAT), Filter_Min(GL_LINEAR), Filter_Max(GL_LINEAR)
{
	GladGLContext* context = gladGetGLContext();
	context->GenTextures(1, &this->ID);
}

void Texture2D::Generate(unsigned int width, unsigned int height, unsigned char* data)
{
	GladGLContext* context = gladGetGLContext();

	this->Width = width;
	this->Height = height;
	// create Texture
	context->BindTexture(GL_TEXTURE_2D, this->ID);
	context->TexImage2D(GL_TEXTURE_2D, 0, this->Internal_Format, width, height, 0, this->Image_Format, GL_UNSIGNED_BYTE, data);
	// set Texture wrap and filter modes
	context->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, this->Wrap_S);
	context->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, this->Wrap_T);
	context->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, this->Filter_Min);
	context->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, this->Filter_Max);
	// unbind texture
	context->BindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::Bind() const
{
	GladGLContext* context = gladGetGLContext();
	context->BindTexture(GL_TEXTURE_2D, this->ID);
}