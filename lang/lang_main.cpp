#include "lang_main.h"

Type ObjectTemplate::LowerToType() {
    if (ll_type_ == nullptr) {
        auto struct_fields = irg_->ConcatTypesVectors(inputs_.types_, outputs_.types_);
        if (struct_fields.empty()) {
            UNREACHABLE();
        } else {
            ll_type_ = llvm::StructType::create(struct_fields, name_);
        }
    }
    return ll_type_;
}

IRGen *IRG;

int main(int argc, char *argv[])
{
    IRGen irg{};
    IRG = &irg;
    if (argc != 2) {
        return 1;
    }
    auto file = std::fopen(argv[1], "r");
    if (file == nullptr) {
        return 2;
    }

    yydebug = 1;
    yyin = file;
    IRG->CreateIR();
    IRG->LaunchEE();

    return 0;
}
