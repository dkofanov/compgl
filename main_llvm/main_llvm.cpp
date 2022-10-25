//#include "graphics.h"
//#include "engine_llvm.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include <iostream>

void test()
{
    std::cout << "Hello" <<std::endl;
}

class IRGen {
public:
    IRGen() : context_(), builder_(context_), module_(new llvm::Module("top", context_))
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
    }

    void CreateMain() {
        // declare void @main()
        auto *func_type = llvm::FunctionType::get(builder_.getInt32Ty(), false);
        main_ = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, "main", module_);

        // entrypoint:
        auto *entry = llvm::BasicBlock::Create(context_, "entrypoint", main_);
        builder_.SetInsertPoint(entry);
        llvm::Function *Func_init_gas = llvm::Function::Create(
            llvm::FunctionType::get(builder_.getVoidTy(),
                                llvm::ArrayRef<llvm::Type *>(builder_.getVoidTy()),
                                false),
            llvm::Function::ExternalLinkage,
            "test",
            module_);

        builder_.CreateCall(Func_init_gas);
        builder_.CreateRet(llvm::ConstantInt::get(builder_.getInt32Ty(), 0, true));
    }

    void LaunchEE() {
        llvm::ExecutionEngine *ee = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_)).create();
        ee->InstallLazyFunctionCreator([&](const std::string &fnName) -> void *
            {
                if (fnName == "test") {
                    return reinterpret_cast<void *>(test);
                }
                std::cout << "NO_FUNC" << std::endl;
                return nullptr;
            });
        ee->finalizeObject();
        std::vector<llvm::GenericValue> noargs;
        llvm::GenericValue gv = ee->runFunction(main_, noargs);
    }
private:
    llvm::LLVMContext context_;
    llvm::IRBuilder<> builder_;
    llvm::Module *module_;
    llvm::Function *main_;
    const llvm::FunctionType *FUNCTION_VOID_TYPE = llvm::FunctionType::get(builder_.getVoidTy(), false);
    const llvm::FunctionType *FUNCTION_I32_TYPE = llvm::FunctionType::get(builder_.getInt32Ty(), false);
};

int main()
{
    IRGen irg;
    irg.CreateMain();
    irg.LaunchEE();   

    return 0;
}
