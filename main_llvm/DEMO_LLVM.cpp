#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include <iostream>
//#include "DEMO_interface.h"

#define GAS_SIZE 128
#define V_MAX 16
#define SLOWDOWN 4 // (V_MAX - SLOWDOWN) / V_MAX
#define SCALE 8
#define SIZE_X SCALE *DEMO_SIZE_X
#define SIZE_Y SCALE *DEMO_SIZE_Y

typedef struct
{
  int32_t x;
  int32_t y;
  int32_t vx;
  int32_t vy;
} mol_t;

mol_t LLVM_gas[GAS_SIZE];

void LLVM_init_gas()
{
  std::cout <<"asssss" <<std::endl;
}

uint32_t LLVM_get_mol_color(uint32_t vx, uint32_t vy, int32_t v2max)
{
  uint32_t v = vx * vx + vy * vy;
  uint32_t red = 0xff * v / v2max;
  uint32_t blue = 0xff - red;
  return 0xff000000 + (red << 16) + blue;
}

void LLVM_draw_gas()
{
  int32_t v2max = 0;
  for (uint32_t i = 0; i < GAS_SIZE; i++)
  {
    if (v2max <
        LLVM_gas[i].vx * LLVM_gas[i].vx + LLVM_gas[i].vy * LLVM_gas[i].vy)
    {
      v2max = LLVM_gas[i].vx * LLVM_gas[i].vx + LLVM_gas[i].vy * LLVM_gas[i].vy;
    }
  }
}

void LLVM_step_gas()
{
  for (uint32_t i = 0; i < GAS_SIZE; i++)
  {
    LLVM_gas[i].x += LLVM_gas[i].vx;
    if (LLVM_gas[i].x < 0)
    {
      LLVM_gas[i].x = -LLVM_gas[i].x;
      LLVM_gas[i].vx = -LLVM_gas[i].vx;
    }
    LLVM_gas[i].y += LLVM_gas[i].vy;
    if (LLVM_gas[i].y < 0)
    {
      LLVM_gas[i].y = -LLVM_gas[i].y;
      LLVM_gas[i].vy = -LLVM_gas[i].vy;
    }
  }
}

int main()
{
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  llvm::LLVMContext context;
  // ; ModuleID = 'top'
  // source_filename = "top"
  llvm::Module *module = new llvm::Module("top", context);
  llvm::IRBuilder<> builder(context);

  // declare void @main()
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(builder.getInt32Ty(), false);
  llvm::Function *mainFunc = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, "main", module);

  // entrypoint:
  llvm::BasicBlock *entry =
      llvm::BasicBlock::Create(context, "entrypoint", mainFunc);
  builder.SetInsertPoint(entry);

  // Simple Run LLVM_main
  /*llvm::Function *CalleeF =
      llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(),
                                                     llvm::ArrayRef<llvm::Type *>(builder.getVoidTy()),
                                                     false),
                             llvm::Function::ExternalLinkage, "LLVM_main",
                             module);
  builder.CreateCall(CalleeF);

  // Interpreter of LLVM IR
  llvm::ExecutionEngine *ee =
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module)).create();
  ee->InstallLazyFunctionCreator([&](const std::string &fnName) -> void *
                                 {
        if (fnName == "LLVM_main") { return reinterpret_cast<void *>(LLVM_main);
     } return nullptr; });

  ee->finalizeObject();
  std::vector<llvm::GenericValue> noargs;
  llvm::GenericValue v = ee->runFunction(mainFunc, noargs);

  return 0;*/

  // Build LLVM_main function
  // https://godbolt.org/z/Y4bo6M5v9
  llvm::Function *Func_init_gas = llvm::Function::Create(
      llvm::FunctionType::get(builder.getVoidTy(),
                              llvm::ArrayRef<llvm::Type *>(builder.getVoidTy()),
                              false),
      llvm::Function::ExternalLinkage, "LLVM_init_gas", module);
  llvm::Function *Func_draw_gas = llvm::Function::Create(
      llvm::FunctionType::get(builder.getVoidTy(),
                              llvm::ArrayRef<llvm::Type *>(builder.getVoidTy()),
                              false),
      llvm::Function::ExternalLinkage, "LLVM_draw_gas", module);
  llvm::Function *Func_step_gas = llvm::Function::Create(
      llvm::FunctionType::get(builder.getVoidTy(),
                              llvm::ArrayRef<llvm::Type *>(builder.getVoidTy()),
                              false),
      llvm::Function::ExternalLinkage, "LLVM_step_gas", module);

  builder.CreateCall(Func_init_gas);
  builder.CreateRet(llvm::ConstantInt::get(builder.getInt32Ty(), 0, true));

  // Interpreter of LLVM IR

  llvm::ExecutionEngine *ee =
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module)).create();
  ee->InstallLazyFunctionCreator([&](const std::string &fnName) -> void *
                                 {
    if (fnName == "LLVM_init_gas") {
      return reinterpret_cast<void *>(LLVM_init_gas);
    }
    if (fnName == "LLVM_draw_gas") {
      return reinterpret_cast<void *>(LLVM_draw_gas);
    }
    if (fnName == "LLVM_step_gas") {
      return reinterpret_cast<void *>(LLVM_step_gas);
    }
    return nullptr; });
  ee->finalizeObject();
  std::vector<llvm::GenericValue> noargs;
  llvm::GenericValue v = ee->runFunction(mainFunc, noargs);

  return 0;
}
