#ifndef TDFA_TESTS_PROCESS_HARNESS_HPP
#define TDFA_TESTS_PROCESS_HARNESS_HPP

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tdfa::test
{

// A protocol-neutral, bounded child-process driver intended for Catch tests.
// On Windows the child is suspended until it has been assigned to a
// kill-on-close Job Object. Public operations are serialized; output is drained
// concurrently by one reader thread per stream.
class ProcessHarness final
{
public:
    using Clock = std::chrono::steady_clock;
    using Milliseconds = std::chrono::milliseconds;

    enum class Channel
    {
        Stdout,
        Stderr
    };

    enum class LineEnding
    {
        None,
        Lf,
        CrLf,
        Cr
    };

    enum class InputKind
    {
        Start,
        Send,
        CloseStdin
    };

    enum class Outcome
    {
        Ok,
        Timeout,
        Closed,
        ProcessExited,
        ResourceLimit,
        OsError,
        InvalidArgument,
        Unsupported
    };

    enum class Phase
    {
        None,
        Launch,
        StdinWrite,
        Response,
        GracefulExit,
        TotalCase,
        HardCleanup,
        Drain,
        Capture
    };

    struct Watchdogs
    {
        Milliseconds stdin_write{10000};
        Milliseconds response{10000};
        Milliseconds graceful_exit{10000};
        Milliseconds total_case{60000};
        Milliseconds hard_cleanup{30000};
    };

    struct ResourceLimits
    {
        std::size_t max_stdin_event_bytes{1048576};
        std::size_t max_stdout_retained_bytes{8388608};
        std::size_t max_stderr_retained_bytes{8388608};
        std::size_t max_line_bytes{1048576};
        std::size_t max_output_events{100000};
    };

    struct EnvironmentEntry
    {
        std::string name;
        std::string value;
    };

    struct LaunchOptions
    {
        std::filesystem::path executable;
        // Arguments exclude argv[0]; the harness supplies the exact executable
        // path as argv[0] and applies Windows command-line quoting.
        std::vector<std::string> arguments;
        std::optional<std::filesystem::path> working_directory;

        // Only these parent variables are copied. Overrides replace matching
        // names case-insensitively on Windows. The defaults avoid inheriting an
        // unrecorded environment while retaining ordinary loader/temp/locale
        // behavior.
        std::vector<std::string> inherited_environment_names{
            "SystemRoot", "WINDIR", "PATH", "PATHEXT", "TEMP", "TMP",
            "LANG", "LC_ALL", "LC_CTYPE"};
        std::vector<EnvironmentEntry> environment_overrides;

        std::string instrumentation{"unspecified"};
        Watchdogs watchdogs{};
        ResourceLimits limits{};
    };

    struct ByteCapture
    {
        // Before truncation, prefix followed by suffix is the exact byte
        // sequence. After truncation they are the retained first and last
        // portions respectively.
        std::vector<std::uint8_t> prefix;
        std::vector<std::uint8_t> suffix;
        std::uint64_t total_bytes{0};
        bool truncated{false};

        [[nodiscard]] bool exact() const noexcept;
        [[nodiscard]] std::vector<std::uint8_t> ExactBytes() const;
    };

    struct Diagnostic
    {
        Outcome outcome{Outcome::Ok};
        Phase phase{Phase::None};
        std::uint64_t event_ordinal{0};
        std::uint32_t native_error{0};
        Milliseconds elapsed{0};
        std::string message;
    };

    struct LaunchRecord
    {
        std::filesystem::path executable;
        std::string executable_sha256;
        std::vector<std::string> argument_vector;
        std::filesystem::path working_directory;
        std::vector<EnvironmentEntry> inherited_environment;
        std::string locale;
        std::string operating_system;
        std::string architecture;
        std::string build_mode;
        std::string instrumentation;
        std::uint64_t process_id{0};
        std::uint64_t job_identity{0};
        std::uint32_t creation_flags{0};
    };

    struct InputEvent
    {
        std::uint64_t ordinal{0};
        InputKind kind{InputKind::Send};
        std::vector<std::uint8_t> bytes;
        std::uint64_t total_bytes{0};
        bool bytes_omitted_due_to_limit{false};
        Milliseconds started_elapsed{0};
        Milliseconds completed_elapsed{0};
        Outcome outcome{Outcome::Ok};
        std::uint32_t native_error{0};
    };

    struct OutputEvent
    {
        std::uint64_t ordinal{0};
        Channel channel{Channel::Stdout};
        ByteCapture bytes;
        LineEnding line_ending{LineEnding::None};
        std::string leading_token;
        Milliseconds elapsed{0};
        // True when the independently retained stream prefix/suffix is the
        // only byte record because that stream exceeded its byte ceiling.
        bool payload_omitted_due_to_stream_limit{false};
    };

    struct StreamSnapshot
    {
        ByteCapture bytes;
        std::uint64_t line_events{0};
        std::uint64_t first_overflow_ordinal{0};
        bool eof{false};
    };

    struct Transcript
    {
        LaunchRecord launch;
        std::vector<InputEvent> input_events;
        std::vector<OutputEvent> output_events;
        StreamSnapshot stdout_stream;
        StreamSnapshot stderr_stream;
        std::vector<Diagnostic> diagnostics;
        std::uint64_t total_output_events{0};
        std::uint64_t first_output_event_overflow_ordinal{0};
        std::optional<std::uint32_t> exit_code;
        bool output_events_truncated{false};
        bool stdin_closed{false};
        bool process_exited{false};
        bool process_group_empty{false};
        bool hard_cleanup_attempted{false};
        bool hard_cleanup_completed{false};
    };

    struct OperationResult
    {
        Outcome outcome{Outcome::Ok};
        std::optional<Diagnostic> diagnostic;

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return outcome == Outcome::Ok;
        }
    };

    struct SendResult : OperationResult
    {
        std::uint64_t ordinal{0};
        Clock::time_point started_at{};
        std::size_t bytes_written{0};
    };

    struct LineResult : OperationResult
    {
        std::optional<OutputEvent> event;
    };

    struct ExitResult : OperationResult
    {
        std::optional<std::uint32_t> exit_code;
    };

    using LinePredicate = std::function<bool(OutputEvent const&)>;

    [[nodiscard]] static bool IsSupported() noexcept;
    [[nodiscard]] static LaunchOptions FromEnvironment(
        std::string const& variable = "TDFA_ENGINE_PATH");

    explicit ProcessHarness(LaunchOptions options);
    ~ProcessHarness();

    ProcessHarness(ProcessHarness const&) = delete;
    ProcessHarness& operator=(ProcessHarness const&) = delete;
    ProcessHarness(ProcessHarness&&) noexcept;
    ProcessHarness& operator=(ProcessHarness&&) noexcept;

    [[nodiscard]] bool started() const noexcept;
    [[nodiscard]] std::optional<Diagnostic> launch_diagnostic() const;
    [[nodiscard]] std::uint64_t process_id() const noexcept;

    // SEND owns a copy of bytes until the watchdog-controlled write completes.
    // Binary NULs are preserved by both overloads.
    [[nodiscard]] SendResult Send(std::string_view bytes);
    [[nodiscard]] SendResult Send(std::vector<std::uint8_t> const& bytes);
    [[nodiscard]] OperationResult CloseStdin();

    // The response deadline is always send.started_at + watchdogs.response,
    // capped by the total-case deadline. Partial output cannot extend it.
    // after_ordinal permits sequential consumption under the same SEND anchor.
    [[nodiscard]] LineResult WaitForLineAfter(
        SendResult const& send,
        LinePredicate predicate,
        std::uint64_t after_ordinal = 0);
    [[nodiscard]] LineResult WaitForTokenAfter(
        SendResult const& send,
        std::string_view leading_token,
        Channel channel = Channel::Stdout,
        std::uint64_t after_ordinal = 0);

    // The graceful deadline is anchored to this call. On expiry, cleanup uses
    // its own absolute hard-cleanup deadline and targets only the recorded job.
    [[nodiscard]] ExitResult WaitForExit();
    [[nodiscard]] OperationResult TerminateAndDrain();

    [[nodiscard]] Transcript Snapshot() const;

    [[nodiscard]] static std::string EscapeBytes(ByteCapture const& bytes);
    [[nodiscard]] static std::string HexBytes(ByteCapture const& bytes);
    [[nodiscard]] static char const* ToString(Outcome outcome) noexcept;
    [[nodiscard]] static char const* ToString(Phase phase) noexcept;
    [[nodiscard]] static char const* ToString(Channel channel) noexcept;
    [[nodiscard]] static char const* ToString(LineEnding ending) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tdfa::test

#endif
