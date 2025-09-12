#include <iostream>
#include "Parser.h"

// TODO: refactor, save in file, time of execution

int main() {
    Parser& parser = Parser::GetInstance();

    parser.OpenFolder();
    parser.PrintFiles();
    std::cin.get();
    return 0;
}