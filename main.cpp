#include <iostream>
#include "Parser.h"

int main() {
  Parser &parser = Parser::GetInstance();
  parser.OpenFolder();
  std::cin.get();
  return 0;
}
