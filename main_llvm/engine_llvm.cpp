#include "engine_llvm.h"

EngineData ENG_DATA;

void ENG_FillRandom(unsigned int seed)
{
    ENG_Check();
    srand(seed);
    for (size_t i = 0; i < ENG_DATA.w_h_ * ENG_DATA.w_w_; i++) {
        ENG_DATA.world_[i] = ((rand() % 10) <= 1);
    }
}

void ENG_IterateWorld(void (*callback)(char *data, size_t x, size_t y))
{
    ENG_Check();
    for (size_t i = 0; i < ENG_DATA.w_h_ * ENG_DATA.w_w_; i++) {
        callback(&ENG_DATA.world_[i], (i % ENG_DATA.w_w_), (i / ENG_DATA.w_w_));
    }
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

void CallbackDraw(char *data, size_t x, size_t y)
{
    if (*data) {
        GR_PutPixel(x, y);
    }
}

void CallbackFillMirror(char *data, size_t x, size_t y)
{
    if (ENG_IsBoundary(x, y)) {
        ENG_SetMirrorCell(1, x, y);
    } else {
        ENG_SetMirrorCell(0, x, y);
    }
}

void CallbackCalc(char *data, size_t x, size_t y)
{
    // Boundaries:
    if (ENG_IsBoundary(x, y)) {
        ENG_SetMirrorCell(0, x, y);
        return;
    }
    // Count neighbours:
    size_t xmin = x - 1, xmax = x + 1, ymin = y - 1, ymax = y + 1;
    size_t count_alives = 0;
    for (size_t i = 0; i < 3; i++) {
        if (ENG_GetCell(x - 1 + i, ymax)) {
            count_alives++;
        }
    }
    for (size_t i = 0; i < 3; i++) {
        if (ENG_GetCell(x - 1 + i, ymin)) {
            count_alives++;
        }
    }
    if (ENG_GetCell(xmin, y)) {
        count_alives++;
    }
    if (ENG_GetCell(xmax, y)) {
        count_alives++;
    }

    // Set Result
    if (!ENG_GetCell(x, y)) {
        if (count_alives == 3) {
            ENG_SetMirrorCell(1, x, y);
        } else {
            ENG_SetMirrorCell(0, x, y);
        }
    } else {
        if (count_alives == 3 || count_alives == 2) {
            ENG_SetMirrorCell(1, x, y);
        } else {
            ENG_SetMirrorCell(0, x, y);
        }
    }
}
