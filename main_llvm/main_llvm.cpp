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



class IRGen {
public:
#define __ builder_. 
#define VOID_T __ getVoidTy()
#define PTR_T __ getInt8PtrTy()
#define I8_T __ getInt8Ty()
#define I32_T __ getInt32Ty()
#define I64_T __ getInt64Ty()


    IRGen() : context_(), builder_(context_), module_(new llvm::Module("top", context_))
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
    }

    void DeclareGlobals()
    {
        // declare ENG struct:
        llvm::StructType *eng_ty = llvm::StructType::create(context_, GetArgsType({PTR_T, PTR_T, I64_T, I64_T}));
        static_cast<llvm::GlobalVariable *>(module_->getOrInsertGlobal("ENG_DATA", eng_ty))->setLinkage(llvm::GlobalVariable::ExternalLinkage);;
    }

    void DeclareFunctions()
    {
        // declare void @main()
        main_ = GetFunction(I32_T, GetArgsType({}), "main");
        GetFunction(VOID_T, GetArgsType({I64_T, I64_T, I64_T}), "GR_Initialize");
        auto eng_fill_rand = GetFunction(VOID_T, {I32_T}, "ENG_FillRandom");

        auto callb_ty = GetFunctionType(VOID_T, GetArgsType({PTR_T, I64_T, I64_T}));
        auto callb_draw = GetFunction(callb_ty, "CallbackDraw");
        auto callb_calc = GetFunction(callb_ty, "CallbackCalc");

        auto iter_wrld = GetFunction(VOID_T, GetArgsType({callb_ty}), "ENG_IterateWorld");
        auto swap = GetFunction(VOID_T, GetArgsType({}), "ENG_SwapBuffers");
        auto gl_flush = GetFunction(VOID_T, GetArgsType({}), "GR_Flush");
        auto sleep = GetFunction(I32_T, GetArgsType({I32_T}), "usleep");

    }

    void CreateIR()
    {
        DeclareGlobals();
        DeclareFunctions();

        CreateMain();
        //CreateIterateWorld();
    }

    void CreateIterateWorld()
    {
        // entrypoint:
        auto *entry = llvm::BasicBlock::Create(context_, "entrypoint", Func("ENG_IterateWorld"));
        __ SetInsertPoint(entry);
        llvm::GlobalVariable *eng = module_->getNamedGlobal("ENG_DATA");
        auto gep_2 = __ CreateGEP(eng, GetArgs({I64(0), I32(2)}));
        auto gep_3 = __ CreateGEP(eng, GetArgs({I64(0), I32(3)}));
        auto eng_2 = __ CreateLoad(gep_2);
        auto eng_3 = __ CreateLoad(gep_3);
        auto mul = __ CreateMul(eng_2, eng_3);
        auto 
    }

    void CreateMain()
    {
        llvm::GlobalVariable *eng = module_->getNamedGlobal("ENG_DATA");

        // entrypoint:
        auto *entry = llvm::BasicBlock::Create(context_, "entrypoint", main_);
        __ SetInsertPoint(entry);

        auto wrld_buf = __ CreateAlloca(I8_T, I64(25000));
        auto wrld_mirror_buf = __ CreateAlloca(I8_T, I64(25000));

        __ CreateCall(Func("GR_Initialize"), GetArgs({I64(1250), I64(500), I64(5)}));

        __ CreateLifetimeStart(wrld_buf, I64(25000));
        __ CreateMemSet(wrld_buf, I8(0), I64(25000), llvm::MaybeAlign(16));
        __ CreateLifetimeStart(wrld_mirror_buf, I64(25000));
        __ CreateMemSet(wrld_mirror_buf, I8(0), I64(25000), llvm::MaybeAlign(16));
        __ CreateStore(wrld_buf, eng);
        auto eng_1 = __ CreateGEP(eng, GetArgs({I64(0), I32(1)}));
        auto eng_2 = __ CreateGEP(eng, GetArgs({I64(0), I32(2)}));
        auto eng_3 = __ CreateGEP(eng, GetArgs({I64(0), I32(3)}));
        __ CreateStore(wrld_mirror_buf, eng_1);
        __ CreateStore(I64(250), eng_2);
        __ CreateStore(I64(100), eng_3);

        __ CreateCall(Func("ENG_FillRandom"), I32(222));

        llvm::BasicBlock *loop = llvm::BasicBlock::Create(context_, "loop", main_);
        __ CreateBr(loop);

        __ SetInsertPoint(loop);
            __ CreateCall(Func("ENG_IterateWorld"), Func("CallbackCalc"));
            __ CreateCall(Func("ENG_IterateWorld"), Func("CallbackDraw"));
            __ CreateCall(Func("ENG_SwapBuffers"));
            __ CreateCall(Func("GR_Flush"));
            __ CreateCall(Func("usleep"), I32(4000));
            __ CreateBr(loop);

        __ CreateRet(llvm::ConstantInt::get(I32_T, 0, true));
        module_->print(llvm::outs(), nullptr);
    }

    void LaunchEE() {
        llvm::ExecutionEngine *ee = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_)).create();
        ee->InstallLazyFunctionCreator([&](const std::string &fn) -> void *
            {
                if (fn == "usleep") { return reinterpret_cast<void *>(usleep); }
                if (fn == "GR_Flush") { return reinterpret_cast<void *>(GR_Flush); }
                if (fn == "GR_Initialize") { return reinterpret_cast<void *>(GR_Initialize); }

                if (fn == "ENG_IterateWorld") { return reinterpret_cast<void *>(ENG_IterateWorld); }
                if (fn == "ENG_FillRandom") { return reinterpret_cast<void *>(ENG_FillRandom); }
                if (fn == "CallbackCalc") { return reinterpret_cast<void *>(CallbackCalc); }
                if (fn == "CallbackDraw") { return reinterpret_cast<void *>(CallbackDraw); }
                if (fn == "ENG_SwapBuffers") { return reinterpret_cast<void *>(ENG_SwapBuffers); }

                std::cerr << "NO_FUNC " << fn << std::endl;
                return nullptr;
            });
        ee->finalizeObject();
        std::vector<llvm::GenericValue> noargs;
        llvm::GenericValue gv = ee->runFunction(main_, noargs);
    }

