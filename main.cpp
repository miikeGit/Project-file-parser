#include <iostream>
#include "Parser.h"

// TODO: save in file

int main() {
  Parser &parser = Parser::GetInstance();
  parser.OpenFolder();
  parser.PrintSummary();
  std::cin.get();
  return 0;
}
