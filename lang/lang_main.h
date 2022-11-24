#include "lexer_decls.h"

#include "../common/macro.h"
#include "token_processor.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/Support/TargetSelect.h"
#include <iostream>
#include <unistd.h>
#include <unordered_map>
#include <stack>

class IRGenBase {
public:
    using token_t = Token;
    using tokcr_t = const token_t &;
    using Value = token_t::Value;
    using Type = token_t::Type;
    using Op = token_t::Op;
    using Id = token_t::Id;
    using BB = token_t::BB;
    using Num = token_t::Num;
    using TypedRef = std::pair<Type, Value>;

public:
    using HashTypes = std::unordered_map<std::string, Type>;
    using HashTypedReferences = std::unordered_map<std::string, TypedRef>;

#define __ builder_. 
#define VOID_T __ getVoidTy()
#define PTR_T __ getInt8PtrTy()
#define I8_T __ getInt8Ty()
#define I32_T __ getInt32Ty()
#define I64_T __ getInt64Ty()

    IRGenBase() : context_(), builder_(context_), module_(new llvm::Module("top", context_))
    {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
    }

    void DeclareGlobals()
    {

        // declare ENG struct:
        eng_ty_ = llvm::StructType::create(CreateArgsType({PTR_T, PTR_T, I64_T, I64_T}), "EngineData");
        auto eng = static_cast<llvm::GlobalVariable *>(module_->getOrInsertGlobal("ENG_DATA", eng_ty_));
        eng->setLinkage(llvm::GlobalVariable::ExternalLinkage);
        //eng->setAlignment(llvm::Align(8));
    }
    
    void DeclarePrimitiveTypes()
    {
        ASSERT(declared_types_.empty());
        declared_types_[std::string("Num")] = I64_T;
    }

    Type ResolveTypeByName(const std::string &type) {
        return declared_types_[type];
    }

    void LaunchEE() {
        llvm::ExecutionEngine *ee = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_)).create();
        ee->InstallLazyFunctionCreator([&](const std::string &fn) -> void *
            {
                if (fn == "PrintNum") {return reinterpret_cast<void *>(PrintNum);}
                std::cerr << "NO_FUNC " << fn << std::endl;
                return nullptr;
            });
        ee->finalizeObject();
        std::vector<llvm::GenericValue> noargs;
        llvm::GenericValue gv = ee->runFunction(main_, noargs);
    }
    static void PrintNum(void *ptr, char *name) {
        printf("0x");
        for (size_t i = 0; i < sizeof(token_t::Num); i++) {
            printf("%02hhX", *(static_cast<char *>(ptr) + i));
        }
        std::cout << " (" << name << " = " << *(static_cast<token_t::Num *>(ptr)) << ')'  << std::endl;
    }
protected:
    // Storage:
    llvm::ArrayRef<llvm::Type *> CreateArgsType(const std::vector<llvm::Type *> &vec)
    {
        storage_.func_args_types_.push_back(vec);
        return llvm::ArrayRef<llvm::Type *>(storage_.func_args_types_.back());
    }

    llvm::ArrayRef<llvm::Value *> CreateArgs(const std::vector<llvm::Value *> &vec)
    {
        storage_.func_args_.push_back(vec);
        return llvm::ArrayRef<llvm::Value *>(storage_.func_args_.back());
    }

    llvm::FunctionType *GetFunctionType(llvm::Type *type, const std::vector<llvm::Type *> &args_ty)
    {
        return llvm::FunctionType::get(type, CreateArgsType(args_ty), false);
    }

    llvm::Function *CreateFunction(llvm::FunctionType *type, std::string func_name)
    {
        return llvm::Function::Create(type, llvm::Function::ExternalLinkage, func_name, module_);
    }

    llvm::Function *CreateFunction(llvm::Type *ret_ty, const std::vector<llvm::Type *> &args_ty, std::string func_name)
    {
        return llvm::Function::Create(GetFunctionType(ret_ty, CreateArgsType(args_ty)), llvm::Function::ExternalLinkage, func_name, module_);
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

    llvm::Function *GetFunc(const std::string &fn)
    {
        return module_->getFunction(fn);
    }

protected:
    // lang:
    llvm::LLVMContext context_;
    llvm::IRBuilder<> builder_;
    llvm::Module *module_{};
    llvm::Function *main_{};

    llvm::StructType *eng_ty_{};
    llvm::FunctionType *callb_ty_{};

    HashTypes declared_types_ {};
    
    struct {
        // llvm::ArrayRefs should point to this storage:
        // TODO(dkofanov): arrange storage properly.
        std::vector<std::vector<llvm::Type *>> func_args_types_;
        std::vector<std::vector<llvm::Value *>> func_args_;
        std::vector<llvm::Value *> values_;
        std::vector<llvm::Type *> types_;
    } storage_;
};