private:
    llvm::ArrayRef<llvm::Type *> GetArgsType(const std::vector<llvm::Type *> &vec)
    {
        storage_.func_args_types_.push_back(vec);
        return llvm::ArrayRef<llvm::Type *>(storage_.func_args_types_.back());
    }
    llvm::ArrayRef<llvm::Value *> GetArgs(const std::vector<llvm::Value *> &vec)
    {
        storage_.func_args_.push_back(vec);
        return llvm::ArrayRef<llvm::Value *>(storage_.func_args_.back());
    }
    llvm::FunctionType *GetFunctionType(llvm::Type *type, llvm::ArrayRef<llvm::Type *> args_ty)
    {
        return llvm::FunctionType::get(type, args_ty, false);
    }
    llvm::Function *GetFunction(llvm::FunctionType *type, std::string func_name)
    {
        return llvm::Function::Create(type, llvm::Function::ExternalLinkage, func_name, module_);
    }
    llvm::Function *GetFunction(llvm::Type *ret_ty, const std::vector<llvm::Type *> &args_ty, std::string func_name)
    {
        return llvm::Function::Create(GetFunctionType(ret_ty, GetArgsType(args_ty)), llvm::Function::ExternalLinkage, func_name, module_);
    }
    llvm::ConstantInt *I8(int8_t val)
    {
        return llvm::ConstantInt::get(__ getInt8Ty(), val, true);
    }
    llvm::ConstantInt *I32(int32_t val)
    {
        return llvm::ConstantInt::get(__ getInt32Ty(), val, true);
    }
    llvm::ConstantInt *I64(int64_t val)
    {
        return llvm::ConstantInt::get(__ getInt64Ty(), val, true);
    }
    llvm::Function *Func(const std::string &fn) {
        return module_->getFunction(fn);
    }
#undef __

private:
    // LLVM:
    llvm::LLVMContext context_;
    llvm::IRBuilder<> builder_;
    llvm::Module *module_;
    llvm::Function *main_;

    struct {
        std::vector<std::vector<llvm::Type *>> func_args_types_;
        std::vector<std::vector<llvm::Value *>> func_args_;
        std::vector<llvm::Value *> values_;
        std::vector<llvm::Type *> types_;
    } storage_;
};

int main()
{
    IRGen irg;
    irg.CreateIR();
    irg.LaunchEE();

    return 0;
}
