#ifndef LANG_MAIN_H
#define LANG_MAIN_H

#include "lexer_decls.h"

#include "../common/macro.h"
#include "graphics/graphics.h"
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


using token_t = Token;
using tokcr_t = const token_t &;
using Value = token_t::Value;
using Type = token_t::Type;
using Op = token_t::Op;
using Id = token_t::Id;
using BB = token_t::BB;
using Num = token_t::Num;
using TypedRef = token_t::TypedRef;
using TypedName = std::pair<Type, std::string>;
using TypedIdx = std::pair<Type, size_t>;

using VectorTypes = std::vector<Type>;
using VectorTypedNames = std::vector<TypedName>;
using HashIndices = std::unordered_map<std::string, size_t>;
using HashTypes = std::unordered_map<std::string, Type>;
using HashTypedReferences = std::unordered_map<std::string, TypedRef>;

class IRGenBase;

   class ObjectTemplate {
    public:
        ObjectTemplate(IRGenBase *irg) : irg_(irg) {}
        void AddInitializer(const Id& name, const Type &type) {
            initializers_.Add(name, type);
        }
        void AddMember(const Id& name, const Type &type) {
            members_.Add(name, type);
        }
        void SetName(const Id &name) {
            name_ = name;
        }
        const Id &GetName() const {
            return name_;
        }
        TypedIdx GetMemberTypedIdx(const Id &name) {
            if (initializers_.Contains(name)) {
                return initializers_.GetElementIdx(name);
            } else if (members_.Contains(name)) {
                auto typed_idx = members_.GetElementIdx(name);
                return {typed_idx.first, typed_idx.second + initializers_.Size()};
            }
            UNREACHABLE("Unknown var name: " << name);
        }

        void SetFunction(llvm::Function *func) {
            func_ = func;
        }
    
        Type LowerToType();

        Type LowerToTypePointer() {
            return llvm::PointerType::getUnqual(LowerToType());
        }
        
        auto *GetFunction() {
            return func_;
        }
    private:
        struct Members {
        public:
            void Add(const Id& name, const Type &type) {
                ASSERT(indices_.find(name) == indices_.end());
                types_.push_back(type);
                indices_[name] = types_.size() - 1;
            }

            bool Contains(const Id &name) {
                if (indices_.find(name) != indices_.end()) {
                    return true;
                }
                return false;
            }

            TypedIdx GetElementIdx(const Id &name) {
                size_t idx = indices_[name];
                return {types_[idx], idx};
            }

            size_t Size() {
                return types_.size();
            }
        public:
            HashIndices indices_ {};
            VectorTypes types_ {};
        };
        IRGenBase *irg_ {};
        Members initializers_ {};
        Members members_ {};
        llvm::Function *func_ {};
        Type ll_type_ {};
        Id name_ {};
    };