class IRGen : public IRGenBase {
public:

public:
    IRGen() : IRGenBase() {}
    
    void CreateIR()
    {
        //DeclareGlobals();
        DeclarePrimitiveTypes();
        //DeclareFunctions();
        CreateMain();
        DumpIr(); 
    }
    void DumpIr() {
        module_->print(llvm::outs(), nullptr);
    }

    void CreateMain()
    {
        auto *intrinsic = CreateFunction(VOID_T, {PTR_T, I64_T}, "PrintNum");

        main_ = CreateFunction(VOID_T, CreateArgsType({}), "__ENTRY");
        variables_scopes_stack_.push_back({});
        blocks_counter_ = 0;
        cur_f_ = main_;
        if (yyparse() != 0) {
            //abort();
        }
        __ CreateRetVoid();
        variables_scopes_stack_.pop_back();
    }

    std::string GenBBName() {
        blocks_counter_++;
        return std::string("bb_" + std::to_string(blocks_counter_));
    }

    void BeginSB() {
        auto *entry_bb = CreateBB();
        blocks_stack_.push_back(entry_bb);
        variables_scopes_stack_.push_back({});
        __ SetInsertPoint(entry_bb);
    }
    
    void BeginLinkedSB() {
        auto *entry_bb = CreateBB();
        __ CreateBr(entry_bb);
        blocks_stack_.push_back(entry_bb);
        variables_scopes_stack_.push_back({});
        __ SetInsertPoint(entry_bb);
    }

    BB CreateBB() {
        return llvm::BasicBlock::Create(context_, GenBBName(), cur_f_);
    }
    
    BB EndSB() {
        auto bb = blocks_stack_.back();
        blocks_stack_.pop_back();
        variables_scopes_stack_.pop_back();
        return bb;
    }

    auto GetCurSBlock() const {
        return blocks_stack_.back();
    }

    void FinalizeWhileStatement(tokcr_t cmp_tok, tokcr_t loop_body_tok) {
        auto loop_body = loop_body_tok.To<BB>();
        auto cmp = cmp_tok.To<Value>();
        auto loop_header = GetCurSBlock();
        auto continuation = CreateBB();

        // Link basic blocks:
        // __ SetInsertPoint(...) should point to correct place for the backward jump:
        __ CreateBr(loop_header);

        __ SetInsertPoint(loop_header);
        __ CreateCondBr(cmp, loop_body, continuation);
        __ SetInsertPoint(continuation);
    }

    void FinalizeIfStatement(tokcr_t cmp_tok, tokcr_t if_body_tok) {
        auto if_body = if_body_tok.To<BB>();
        auto cmp = cmp_tok.To<Value>();
        auto if_header = GetCurSBlock();
        auto continuation = CreateBB();

        // Link basic blocks:
        // __ SetInsertPoint(...) should point to correct place for the backward jump:
        __ CreateBr(continuation);

        __ SetInsertPoint(if_header);
        __ CreateCondBr(cmp, if_body, continuation);
        __ SetInsertPoint(continuation);
    }

    Value DeclareLocalVar(tokcr_t type_id, tokcr_t var_name) {
        auto *type = ResolveTypeByName(type_id.To<Id>());
        auto *var_ref = __ CreateAlloca(type, I64(1));
        variables_scopes_stack_.back().insert({var_name.To<Id>(), {type, var_ref}});
        return var_ref;
    }
    
