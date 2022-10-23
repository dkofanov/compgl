#include "graphics.h"
#include "engine.h"

#define WIDTH 250
#define HEIGHT 100

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

int main()
{
    size_t scale = 5;

    GR_Initialize(WIDTH * scale, HEIGHT * scale, scale);

    ENG_InitializeWorld(WIDTH, HEIGHT);
    ENG_FillRandom(222);

    for (size_t i = 0; i < 24999; i += 1) {
        // Calc mirror (next step)
        ENG_IterateWorld(CallbackCalc);

        ENG_IterateWorld(CallbackDraw);
        ENG_SwapBuffers();
        GR_Flush();
        usleep(300000);
    }

    ENG_DestroyWorld();
    GR_Destroy();

    return 0;
}