class IRGenBase {
public:
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
        ReserveStorage();
    }

    void ReserveStorage() {
        storage_.Reserve();
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

    void DeclareUserType(Type type, const Id &name)
    {
        declared_types_[name] = type;
    }

    template <bool is_ptr = false>
    Type ResolveTypeByName(const std::string &type) {
        if constexpr (is_ptr) {
            return llvm::PointerType::getUnqual(declared_types_[type]);
        }
        return declared_types_[type];
    }

    void LaunchEE() {
        llvm::ExecutionEngine *ee = llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module_)).create();

        ee->InstallLazyFunctionCreator([&](const std::string &fn) -> void *
            {
                if (fn == "PrintNum") {return reinterpret_cast<void *>(PrintNum);}
                if (fn == "rand") { return reinterpret_cast<void *>(rand); }
                if (fn == "srand") { return reinterpret_cast<void *>(srand); }
                if (fn == "usleep") { return reinterpret_cast<void *>(usleep); }
                if (fn == "GR_Initialize") { return reinterpret_cast<void *>(GR_Initialize); }
                if (fn == "GR_PutPixel") { return reinterpret_cast<void *>(GR_PutPixel); }
                if (fn == "GR_Flush") { return reinterpret_cast<void *>(GR_Flush); }

                UNREACHABLE("NO_FUNC " << fn);
            });
        ee->finalizeObject();
        std::vector<llvm::GenericValue> noargs;
        llvm::GenericValue gv = ee->runFunction(main_, noargs);
    }

    static void PrintNum(void *ptr, char *name) {
        printf("%p: 0x", ptr);
        for (size_t i = 0; i < sizeof(token_t::Num); i++) {
            printf("%02hhX", *(static_cast<char *>(ptr) + i));
        }
        std::cout << " ( `" << name << "` = " << *(static_cast<token_t::Num *>(ptr)) << ')'  << std::endl;
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

    ObjectTemplate *CreateObject()
    {
        storage_.objects_.emplace_back(this);
        return &storage_.objects_.back();
    }

    llvm::Function *CreateFunction(llvm::Type *ret_ty, const std::vector<llvm::Type *> &args_ty, std::string func_name)
    {
        return llvm::Function::Create(GetFunctionType(ret_ty, CreateArgsType(args_ty)), llvm::Function::ExternalLinkage, func_name, module_);
    }

    llvm::ArrayRef<llvm::Type *> ConcatTypesVectors(const std::vector<llvm::Type *> &vec1, const std::vector<llvm::Type *> &vec2) {
        storage_.types_.push_back(vec1);
        storage_.types_.back().insert(storage_.types_.back().end(), vec2.begin(), vec2.end());
        return storage_.types_.back();
    }

    std::string *AllocateEmptyString() {
        storage_.strings_.push_back("");
        return &storage_.strings_.back();
    }

public:
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

protected:
    llvm::Function *GetFunc(const std::string &fn)
    {
        auto f = module_->getFunction(fn);
        ASSERT(f != nullptr);
        return f;
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
    public:
        void Reserve() {
            // if relocation happens, all pointers will be invalidated:
            objects_.reserve(100);
            types_.reserve(100);
            values_.reserve(100);
            func_args_.reserve(100);
            func_args_types_.reserve(100);
            strings_.reserve(100);
        }
    public:
        // llvm::ArrayRefs should point to this storage:
        // TODO(dkofanov): arrange storage properly.
        std::vector<std::vector<llvm::Type *>> func_args_types_ {};
        std::vector<std::vector<llvm::Value *>> func_args_ {};
        std::vector<llvm::Value *> values_ {};
        std::vector<std::vector<llvm::Type *>> types_ {};
        std::vector<ObjectTemplate> objects_ {};
        std::vector<std::string> strings_ {};
    } storage_;

    friend ObjectTemplate;
};

class IRGen : public IRGenBase {
public:
    IRGen() : IRGenBase() {}
    
    void CreateIR()
    {
        DeclarePrimitiveTypes();
        DeclareIntrinsics();

        ParseScope();
        DumpIr(); 
    }
    void DumpIr() {
        module_->print(llvm::outs(), nullptr);
    }

    void BeginObject(const Id &name) {
        ASSERT(name != "__ENTRY");
        auto obj_ptr = CreateObject();
        object_stack_.push_back(obj_ptr);
        ASSERT(declared_hl_types_.find(name) == declared_hl_types_.end());
        declared_hl_types_[name] = obj_ptr; 
        obj_ptr->SetName(name);
    }

    void EndObject(const Id &name) {

        DeclareUserType(GetCurrentObject()->LowerToType(), name);
        if (name == "main") {
            ASSERT(main_ == nullptr);
            CreateMainEntry();
        }
        object_stack_.pop_back();
    }

    void CreateMainEntry() {
        ASSERT(GetCurrentObject()->GetName() == "main");
        main_ = CreateFunction(VOID_T, {}, "__ENTRY");
        auto *bb = llvm::BasicBlock::Create(context_, "__entry", main_);
        __ SetInsertPoint(bb);
        auto *obj_ptr = __ CreateAlloca(GetCurrentObject()->LowerToType());
        InvokeObject(GetCurrentObject(), obj_ptr);
        __ CreateRetVoid();
    }

    void InvokeObject(ObjectTemplate *obj, Value obj_ptr) {
        __ CreateCall(obj->GetFunction(), CreateArgs({obj_ptr}));
    }

