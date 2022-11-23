#include "lang_main.h"

IRGen IRG;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        return 1;
    }
    auto file = std::fopen(argv[1], "r");
    if (file == nullptr) {
        return 2;
    }

    yydebug = 0;
    yyin = file;
    IRG.CreateIR();
    IRG.LaunchEE();

    return 0;
}
