#pragma once
#include <mutex>
#include <thread>
#include <filesystem>
#include <utility>
#include <vector>
#include <unordered_set>
#include <chrono>

class Parser {
private:
  struct FileInfo {
    std::filesystem::path filename;
    size_t totalLines = 0;
    size_t blankLines = 0;
    size_t commentLines = 0;

    explicit FileInfo(std::filesystem::path name) : filename(std::move(name)) {
    }

    size_t getCodeLines() const {
      return totalLines - blankLines - commentLines;
    }
  };

  std::unordered_set<std::filesystem::path> filetypes{{".cpp", ".h", ".c", ".hpp"}};
  std::vector<std::thread> threads;
  std::vector<FileInfo> fileInfos;
  std::mutex mtx;

  std::chrono::steady_clock::time_point startTime;

  Parser() = default;
  void joinThreads();
  void countLines(const std::filesystem::path &path);

public:
  Parser(Parser &other) = delete;
  Parser(Parser &&other) = delete;
  Parser &operator=(Parser &other) = delete;
  Parser &operator=(Parser &&other) = delete;

  ~Parser() { joinThreads(); }

  static Parser &GetInstance() {
    static Parser instance;
    return instance;
  }

  void OpenFolder();
  void ParseFiles(const std::filesystem::path &path);
  void PrintSummary();
};
