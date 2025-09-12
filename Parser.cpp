#include "Parser.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>

void Parser::OpenFolder() {
    bool status = true;
    std::string path;

    while (status) {
        std::cout << " Please, specify project folder: ";
        std::getline(std::cin, path);
        if (!std::filesystem::exists(path)) {
            std::cerr << "No such folder";
            status = true;
        } else {
            status = false;
        }
    }
    ParseFiles(path);
}

void Parser::countLines(const std::filesystem::path& path) {
    if (std::ifstream file(path); file.is_open()) {
        bool isCommented = false;
        std::string line;

        FileInfo fileInfo(path.filename().string());
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
                            }

            else if (line.empty())
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
    } else {
        std::cerr << "File couldn't be opened";
    }
}

void Parser::ParseFiles(const std::string &path) {
    for (const auto& item : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_regular_file(item.path())) {
            if (filetypes[item.path().extension().string()])
                threads.emplace_back(&Parser::countLines, this, item.path());
        } else
            ParseFiles(item.path().string());
    }
}

void Parser::PrintFiles() {
    for (std::thread& thread: threads) {
        thread.join();
    }

    std::cout << std::left << std::setw(30) << "Name"
              << std::right << std::setw(15) << "Total"
              << std::setw(15) << "Blank"
              << std::setw(15) << "Comments"
              << std::setw(15) << "Code" << "\n";

    std::cout << std::string(90, '-') << "\n";

    for (const FileInfo& file : fileInfos) {
        const size_t codeLines = file.totalLines - file.blankLines - file.commentLines;

        std::cout << std::left  << std::setw(30) << file.filename
                  << std::right << std::setw(15) << file.totalLines
                                << std::setw(15) << file.blankLines
                                << std::setw(15) << file.commentLines
                                << std::setw(15) << codeLines << "\n";
    }
}
