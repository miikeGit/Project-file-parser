#include "gtest/gtest.h"
#include "Parser.h"
#include <fstream>

class ParserTestHelper {
public:
    struct TestFileInfo {
        bool found = false;
        size_t totalLines = 0;
        size_t blankLines = 0;
        size_t commentLines = 0;
        size_t codeLines = 0;
    };

    static void ParseFiles(const std::filesystem::path &path) {
        Parser::GetInstance().ParseFiles(path);
    }

    static void Reset() {
        Parser::GetInstance().fileInfos.clear();
        Parser::GetInstance().threads.clear();
        Parser::GetInstance().duration = std::chrono::milliseconds::zero();
    }

    static TestFileInfo GetFileInfo(const std::string& filename) {
        const auto& infos = Parser::GetInstance().fileInfos;
        const auto it = std::ranges::find_if(infos,
                                       [&](const auto& info) {
                                           return info.filename == filename;
                                       });

        if (it != infos.end()) {
            return {true, it->totalLines, it->blankLines, it->commentLines, it->getCodeLines()};
        }
        return {false};
    }

    static size_t GetFileCount() {
        return Parser::GetInstance().fileInfos.size();
    }
};

class ParserTest : public ::testing::Test {
protected:
    const std::string testDir = "testDir";

    void SetUp() override {
        std::filesystem::create_directory(testDir);
        ParserTestHelper::Reset();
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir);
        ParserTestHelper::Reset();
    }

    void CreateTestFile(const std::string& filename, const std::string& content) const {
        if (std::ofstream file(std::filesystem::path(testDir) / filename); file.is_open()) {
            file << content;
            file.close();
        }
    }
};

