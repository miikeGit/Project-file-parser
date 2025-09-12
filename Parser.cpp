#include "Parser.h"
#include <iostream>
#include <fstream>

void Parser::OpenFolder() {
  bool status = true;
  std::string path;
  while (status) {
    std::cout << " Please, specify project folder: ";
    std::getline(std::cin, path);
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
      std::cerr << " No such folder\n";
      status = true;
    } else {
      status = false;
    }
  }
  ParseFiles(path);
}

void Parser::countLines(const std::filesystem::path &path) {
  std::ifstream file(path);

  if (!file.is_open()) {
    std::cerr << "File couldn't be opened";
  }

  bool isCommented = false;
  std::string line;

  FileInfo fileInfo(path.filename());
  while (std::getline(file, line)) {
    ++fileInfo.totalLines;

    if (line.find("/*") != std::string::npos)
      isCommented = true;

    if (const auto pos = line.find("//");
      isCommented || pos != std::string::npos &&
      // In case of code before comment in the same line
      std::all_of(line.begin(), line.begin() + pos,
                  [](const char c) {
                    return c == ' ';
                  })) {
      ++fileInfo.commentLines;
    } else if (line.empty())
      ++fileInfo.blankLines;

    if (const auto pos = line.find("*/"); pos != std::string::npos) {
      isCommented = false;
      // In case of code after closing comment in the same line
      if (std::any_of(line.begin() + pos + 2, line.end(),
                      [](const char c) {
                        return c != ' ';
                      })) {
        fileInfo.commentLines--;
      }
    }
  }
  std::lock_guard lock(mtx);
  fileInfos.emplace_back(fileInfo);
}

void Parser::getSummary() {
  writeOutput(std::cout);

  std::ofstream file ("output.txt");
  if (!file.is_open()) {
    std::cerr << "File couldn't be opened";
  }

  writeOutput(file);

  file.close();
}


void Parser::ParseFiles(const std::filesystem::path &path) {
  startTime = std::chrono::steady_clock::now();

  for (const auto &item: std::filesystem::recursive_directory_iterator(path)) {
    if (std::filesystem::is_regular_file(item.path())) {
      if (filetypes.contains(item.path().extension()))
        threads.emplace_back(&Parser::countLines, this, item.path());
    }
  }

  getSummary();
}

void Parser::joinThreads() {
  for (std::thread &thread: threads) {
    thread.join();
  }

  if (!threads.empty()) {
    const auto endTime = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
  }

  threads.clear();
}

void Parser::writeOutput(std::ostream& os) {
  joinThreads();

  os << std::left  << std::setw(50) << "Name"
     << std::right << std::setw(15) << "Total"
                   << std::setw(15) << "Blank"
                   << std::setw(15) << "Comments"
                   << std::setw(15) << "Code\n";

  os << std::string(110, '-') << "\n";

  for (const FileInfo &file: fileInfos) {
    os << std::left  << std::setw(50) << file.filename
       << std::right << std::setw(15) << file.totalLines
                     << std::setw(15) << file.blankLines
                     << std::setw(15) << file.commentLines
                     << std::setw(15) << file.getCodeLines() << "\n";
  }

  os << std::string(110, '-');
  os << "\nTotal processed files - " << fileInfos.size();
  os << "\nTime of execution - " << duration.count() << " ms";
}