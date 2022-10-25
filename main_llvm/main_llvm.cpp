#include "graphics.h"
#include "engine_llvm.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
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
        // declare ENG struct:
        llvm::StructType *eng_ty = llvm::StructType::create(GetArgsType({PTR_T, PTR_T, I64_T, I64_T}));
        llvm::GlobalVariable eng(eng_ty, false, llvm::GlobalVariable::ExternalLinkage);

        // declare void @main()
        main_ = GetFunction(I32_T, {}, "main");

        // entrypoint:
        auto *entry = llvm::BasicBlock::Create(context_, "entrypoint", main_);
        builder_.SetInsertPoint(entry);


        auto wrld_buf = builder_.CreateAlloca(I8_T, I64(25000));
        auto wrld_mirror_buf = builder_.CreateAlloca(I8_T, I64(25000));

        auto gl_init_args = GetArgs({I64(1250), I64(500), I64(5)});
        auto gl_init = GetFunction(VOID_T, {I64_T, I64_T, I64_T}, "GR_Initialize");
        builder_.CreateCall(gl_init, gl_init_args);

        builder_.CreateLifetimeStart(wrld_buf, I64(25000));
        builder_.CreateMemSet(wrld_buf, I8(0), I64(25000), llvm::MaybeAlign(16));
        builder_.CreateLifetimeStart(wrld_mirror_buf, I64(25000));
        builder_.CreateMemSet(wrld_mirror_buf, I8(0), I64(25000), llvm::MaybeAlign(16));
        builder_.CreateStore(wrld_buf, &eng);
        auto eng_2 = builder_.CreateInBoundsGEP(&eng, GetArgs({I64(0), I32(1)}));
        auto eng_3 = builder_.CreateInBoundsGEP(&eng, GetArgs({I64(0), I32(2)}));
        auto eng_4 = builder_.CreateInBoundsGEP(&eng, GetArgs({I64(0), I32(3)}));
        builder_.CreateStore(wrld_mirror_buf, eng_2);
        builder_.CreateStore(I64(250), eng_3);
        builder_.CreateStore(I64(100), eng_4);

        auto eng_fill_rand_args = GetArgs({I32(222)});
        auto eng_fill_rand = GetFunction(VOID_T, {I32_T}, "ENG_FillRandom");
        builder_.CreateCall(eng_fill_rand, eng_fill_rand_args);

        llvm::BasicBlock *loop = llvm::BasicBlock::Create(context_, "loop", main_);
        builder_.CreateBr(loop);

        auto callb_ty = GetFunctionType(VOID_T, GetArgsType({PTR_T, I64_T, I64_T}));
        auto callb_draw = GetFunction(VOID_T, {PTR_T, I64_T, I64_T}, "CallbackDraw");
        auto callb_calc = GetFunction(VOID_T, {PTR_T, I64_T, I64_T}, "CallbackCalc");
        auto iter_wrld = GetFunction(VOID_T, {callb_ty}, "ENG_IterateWorld");
        auto swap = GetFunction(VOID_T, {}, "ENG_SwapBuffers");
        auto gl_flush = GetFunction(VOID_T, {}, "GR_Flush");
        auto sleep = GetFunction(I32_T, {I32_T}, "usleep");
        builder_.SetInsertPoint(loop);
            // builder_.CreatePHI(I64_T, 2);
            builder_.CreateCall(iter_wrld, callb_calc);
            builder_.CreateCall(iter_wrld, callb_draw);
            builder_.CreateCall(swap);
            builder_.CreateCall(gl_flush);
            builder_.CreateCall(sleep);
            builder_.CreateBr(loop);


        builder_.CreateRet(llvm::ConstantInt::get(I32_T, 0, true));
    }

    void LaunchEE() {
        llvm::ExecutionEngine *ee = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_)).create();
        ee->InstallLazyFunctionCreator([&](const std::string &fn) -> void *
            {
                if (fn == "ENG_FillRandom") { return reinterpret_cast<void *>(ENG_FillRandom); }
                if (fn == "GR_Initialize") { return reinterpret_cast<void *>(GR_Initialize); }
                if (fn == "CallbackCalc") { return reinterpret_cast<void *>(CallbackCalc); }
                if (fn == "CallbackDraw") { return reinterpret_cast<void *>(CallbackDraw); }
                if (fn == "ENG_IterateWorld") { return reinterpret_cast<void *>(ENG_IterateWorld); }
                if (fn == "GR_Flush") { return reinterpret_cast<void *>(GR_Flush); }
                if (fn == "usleep") { return reinterpret_cast<void *>(usleep); }
                std::cout << "NO_FUNC" << std::endl;
                return nullptr;
            });
        ee->finalizeObject();
        std::vector<llvm::GenericValue> noargs;
        llvm::GenericValue gv = ee->runFunction(main_, noargs);
    }

private:
    llvm::ArrayRef<llvm::Type *> GetArgsType(std::vector<llvm::Type *> vec)
    {
        return llvm::ArrayRef<llvm::Type *>(vec);
    }
    llvm::ArrayRef<llvm::Value *> GetArgs(std::vector<llvm::Value *> vec)
    {
        return llvm::ArrayRef<llvm::Value *>(vec);
    }
    llvm::FunctionType *GetFunctionType(llvm::Type *type, llvm::ArrayRef<llvm::Type *> args_ty)
    {
        return llvm::FunctionType::get(type, args_ty, false);
    }
    llvm::Function *GetFunction(llvm::FunctionType *type, std::string func_name)
    {
        return llvm::Function::Create(type, llvm::Function::ExternalLinkage, func_name, module_);
    }
    llvm::Function *GetFunction(llvm::Type *ret_ty, std::vector<llvm::Type *> args_ty, std::string func_name)
    {
        return llvm::Function::Create(GetFunctionType(ret_ty, GetArgsType(args_ty)), llvm::Function::ExternalLinkage, func_name, module_);
    }
    llvm::ConstantInt *I8(int8_t val)
    {
        return llvm::ConstantInt::get(builder_.getInt8Ty(), val, true);
    }
    llvm::ConstantInt *I32(int32_t val)
    {
        return llvm::ConstantInt::get(builder_.getInt32Ty(), val, true);
    }
    llvm::ConstantInt *I64(int64_t val)
    {
        return llvm::ConstantInt::get(builder_.getInt64Ty(), val, true);
    }
private:
    // LLVM:
    llvm::LLVMContext context_;
    llvm::IRBuilder<> builder_;
    llvm::Module *module_;
    llvm::Function *main_;
    const llvm::FunctionType *VOID_F_VOID = llvm::FunctionType::get(builder_.getVoidTy(), false);
    const llvm::FunctionType *I32_F_VOID = llvm::FunctionType::get(builder_.getInt32Ty(), false);
    llvm::Type *VOID_T = builder_.getVoidTy();
    llvm::Type *PTR_T = builder_.getInt8PtrTy();
    llvm::IntegerType *I8_T = builder_.getInt8Ty();
    llvm::IntegerType *I32_T = builder_.getInt32Ty();
    llvm::IntegerType *I64_T = builder_.getInt64Ty();
};

int main()
{
    IRGen irg;
    irg.CreateMain();
    irg.LaunchEE();   

    return 0;
}
