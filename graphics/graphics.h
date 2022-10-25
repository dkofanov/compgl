#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

void GR_Initialize(size_t w_width, size_t w_height, size_t scale);
void GR_PutPixel(size_t x, size_t y);
void GR_Flush();
void GR_Destroy();

#endif  // GRAPHICS_H

