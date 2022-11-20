#ifndef TOKEN_PROCESSOR_H
#define TOKEN_PROCESSOR_H

#include <variant>
#include <cstdint>

#define YYSTYPE Token


#define UNREACHABLE() __builtin_unreachable()

class Token {
public:
    enum class Op {
        ADD,
        SUB,
        MUL,
        DIV,
    };
    using Num = int64_t;
    using val_t = std::variant<Num, Op>;

    Num ToNum()
    {
        return *std::get_if<Num>(&val_);
    }
    Op ToOp()
    {
        return *std::get_if<Op>(&val_);
    }

    Token operator=(Num num)
    {
        val_ = num;
        return *this;
    }

    Token operator=(char *num)
    {
        switch (*num)
        {
            case '+': {
                val_ = Op::ADD;
                return *this;
            } case '-': {
                val_ = Op::SUB;
                return *this;
            }
            case '*': {
                val_ = Op::MUL;
                return *this;
            } case '/': {
                val_ = Op::DIV;
                return *this;
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
