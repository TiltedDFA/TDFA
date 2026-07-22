#include <catch2/catch_test_macros.hpp>

#include "Uci.hpp"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{

class ScopedStandardStreams final
{
public:
    explicit ScopedStandardStreams(std::string input)
        : input_(std::move(input)), old_input_state_(std::cin.rdstate()),
          old_output_state_(std::cout.rdstate())
    {
        old_input_ = std::cin.rdbuf(input_.rdbuf());
        old_output_ = std::cout.rdbuf(output_.rdbuf());
        std::cin.clear();
        std::cout.clear();
    }

    ~ScopedStandardStreams()
    {
        std::cin.rdbuf(old_input_);
        std::cout.rdbuf(old_output_);
        std::cin.clear(old_input_state_);
        std::cout.clear(old_output_state_);
    }

    ScopedStandardStreams(const ScopedStandardStreams &) = delete;
    ScopedStandardStreams &operator=(const ScopedStandardStreams &) = delete;

    [[nodiscard]] std::string output() const { return output_.str(); }

private:
    std::istringstream input_;
    std::ostringstream output_;
    std::streambuf *old_input_ = nullptr;
    std::streambuf *old_output_ = nullptr;
    std::ios::iostate old_input_state_;
    std::ios::iostate old_output_state_;
};

class ScopedEmptyWorkingDirectory final
{
public:
    ScopedEmptyWorkingDirectory()
        : original_(std::filesystem::current_path()),
          temporary_(std::filesystem::temp_directory_path() /
                     ("tdfa-coverage-audit-" +
                      std::to_string(std::chrono::steady_clock::now()
                                         .time_since_epoch()
                                         .count())))
    {
        std::error_code error;
        if (!std::filesystem::create_directory(temporary_, error) || error)
            throw std::runtime_error("cannot create isolated audit directory: " +
                                     error.message());
        std::error_code change_error;
        std::filesystem::current_path(temporary_, change_error);
        if (change_error)
        {
            std::error_code cleanup_error;
            std::filesystem::remove_all(temporary_, cleanup_error);
            temporary_.clear();
            throw std::runtime_error("cannot enter isolated audit directory: " +
                                     change_error.message());
        }
    }

    ~ScopedEmptyWorkingDirectory()
    {
        std::error_code restore_error;
        std::filesystem::current_path(original_, restore_error);
        if (!restore_error)
        {
            std::error_code cleanup_error;
            std::filesystem::remove_all(temporary_, cleanup_error);
        }
    }

    ScopedEmptyWorkingDirectory(const ScopedEmptyWorkingDirectory &) = delete;
    ScopedEmptyWorkingDirectory &
    operator=(const ScopedEmptyWorkingDirectory &) = delete;

private:
    std::filesystem::path original_;
    std::filesystem::path temporary_;
};

} // namespace

TEST_CASE("Coverage audit exercises the public UCI loop's bounded handlers",
          "[audit][coverage]")
{
    // This packet is intentionally implementation-aware. It supplies safe,
    // bounded inputs to otherwise uncovered public-loop paths, but its execution
    // is not semantic evidence for any Routine TESTED mapping.
    const std::string input =
        "   \n"
        "position startpos\n"
        "position startpos moves e2e4\n"
        "go wtime 0 btime 0 winc 0 binc 0\n"
        "position startpos\n"
        "go wtime 0 btime 0 winc 0 binc 0\n"
        "position fen 7k/8/8/8/8/8/6K1/8 b - - 0 1\n"
        "stop\n"
        "ucinewgame\n"
        "setoption name hash value 1\n"
        "print state\n"
        "bench nope\n"
        "bench perft suite\n"
        "quit\n";

    ScopedEmptyWorkingDirectory empty_working_directory;
    ScopedStandardStreams streams(input);
    Uci session;
    session.Loop();

    const std::string output = streams.output();
    REQUIRE(output.size() < 64U * 1024U);
    CAPTURE(output);
}
