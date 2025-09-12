#pragma once
#include <mutex>
#include <thread>
#include <filesystem>
#include <utility>
#include <vector>
#include <unordered_map>

class Parser {
private:
    struct FileInfo {
        std::filesystem::path filename;
        size_t totalLines = 0;
        size_t blankLines = 0;
        size_t commentLines = 0;

        explicit FileInfo(std::filesystem::path name) : filename(std::move(name)) {}
    };

    std::unordered_map<std::string, bool> filetypes {
        {".cpp", true},
        {".h", true},
        {".c", true},
        {".hpp", true}
    };
    std::vector<std::thread> threads;
    std::vector<FileInfo> fileInfos;
    std::mutex mtx;
    Parser() = default;

    void countLines(const std::filesystem::path& path);

public:
    Parser(Parser& other) = delete;
    Parser(Parser&& other) = delete;
    ~Parser() = default;

    static Parser& GetInstance() {
        static Parser instance;
        return instance;
    }

    void OpenFolder();
    void ParseFiles(const std::string& path);
    void PrintFiles();
};
