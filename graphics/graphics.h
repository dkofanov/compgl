#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void GR_Initialize(size_t w_width, size_t w_height, size_t scale);
EXTERNC void GR_PutPixel(size_t x, size_t y);
EXTERNC void GR_Flush();
EXTERNC void GR_Destroy();

#endif  // GRAPHICS_H

