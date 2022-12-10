#ifndef ENGINE_H
#define ENGINE_H

#include <stddef.h>
#include "graphics/graphics.h"


typedef char pxl_t;

struct EngineData
{
    char *world_;
    char *world_mirror_;
    size_t w_w_;
    size_t w_h_;
};

extern EngineData ENG_DATA;

static void ENG_Check() {
    if (ENG_DATA.world_ == NULL) {
        exit(-1);
    }
}

#define ENG_InitializeWorld(width, height) \
    pxl_t WORLD_BUF[width * height] = {};\
    pxl_t WORLD_BUF_MIRROR[width * height] = {};\
    ENG_DATA.world_ = WORLD_BUF;\
    ENG_DATA.world_mirror_ = WORLD_BUF_MIRROR;\
    ENG_DATA.w_w_ = width;\
    ENG_DATA.w_h_ = height;

void ENG_FillRandom(unsigned int seed);
void ENG_IterateWorld(void (*callback)(char *data, size_t x, size_t y));
char ENG_IsBoundary(size_t x, size_t y);
void ENG_SetMirrorCell(char data, size_t x, size_t y);
char ENG_GetCell(size_t x, size_t y);
void ENG_SwapBuffers();
int ENG_ClearMirror();
void ENG_DestroyWorld();
void CallbackDraw(char *data, size_t x, size_t y);
void CallbackFillMirror(char *data, size_t x, size_t y);
void CallbackCalc(char *data, size_t x, size_t y);

#endif  // ENGINE_H
