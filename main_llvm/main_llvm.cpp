#include "graphics.h"
#include "engine.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"

auto CreateLLVMIR()
{

static LLVMContext theContext;
static IRBuilder<> builder(theContext);
static std::unique_ptr<Module> theModule;
static Value* theMemory;
static Function* main_func;
static Function* printf_func;
static Function* scanf_func;
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