    ObjectTemplate *GetCurrentObject() {
        return object_stack_.back();
    }

    void BeginMemberFunction() {
        auto *f = CreateFunction(VOID_T, {GetCurrentObject()->LowerToTypePointer()}, "m");
        functions_stack_.push_back(f);
    }

    void EndMemberFunction() {
        __ CreateRetVoid();
        functions_stack_.pop_back();
    }

    llvm::Function *GetCurrentFunction() {
        return functions_stack_.back();
    }

    void ParseScope()
    {
        auto *intrinsic = CreateFunction(VOID_T, {PTR_T, I64_T}, "PrintNum");
        AccessChainAllocCtx();
        variables_scopes_stack_.push_back({});
        blocks_counter_ = 0;
        if (yyparse() != 0) {
            ParseErrorHandleAndAbort();
        }
        AccessChainFreeCtx();
        variables_scopes_stack_.pop_back();
    }

    void ParseErrorHandleAndAbort() {
        std::cout << "\n# Parsing failed!\n  Print generated ir? (y/n)" << std::endl;
        char ans = getchar();
        while (1) {
            switch (ans)
            {
                case 'y':
                    DumpIr();
                    exit(-1);
                case 'n':
                    exit(-1);
                case '\n':
                    ans = getchar();
                    break;
                default:
                    std::cout << "Press (y/n)" << std::endl;
                    ans = getchar();
            }
        }
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
        return llvm::BasicBlock::Create(context_, GenBBName(), GetCurrentFunction());
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

    Value DeclareLocalVar(tokcr_t type_id, tokcr_t var_name, Value ar_length) {
        auto *type = ResolveTypeByName(type_id.To<Id>());
        if (type == nullptr) {
            UNREACHABLE("Unknown typename: " << type_id.To<Id>());
        }
        Value var_ref = __ CreateAlloca(type, ar_length);
        variables_scopes_stack_.back().insert({var_name.To<Id>(), {type, var_ref}});
        return var_ref;
    }

    Value PromoteToLocalArray(tokcr_t var_name) {
        // if it is an array, typed_ref.second->getType() is a type of array element;
        for (auto scope = variables_scopes_stack_.rbegin(); scope != variables_scopes_stack_.rend(); scope++) {
            if (scope->find(var_name.To<Id>()) != scope->end()) {
                auto typed_ref = (*scope)[var_name.To<Id>()];
                auto type_ptr = llvm::PointerType::getUnqual(typed_ref.first);
                (*scope)[var_name.To<Id>()] = {type_ptr, typed_ref.second};
                return typed_ref.second;
            }
        }
        UNREACHABLE("Can't promote to pointer: " << var_name.To<Id>());
    }
    
    Value CreateStore(tokcr_t ptr, tokcr_t value) {
        __ CreateStore(value.To<Value>(), ptr.To<Value>());
        return ptr.To<Value>();
    }

    Value CreateNum(tokcr_t num) {
        return I64(num.To<Num>());
    }

    void AppendAccessChain(const Id &var_id, Value offset = nullptr) {
        temp_access_chain_stack_.back().push_back({var_id, offset});
    }
    
    TypedRef ResolveAccessChain() {
        ASSERT(temp_access_chain_stack_.back().size() > 0);
        const auto &var_name = temp_access_chain_stack_.back()[0].first;
        auto offset = temp_access_chain_stack_.back()[0].second;
        auto var0_typed_ref = ResolveVar(var_name);
        auto current_type = var0_typed_ref.first;
        auto current_ref = var0_typed_ref.second;
        bool should_be_dereferenced = ((temp_access_chain_stack_.back().size() != 1) || (offset != nullptr));
        if ((current_type->isPointerTy())  && should_be_dereferenced) {
            if (current_type != current_ref->getType()) {
                // Resolve pure-pointer:
                current_ref = __ CreateLoad(current_type, current_ref);
            }
            current_type = current_type->getPointerElementType();
        }
        
        if ((offset != nullptr) && (temp_access_chain_stack_.back().size() == 1)) {
            // resolve elem by offset here as main loop wouldn't be hit:
            ASSERT(current_type == current_ref->getType()->getPointerElementType());
            current_ref = __ CreateGEP(current_type, current_ref, CreateArgs({offset}));
        }
    
        for (size_t i = 1; i < temp_access_chain_stack_.back().size(); i++) {
            /*if (current_type->isPointerTy()) {
                ASSERT((temp_access_chain_stack_.back().size() == (i + 1)) || (offset != nullptr));
                if (offset != nullptr) {
                    current_type = current_type->getPointerElementType();
                }
            } else {
                ASSERT(offset == nullptr);
            }*/
            ASSERT(current_type->isStructTy());

            if (current_type->isStructTy()) {
                const auto &pending_var_name = temp_access_chain_stack_.back()[i].first;
                auto pending_offset = temp_access_chain_stack_.back()[i].second;

                ASSERT(current_ref->getType()->isPointerTy());
                
                auto struct_name = static_cast<llvm::StructType *>(current_type)->getName();
                auto typed_idx = GetDeclaredHLType(struct_name)->GetMemberTypedIdx(pending_var_name);
                if (offset == nullptr) {
                    offset = I64(0);
                }
                current_ref = __ CreateGEP(current_type, current_ref, CreateArgs({offset, I32(typed_idx.second)}));
                current_type = typed_idx.first;
                offset = pending_offset;
                if ((current_type->isPointerTy()) && (current_type != current_ref->getType())
                    && ((temp_access_chain_stack_.back().size() != (i + 1)) || (offset != nullptr))) {
                    // Resolve pure-pointer:
                    current_ref = __ CreateLoad(current_type, current_ref);
                    current_type = current_type->getPointerElementType();
                }
                if (offset != nullptr) {
                    current_ref = __ CreateGEP(current_type, current_ref, CreateArgs({offset}));
                }
            }
        }
        // get ref for array element if the the last particle is array:


        auto full_var_name = AllocateEmptyString();
        (*full_var_name).append(temp_access_chain_stack_.back()[0].first);
        if (temp_access_chain_stack_.back()[0].second != nullptr) {
            (*full_var_name).append("[");
            (*full_var_name).append(temp_access_chain_stack_.back()[0].second->getName());
            (*full_var_name).append("]");
        }
        for (size_t i = 1; i < temp_access_chain_stack_.back().size(); i++) {
            (*full_var_name).append(".");
            (*full_var_name).append(temp_access_chain_stack_.back()[i].first);
            if (temp_access_chain_stack_.back()[i].second != nullptr) {
                (*full_var_name).append("[");
                (*full_var_name).append(temp_access_chain_stack_.back()[i].second->getName());
                (*full_var_name).append("]");
            }
        }
        (*full_var_name).append(":ref:");
        current_ref->setName(*full_var_name);
        temp_access_chain_stack_.back().clear();
        return {current_type, current_ref};
    }

    void AccessChainAllocCtx() {
        temp_access_chain_stack_.push_back({});
    }
    void AccessChainFreeCtx() {
        temp_access_chain_stack_.pop_back();
    }

    ObjectTemplate *GetDeclaredHLType(const Id &name) {
        ASSERT(declared_hl_types_.find(name) != declared_hl_types_.end());
        return declared_hl_types_[name];
    }

    TypedRef ResolveVar(tokcr_t var_id) {
        const Id &var_name = var_id.To<Id>();
        // Try resolve local vars:
        for (auto scope = variables_scopes_stack_.rbegin(); scope != variables_scopes_stack_.rend(); scope++) {
            if (scope->find(var_name) != scope->end()) {
                return (*scope)[var_name];
            }
        }
        // Try resolve explicit 'this':
        constexpr size_t implicit_this_idx = 0;
        auto implicit_this = GetCurrentFunction()->getArg(implicit_this_idx);
        if (var_name == "this") {
            ASSERT(implicit_this != nullptr);
            ASSERT(!implicit_this->hasByValAttr());
            return {GetCurrentObject()->LowerToType(),  implicit_this};
        }
        // Try resolve member of implicit 'this'; hits UNREACHABLE on failure:
        auto typed_idx = object_stack_.back()->GetMemberTypedIdx(var_id.To<Id>());
        auto gep = __ CreateGEP(object_stack_.back()->LowerToType(), implicit_this, CreateArgs({I64(0), I32(typed_idx.second)}));
        return {typed_idx.first, gep};
    }

    Value LoadRef(TypedRef typed_ref) {
        if (typed_ref.first == typed_ref.second->getType()) {
            return typed_ref.second;
        }
        auto loaded =  __ CreateLoad(typed_ref.first, typed_ref.second);
        loaded->setName(typed_ref.second->getName() + "rslvd::");
        return loaded;
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
            } case Token::Op::MOD: {
                return __ CreateSRem(lhs.To<Value>(), rhs.To<Value>());
            }
            default: UNREACHABLE("Unknown op: " << static_cast<size_t>(op.To<Op>()));
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
            default: UNREACHABLE("Unknown cmp_pred: " << static_cast<size_t>(cmp_pred.To<Op>()));
        }
        return __ CreateICmp(pred, lhs.To<Value>(), rhs.To<Value>());
    }

    void CreateCallIntrinsicPrintVar(tokcr_t lval_ref_tok) {
        auto lval_ref = lval_ref_tok.To<TypedRef>().second;
        
        auto name = lval_ref->getName();
        size_t name_sz = name.size();
        auto *name_buf_rt = __ CreateAlloca(I8_T, I64(name_sz + 1));
        
        for (size_t i = 0; i < name_sz; i++) {
            auto *name_buf_rt_with_offset = __ CreateAdd(name_buf_rt, I64(i));
            __ CreateStore(I8(name[i]), name_buf_rt_with_offset);
        }
        auto *name_buf_rt_with_offset = __ CreateAdd(name_buf_rt, I64(name_sz));
        __ CreateStore(I8(0), name_buf_rt_with_offset);
        
        __ CreateCall(GetFunc("PrintNum"), CreateArgs({lval_ref, name_buf_rt}));
    }


    void DeclareIntrinsics()
    {
        // declare void @main()
        CreateFunction(VOID_T, CreateArgsType({I64_T, I64_T, I64_T}), "GR_PutPixel");
        CreateFunction(VOID_T, CreateArgsType({I64_T, I64_T, I64_T}), "GR_Initialize");
        CreateFunction(VOID_T, CreateArgsType({}), "GR_Flush");
        CreateFunction(VOID_T, CreateArgsType({I64_T}), "srand");
        CreateFunction(I64_T, CreateArgsType({}), "rand");
        CreateFunction(VOID_T, CreateArgsType({I64_T}), "usleep");
    }


    void intrinsicPutPixol(Value x, Value y) {
        __ CreateCall(GetFunc("GR_PutPixel"), CreateArgs({x, y}));
    }
    void intrinsicInitGraphics(Value w, Value h, Value scale) {
        __ CreateCall(GetFunc("GR_Initialize"), CreateArgs({w, h, scale}));
    }
    void intrinsicFlush() {
        __ CreateCall(GetFunc("GR_Flush"), CreateArgs({}));
    }
    
    void intrinsicSleep(Value x) {
        __ CreateCall(GetFunc("usleep"), CreateArgs({x}));
    }
    void intrinsicSrand(Value x) {
        __ CreateCall(GetFunc("srand"), CreateArgs({x}));
    }
    Value intrinsicRand() {
        auto rand = __ CreateCall(GetFunc("rand"), CreateArgs({}));
        rand->setName("rand_num");
        return rand;
    }

private:
    std::vector<llvm::Function *> functions_stack_ {};
    std::vector<HashTypedReferences> variables_scopes_stack_ {};
    std::vector<BB> blocks_stack_ {};
    std::vector<ObjectTemplate *> object_stack_ {};
    std::unordered_map<std::string, ObjectTemplate *> declared_hl_types_ {};
    std::vector<std::vector<std::pair<std::string, Value>>> temp_access_chain_stack_ {};
    size_t blocks_counter_ {};
};

extern IRGen *IRG;
#undef __

#endif  // LANG_MAIN_H
