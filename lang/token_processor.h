#ifndef TOKEN_PROCESSOR_H
#define TOKEN_PROCESSOR_H

#include "../common/macro.h"
#include "llvm/IR/IRBuilder.h"

#include <variant>
#include <string>
#include <cstdint>

#define YYSTYPE Token

class Token {
public:
    enum class Op {
        ADD,
        SUB,
        MUL,
        DIV,
        CMP_EQ,
        CMP_NE,
        CMP_GE,
        CMP_LE,
        CMP_GT,
        CMP_LT,
    };
    using Num = int64_t;
    using Id = std::string;
    using Type = llvm::Type *;
    using Value = llvm::Value *;
    using BB = llvm::BasicBlock *;

    using val_t = std::variant<Num, Op, Id, Type, Value, BB>;

    template<typename T>
    T To() const
    {
        return *std::get_if<T>(&val_);
    }

    Token operator=(Num num)
    {
        val_ = num;
        return *this;
    }

    Token()
    {
        val_ = 0;
    }
    
    template <typename T>
    Token(const T &val)
    {
        val_ = val;
    }

    Token(char *character)
    {
        switch (*character)
        {
            case '+': {
                val_ = Op::ADD;
                return;
            } case '-': {
                val_ = Op::SUB;
                return;
            }
            case '*': {
                val_ = Op::MUL;
                return;
            } case '/': {
                val_ = Op::DIV;
                return;
            } case '=': {
                if (*(character + 1) == '=') {
                    val_ = Op::CMP_EQ;
                    return;
                }
                UNREACHABLE();
            } case '!': {
                if (*(character + 1) == '=') {
                    val_ = Op::CMP_NE;
                    return;
                } 
                UNREACHABLE();
            } case '>': {
                if (*(character + 1) == '=') {
                    val_ = Op::CMP_GE;
                    return;
                } else if  (*(character + 1) == '\0') {
                    val_ = Op::CMP_GT;
                    return;
                }
                UNREACHABLE();
            } case '<': {
                if (*(character + 1) == '=') {
                    val_ = Op::CMP_LE;
                    return;
                } else if  (*(character + 1) == '\0') {
                    val_ = Op::CMP_LT;
                    return;
                }
                UNREACHABLE();
            }
            
            default:
                UNREACHABLE();
        }
        UNREACHABLE();
    }

private:
    val_t val_;
};

#endif  // TOKEN_PROCESSOR_H
