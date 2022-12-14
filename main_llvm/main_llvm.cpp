#include "graphics/graphics.h"
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
        eng_ty_ = llvm::StructType::create(GetArgsType({PTR_T, PTR_T, I64_T, I64_T}), "EngineData");
        auto eng = static_cast<llvm::GlobalVariable *>(module_->getOrInsertGlobal("ENG_DATA", eng_ty_));
        eng->setLinkage(llvm::GlobalVariable::ExternalLinkage);
        //eng->setAlignment(llvm::Align(8));
    }

    void DeclareFunctions()
    {
        // declare void @main()
        main_ = GetFunction(VOID_T, GetArgsType({}), "main");
        GetFunction(VOID_T, GetArgsType({I64_T, I64_T, I64_T}), "GR_Initialize");
        auto eng_fill_rand = GetFunction(VOID_T, GetArgsType({I32_T}), "ENG_FillRandom");

        callb_ty_ = GetFunctionType(VOID_T, GetArgsType({PTR_T, I64_T, I64_T}));
        auto callb_draw = GetFunction(callb_ty_, "CallbackDraw");
        auto callb_calc = GetFunction(callb_ty_, "CallbackCalc");

        auto iter_wrld = GetFunction(VOID_T, GetArgsType({llvm::PointerType::getUnqual(callb_ty_)}), "ENG_IterateWorld");
        auto swap = GetFunction(VOID_T, GetArgsType({}), "ENG_SwapBuffers");
        auto gl_flush = GetFunction(VOID_T, GetArgsType({}), "GR_Flush");
        auto sleep = GetFunction(I32_T, GetArgsType({I32_T}), "usleep");

    }

    void CreateIR()
    {
        DeclareGlobals();
        DeclareFunctions();

        CreateMain();
        CreateIterateWorld();
        module_->print(llvm::outs(), nullptr);

    }

    void CreateIterateWorld()
    {
        // entrypoint:
        auto *entry = llvm::BasicBlock::Create(context_, "entrypoint", Func("ENG_IterateWorld"));
        __ SetInsertPoint(entry);
        llvm::GlobalVariable *eng = module_->getNamedGlobal("ENG_DATA");
        auto gep_0 =    __ CreateGEP(eng_ty_, eng, GetArgs({I64(0), I32(0)}));
        auto gep_2 =    __ CreateGEP(eng_ty_, eng, GetArgs({I64(0), I32(2)}));
        auto gep_3 =    __ CreateGEP(eng_ty_, eng, GetArgs({I64(0), I32(3)}));
        auto wrld =     __ CreateLoad(PTR_T, gep_0);
        auto eng_2 =    __ CreateLoad(I64_T, gep_2);
        auto eng_3 =    __ CreateLoad(I64_T, gep_3);
        wrld->setAlignment(llvm::Align(8));
        eng_2->setAlignment(llvm::Align(8));
        eng_3->setAlignment(llvm::Align(8));
        auto mul =      __ CreateMul(eng_2, eng_3);
        auto cmp =      __ CreateICmpEQ(mul, I64(0));
        auto *ret = llvm::BasicBlock::Create(context_, "ret", Func("ENG_IterateWorld"));
        auto *loop = llvm::BasicBlock::Create(context_, "loop", Func("ENG_IterateWorld"));
                        __ CreateCondBr(cmp, ret, loop);

        __ SetInsertPoint(ret);
            __ CreateRetVoid();

        __ SetInsertPoint(loop);
            auto phi_1 =        __ CreatePHI(I64_T, 2);
            auto phi_2 =        __ CreatePHI(I64_T, 2);

            auto urem =         __ CreateURem(phi_2, phi_1);
            auto udiv =         __ CreateUDiv(phi_2, phi_1);
            auto wrld_elem =    __ CreateAdd(wrld, phi_2);
                                __ CreateCall(callb_ty_, Func("ENG_IterateWorld")->getArg(0), GetArgs({wrld_elem, urem, udiv}));
            auto add =          __ CreateAdd(phi_2, I64(1));

            auto loop_eng_2 =   __ CreateLoad(I64_T, gep_2);
            auto loop_eng_3 =   __ CreateLoad(I64_T, gep_3);
            auto loop_mul =     __ CreateMul(loop_eng_2, loop_eng_3);
            auto loop_cmp =     __ CreateICmpULT(add, loop_mul);
                                __ CreateCondBr(loop_cmp, loop, ret);

            phi_1->addIncoming(eng_2, entry);
            phi_1->addIncoming(loop_eng_2, loop);
            
            phi_2->addIncoming(I64(0), entry);
            phi_2->addIncoming(add, loop);
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
        auto eng_0 = __ CreateGEP(eng_ty_, eng, GetArgs({I64(0), I32(0)}));
        auto eng_1 = __ CreateGEP(eng_ty_, eng, GetArgs({I64(0), I32(1)}));
        auto eng_2 = __ CreateGEP(eng_ty_, eng, GetArgs({I64(0), I32(2)}));
        eng->setAlignment(llvm::Align(8));
        auto eng_3 = __ CreateGEP(eng_ty_, eng, GetArgs({I64(0), I32(3)}));
        __ CreateStore(wrld_buf, eng_0);
        __ CreateStore(wrld_mirror_buf, eng_1);
        __ CreateStore(I64(250), eng_2);
        __ CreateStore(I64(100), eng_3);

        __ CreateCall(Func("ENG_FillRandom"), I32(222));

        llvm::BasicBlock *loop = llvm::BasicBlock::Create(context_, "loop", main_);
        __ CreateBr(loop);

        __ SetInsertPoint(loop);
            __ CreateCall(Func("ENG_IterateWorld"),  GetArgs({Func("CallbackCalc")}));
            __ CreateCall(Func("ENG_IterateWorld"),  GetArgs({Func("CallbackDraw")}));
            __ CreateCall(Func("ENG_SwapBuffers"));
            __ CreateCall(Func("GR_Flush"));
            __ CreateCall(Func("usleep"), I32(16000));  // 16000us -> ~50 fps
            __ CreateBr(loop);

    }

    void LaunchEE() {
        llvm::ExecutionEngine *ee = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_)).create();
        ee->InstallLazyFunctionCreator([&](const std::string &fn) -> void *
            {
                //if (fn == "usleep") { return reinterpret_cast<void *>(usleep); }
                if (fn == "GR_Flush") { return reinterpret_cast<void *>(GR_Flush); }
                if (fn == "GR_Initialize") { return reinterpret_cast<void *>(GR_Initialize); }

                //if (fn == "ENG_FillRandom") { return reinterpret_cast<void *>(ENG_FillRandom); }
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

    llvm::FunctionType *GetFunctionType(llvm::Type *type, const std::vector<llvm::Type *> &args_ty)
    {
        return llvm::FunctionType::get(type, GetArgsType(args_ty), false);
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

    llvm::Function *Func(const std::string &fn)
    {
        return module_->getFunction(fn);
    }
#undef __

private:
    // LLVM:
    llvm::LLVMContext context_;
    llvm::IRBuilder<> builder_;
    llvm::Module *module_{};
    llvm::Function *main_{};
    llvm::StructType *eng_ty_{};
    llvm::FunctionType *callb_ty_{};

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