TEST_F(ParserTest, ParsesEmptyFile) {
    CreateTestFile("empty.cpp", "");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("empty.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 0);
    EXPECT_EQ(info.blankLines, 0);
    EXPECT_EQ(info.commentLines, 0);
    EXPECT_EQ(info.codeLines, 0);
}

TEST_F(ParserTest, ParsesFileWithOnlyCode) {
    CreateTestFile("code.cpp", "int main() {\n  return 0;\n}");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("code.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 3);
    EXPECT_EQ(info.blankLines, 0);
    EXPECT_EQ(info.commentLines, 0);
    EXPECT_EQ(info.codeLines, 3);
}

TEST_F(ParserTest, ParsesFileWithOnlyBlankLines) {
    CreateTestFile("blank.cpp", "\n\n\n");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("blank.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 3);
    EXPECT_EQ(info.blankLines, 3);
    EXPECT_EQ(info.commentLines, 0);
    EXPECT_EQ(info.codeLines, 0);
}

TEST_F(ParserTest, ParsesFileWithOnlySingleLineComments) {
    CreateTestFile("single_comment.cpp", "// line 1\n// line 2");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("single_comment.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 2);
    EXPECT_EQ(info.blankLines, 0);
    EXPECT_EQ(info.commentLines, 2);
    EXPECT_EQ(info.codeLines, 0);
}

TEST_F(ParserTest, ParsesFileWithOnlyMultiLineComments) {
    CreateTestFile("multi_comment.cpp", "/* comment start\n another line \n end comment */");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("multi_comment.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 3);
    EXPECT_EQ(info.blankLines, 0);
    EXPECT_EQ(info.commentLines, 3);
    EXPECT_EQ(info.codeLines, 0);
}

TEST_F(ParserTest, ParsesFileWithMixedContent) {
    const std::string content =
        "// header comment\n"
        "#include <iostream>\n"
        "\n"
        "/* multi-line\n"
        "   comment */\n"
        "int main() { // code with comment\n"
        "    return 0;\n"
        "}\n";
    CreateTestFile("mixed.cpp", content);
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("mixed.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 8);
    EXPECT_EQ(info.blankLines, 1);
    EXPECT_EQ(info.commentLines, 3);
    EXPECT_EQ(info.codeLines, 4);
}

TEST_F(ParserTest, IgnoresUnsupportedFileTypes) {
    CreateTestFile("document.txt", "some text");
    CreateTestFile("image.jpg", "");
    CreateTestFile("supported.h", "int x;");
    ParserTestHelper::ParseFiles(testDir);

    EXPECT_EQ(ParserTestHelper::GetFileCount(), 1);
    auto info = ParserTestHelper::GetFileInfo("supported.h");
    EXPECT_TRUE(info.found);
    auto unsupported_info = ParserTestHelper::GetFileInfo("document.txt");
    EXPECT_FALSE(unsupported_info.found);
}

TEST_F(ParserTest, ParsesMultipleFilesInDirectory) {
    CreateTestFile("file1.cpp", "int a;");
    CreateTestFile("file2.h", "// header file");
    ParserTestHelper::ParseFiles(testDir);

    EXPECT_EQ(ParserTestHelper::GetFileCount(), 2);
    EXPECT_TRUE(ParserTestHelper::GetFileInfo("file1.cpp").found);
    EXPECT_TRUE(ParserTestHelper::GetFileInfo("file2.h").found);
}

TEST_F(ParserTest, ParsesFilesRecursively) {
    std::filesystem::create_directory(std::filesystem::path(testDir) / "subdir");
    CreateTestFile("root.cpp", "int a;");
    CreateTestFile("subdir/nested.cpp", "int b;");
    ParserTestHelper::ParseFiles(testDir);

    EXPECT_EQ(ParserTestHelper::GetFileCount(), 2);
    EXPECT_TRUE(ParserTestHelper::GetFileInfo("root.cpp").found);
    EXPECT_TRUE(ParserTestHelper::GetFileInfo("nested.cpp").found);
}

TEST_F(ParserTest, HandlesCodeAndCommentOnSameLine) {
    CreateTestFile("code_comment.cpp", "int x = 1; // a variable");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("code_comment.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 1);
    EXPECT_EQ(info.blankLines, 0);
    EXPECT_EQ(info.commentLines, 0);
    EXPECT_EQ(info.codeLines, 1);
}

TEST_F(ParserTest, HandlesMultiLineCommentOnSingleLine) {
    CreateTestFile("compact_comment.cpp", "/* comment */ int x = 1;");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("compact_comment.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 1);
    EXPECT_EQ(info.blankLines, 0);
    EXPECT_EQ(info.commentLines, 0);
    EXPECT_EQ(info.codeLines, 1);
}

TEST_F(ParserTest, HandlesMultiLineCommentWithCodeAfter) {
    CreateTestFile("comment_code.cpp", "/* comment */ \nint x = 1;");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("comment_code.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.totalLines, 2);
    EXPECT_EQ(info.commentLines, 1);
    EXPECT_EQ(info.codeLines, 1);
}

TEST_F(ParserTest, HandlesSingleLineCommentWithSpaces) {
    CreateTestFile("spaced_comment.cpp", "    // comment with leading spaces");
    ParserTestHelper::ParseFiles(testDir);

    auto info = ParserTestHelper::GetFileInfo("spaced_comment.cpp");
    ASSERT_TRUE(info.found);
    EXPECT_EQ(info.commentLines, 1);
    EXPECT_EQ(info.codeLines, 0);
}

TEST_F(ParserTest, ParsesAllSupportedFileExtensions) {
    CreateTestFile("file.cpp", "int a;");
    CreateTestFile("file.h", "int b;");
    CreateTestFile("file.c", "int c;");
    CreateTestFile("file.hpp", "int d;");
    ParserTestHelper::ParseFiles(testDir);

    EXPECT_EQ(ParserTestHelper::GetFileCount(), 4);
}

TEST_F(ParserTest, HandlesSingletonInstance) {
    Parser& instance1 = Parser::GetInstance();
    Parser& instance2 = Parser::GetInstance();

    EXPECT_EQ(&instance1, &instance2);
}