    Value CreateStore(tokcr_t ptr, tokcr_t value) {
        __ CreateStore(value.To<Value>(), ptr.To<Value>());
        return ptr.To<Value>();
    }

    Value CreateNum(tokcr_t num) {
        return I64(num.To<Num>());
    }

    TypedRef ResolveVar(tokcr_t var_id) {
        for (auto scope = variables_scopes_stack_.rbegin(); scope != variables_scopes_stack_.rend(); scope++) {
            if (scope->find(var_id.To<Id>()) != scope->end()) {
                return (*scope)[var_id.To<Id>()];
            }
        }
        // undefined var
        UNREACHABLE();
    }

    Value LoadVar(tokcr_t var_id) {
        for (auto scope = variables_scopes_stack_.rbegin(); scope != variables_scopes_stack_.rend(); scope++) {
            if (scope->find(var_id.To<Id>()) != scope->end()) {
                auto typed_var_ref = (*scope)[var_id.To<Id>()];
                return __ CreateLoad(typed_var_ref.first, typed_var_ref.second);
            }
        }
        // undefined var
        UNREACHABLE();
    }

    Value CreateOp(tokcr_t op, tokcr_t lhs, tokcr_t rhs) {
        switch(op.To<Op>())
        {
            case Token::Op::ADD: {
                return __ CreateAdd(lhs.To<Value>(), rhs.To<Value>());
            } case Token::Op::SUB: {
                return __ CreateSub(lhs.To<Value>(), rhs.To<Value>());
            } case Token::Op::MUL: {
                return __ CreateMul(lhs.To<Value>(), rhs.To<Value>());
            } case Token::Op::DIV: {
                return __ CreateSDiv(lhs.To<Value>(), rhs.To<Value>());
            }
            default: UNREACHABLE();
        }
    }
    
    Value CreateCmp(tokcr_t cmp_pred, tokcr_t lhs, tokcr_t rhs) {
        llvm::CmpInst::Predicate pred;
        switch(cmp_pred.To<Op>())
        {
            case Token::Op::CMP_EQ: {
                pred = llvm::CmpInst::Predicate::ICMP_EQ;
                break;
            } case Token::Op::CMP_NE: {
                pred = llvm::CmpInst::Predicate::ICMP_NE;
                break;
            } case Token::Op::CMP_GE: {
                pred = llvm::CmpInst::Predicate::ICMP_SGE;
                break;
            } case Token::Op::CMP_LE: {
                pred = llvm::CmpInst::Predicate::ICMP_SLE;
                break;
            } case Token::Op::CMP_GT: {
                pred = llvm::CmpInst::Predicate::ICMP_SGT;
                break;
            } case Token::Op::CMP_LT: {
                pred = llvm::CmpInst::Predicate::ICMP_SLT;
                break;
            }
            default: UNREACHABLE();
        }
        return __ CreateICmp(pred, lhs.To<Value>(), rhs.To<Value>());
    }

    void CreateCallIntrinsicPrintVar(tokcr_t var_tok) {
        auto typed_var_ref = ResolveVar(var_tok);
        const char *name = var_tok.To<Id>().c_str();
        size_t name_sz = var_tok.To<Id>().size();
        auto *name_buf_rt = __ CreateAlloca(I8_T, I64(name_sz + 1));
        for (size_t i = 0; i <= name_sz; i++) {
            auto *name_buf_rt_with_offset = __ CreateAdd(name_buf_rt, I64(i));
            __ CreateStore(I8(name[i]), name_buf_rt_with_offset);
        }
        __ CreateCall(GetFunc("PrintNum"), CreateArgs({typed_var_ref.second, name_buf_rt}));
    }
private:
    llvm::Function *cur_f_ {};
    std::vector<HashTypedReferences> variables_scopes_stack_ {};
    std::vector<BB> blocks_stack_ {};

    size_t blocks_counter_ {};
};

extern IRGen IRG;
#undef __
