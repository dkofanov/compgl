#ifndef ENGINE_H
#define ENGINE_H

#include <stddef.h>

typedef char pxl_t;

struct EngineData
{
    char *world_;
    char *world_mirror_;
    size_t w_w_;
    size_t w_h_;

} ENG_DATA;

void ENG_Check() {
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

int ENG_FillRandom(unsigned int seed)
{
    ENG_Check();
    srand(seed);
    for (size_t i = 0; i < ENG_DATA.w_h_ * ENG_DATA.w_w_; i++) {
        ENG_DATA.world_[i] = ((rand() % 10) <= 1);
    }
    return 0;
}

int ENG_IterateWorld(void (*callback)(char *data, size_t x, size_t y))
{
    ENG_Check();
    for (size_t i = 0; i < ENG_DATA.w_h_ * ENG_DATA.w_w_; i++) {
        callback(&ENG_DATA.world_[i], (i % ENG_DATA.w_w_), (i / ENG_DATA.w_w_));
    }
    return 0;
}

char ENG_IsBoundary(size_t x, size_t y)
{
    char is_boundary = (x == 0) || (y == 0) || (x == (ENG_DATA.w_w_ - 1))
                                || (y == (ENG_DATA.w_h_ - 1));
    return is_boundary;
}

void ENG_SetMirrorCell(char data, size_t x, size_t y)
{
    ENG_DATA.world_mirror_[y * ENG_DATA.w_w_ + x] = data;
}
char ENG_GetCell(size_t x, size_t y)
{
    return ENG_DATA.world_[y * ENG_DATA.w_w_ + x];
}

void ENG_SwapBuffers()
{
    char *tmp = ENG_DATA.world_;
    ENG_DATA.world_ = ENG_DATA.world_mirror_;
    ENG_DATA.world_mirror_ = tmp;
}

int ENG_ClearMirror()
{
    ENG_Check();
    for (size_t i = 0; i < ENG_DATA.w_h_ * ENG_DATA.w_w_; i++) {
        ENG_DATA.world_mirror_[i] = 0;
    }
    return 0;
}


void ENG_DestroyWorld()
{
    ENG_DATA.world_ = NULL;
    ENG_DATA.world_mirror_ = NULL;
    ENG_DATA.w_w_ = 0;
    ENG_DATA.w_h_ = 0;
}

#endif  // ENGINE_H
