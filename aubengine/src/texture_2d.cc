#include <iostream>

#include "aubengine/texture_2d.h"

Texture2D::Texture2D(GladGLContext* context)
    : Width(0),
      Height(0),
      Internal_Format(GL_RGB),
      Image_Format(GL_RGB),
      Wrap_S(GL_REPEAT),
      Wrap_T(GL_REPEAT),
      Filter_Min(GL_LINEAR),
      Filter_Max(GL_LINEAR) {
  _context = context;
  _context->GenTextures(1, &this->ID);
}

void Texture2D::Generate(unsigned int width, unsigned int height,
                         unsigned char* data) {
  this->Width = width;
  this->Height = height;
  // create Texture
  _context->BindTexture(GL_TEXTURE_2D, this->ID);
  _context->TexImage2D(GL_TEXTURE_2D, 0, this->Internal_Format, width, height,
                       0, this->Image_Format, GL_UNSIGNED_BYTE, data);
  // set Texture wrap and filter modes
  _context->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, this->Wrap_S);
  _context->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, this->Wrap_T);
  _context->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          this->Filter_Min);
  _context->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                          this->Filter_Max);
  // unbind texture
  _context->BindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::Bind() const { _context->BindTexture(GL_TEXTURE_2D, this->ID); }