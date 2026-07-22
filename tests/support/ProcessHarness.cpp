#include "ProcessHarness.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <limits>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <bcrypt.h>
#if defined(__MINGW32__)
#include <pthread.h>
#endif
#if defined(_MSC_VER)
#pragma comment(lib, "bcrypt.lib")
#endif
#endif

namespace tdfa::test
{

bool ProcessHarness::ByteCapture::exact() const noexcept
{
    return !truncated && prefix.size() + suffix.size() == total_bytes;
}

std::vector<std::uint8_t> ProcessHarness::ByteCapture::ExactBytes() const
{
    if (!exact())
        throw std::logic_error("byte capture is truncated");
    std::vector<std::uint8_t> result;
    result.reserve(prefix.size() + suffix.size());
    result.insert(result.end(), prefix.begin(), prefix.end());
    result.insert(result.end(), suffix.begin(), suffix.end());
    return result;
}

namespace
{
std::uint64_t Omitted(ProcessHarness::ByteCapture const& bytes)
{
    const auto retained = static_cast<std::uint64_t>(bytes.prefix.size() + bytes.suffix.size());
    return bytes.total_bytes > retained ? bytes.total_bytes - retained : 0;
}

void EscapePart(std::ostringstream& out, std::vector<std::uint8_t> const& bytes)
{
    static char const hex[] = "0123456789abcdef";
    for (const std::uint8_t byte : bytes)
    {
        switch (byte)
        {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\0': out << "\\0"; break;
        case '\t': out << "\\t"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        default:
            if (byte >= 0x20 && byte <= 0x7e)
                out << static_cast<char>(byte);
            else
                out << "\\x" << hex[byte >> 4] << hex[byte & 0x0f];
        }
    }
}

void HexPart(std::ostringstream& out, std::vector<std::uint8_t> const& bytes, bool& first)
{
    static char const hex[] = "0123456789abcdef";
    for (const std::uint8_t byte : bytes)
    {
        if (!first)
            out << ' ';
        first = false;
        out << hex[byte >> 4] << hex[byte & 0x0f];
    }
}
} // namespace

std::string ProcessHarness::EscapeBytes(ByteCapture const& bytes)
{
    std::ostringstream out;
    EscapePart(out, bytes.prefix);
    if (bytes.truncated)
        out << "<..." << Omitted(bytes) << " bytes omitted...>";
    EscapePart(out, bytes.suffix);
    return out.str();
}

std::string ProcessHarness::HexBytes(ByteCapture const& bytes)
{
    std::ostringstream out;
    bool first = true;
    HexPart(out, bytes.prefix, first);
    if (bytes.truncated)
    {
        if (!first)
            out << ' ';
        out << "<" << Omitted(bytes) << "-bytes-omitted>";
        first = false;
    }
    HexPart(out, bytes.suffix, first);
    return out.str();
}

char const* ProcessHarness::ToString(Outcome value) noexcept
{
    switch (value)
    {
    case Outcome::Ok: return "ok";
    case Outcome::Timeout: return "timeout";
    case Outcome::Closed: return "closed";
    case Outcome::ProcessExited: return "process-exited";
    case Outcome::ResourceLimit: return "resource-limit";
    case Outcome::OsError: return "os-error";
    case Outcome::InvalidArgument: return "invalid-argument";
    case Outcome::Unsupported: return "unsupported";
    }
    return "unknown";
}

char const* ProcessHarness::ToString(Phase value) noexcept
{
    switch (value)
    {
    case Phase::None: return "none";
    case Phase::Launch: return "launch";
    case Phase::StdinWrite: return "stdin-write";
    case Phase::Response: return "response";
    case Phase::GracefulExit: return "graceful-exit";
    case Phase::TotalCase: return "total-case";
    case Phase::HardCleanup: return "hard-cleanup";
    case Phase::Drain: return "drain";
    case Phase::Capture: return "capture";
    }
    return "unknown";
}

char const* ProcessHarness::ToString(Channel value) noexcept
{
    return value == Channel::Stdout ? "stdout" : "stderr";
}

char const* ProcessHarness::ToString(LineEnding value) noexcept
{
    switch (value)
    {
    case LineEnding::None: return "none";
    case LineEnding::Lf: return "lf";
    case LineEnding::CrLf: return "crlf";
    case LineEnding::Cr: return "cr";
    }
    return "unknown";
}

#if defined(_WIN32)

namespace
{
using PH = ProcessHarness;

class Handle
{
public:
    Handle() = default;
    explicit Handle(HANDLE value) : value_(value) {}
    ~Handle() { reset(); }
    Handle(Handle const&) = delete;
    Handle& operator=(Handle const&) = delete;
    Handle(Handle&& other) noexcept : value_(other.release()) {}
    Handle& operator=(Handle&& other) noexcept
    {
        if (this != &other)
            reset(other.release());
        return *this;
    }
    [[nodiscard]] HANDLE get() const noexcept { return value_; }
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return value_ != nullptr && value_ != INVALID_HANDLE_VALUE;
    }
    HANDLE release() noexcept
    {
        HANDLE result = value_;
        value_ = nullptr;
        return result;
    }
    void reset(HANDLE value = nullptr) noexcept
    {
        if (*this)
            CloseHandle(value_);
        value_ = value;
    }
private:
    HANDLE value_{nullptr};
};

struct WinError : std::runtime_error
{
    WinError(std::string message, DWORD value)
        : std::runtime_error(std::move(message)), code(value) {}
    DWORD code;
};

[[noreturn]] void ThrowLast(std::string const& operation)
{
    throw WinError(operation + " failed", GetLastError());
}

std::wstring Wide(std::string const& utf8)
{
    if (utf8.empty())
        return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(),
        static_cast<int>(utf8.size()), nullptr, 0);
    if (count <= 0)
        ThrowLast("MultiByteToWideChar");
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(),
            static_cast<int>(utf8.size()), result.data(), count) != count)
        ThrowLast("MultiByteToWideChar");
    return result;
}

std::string Utf8(std::wstring const& wide)
{
    if (wide.empty())
        return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(),
        static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0)
        ThrowLast("WideCharToMultiByte");
    std::string result(static_cast<std::size_t>(count), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(),
            static_cast<int>(wide.size()), result.data(), count, nullptr, nullptr) != count)
        ThrowLast("WideCharToMultiByte");
    return result;
}

std::optional<std::wstring> EnvironmentValue(std::wstring const& name)
{
    SetLastError(ERROR_SUCCESS);
    const DWORD needed = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
    if (needed == 0)
    {
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
            return std::nullopt;
        return std::wstring{};
    }
    std::wstring value(static_cast<std::size_t>(needed - 1), L'\0');
    if (GetEnvironmentVariableW(name.c_str(), value.data(), needed) != needed - 1)
        ThrowLast("GetEnvironmentVariableW");
    return value;
}

std::wstring QuoteArgument(std::wstring const& value)
{
    if (!value.empty() && value.find_first_of(L" \t\"") == std::wstring::npos)
        return value;
    std::wstring result(1, L'\"');
    std::size_t slashes = 0;
    for (const wchar_t ch : value)
    {
        if (ch == L'\\')
        {
            ++slashes;
            continue;
        }
        if (ch == L'\"')
        {
            result.append(slashes * 2 + 1, L'\\');
            result.push_back(L'\"');
        }
        else
        {
            result.append(slashes, L'\\');
            result.push_back(ch);
        }
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}

struct CaseInsensitiveLess
{
    bool operator()(std::wstring const& lhs, std::wstring const& rhs) const
    {
        return CompareStringOrdinal(lhs.data(), static_cast<int>(lhs.size()), rhs.data(),
            static_cast<int>(rhs.size()), TRUE) == CSTR_LESS_THAN;
    }
};

std::string Architecture()
{
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64: return "x86_64";
    case PROCESSOR_ARCHITECTURE_ARM64: return "arm64";
    case PROCESSOR_ARCHITECTURE_INTEL: return "x86";
    default: return "unknown";
    }
}

std::string LocaleName()
{
    std::array<wchar_t, LOCALE_NAME_MAX_LENGTH> name{};
    const int count = GetUserDefaultLocaleName(name.data(), static_cast<int>(name.size()));
    return count > 1 ? Utf8(std::wstring(name.data(), static_cast<std::size_t>(count - 1))) : "unknown";
}

std::string Sha256(std::filesystem::path const& path)
{
    Handle file(CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
    if (!file)
        ThrowLast("CreateFileW(SHA-256)");

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    auto cleanup = [&]() {
        if (hash)
            BCryptDestroyHash(hash);
        if (algorithm)
            BCryptCloseAlgorithmProvider(algorithm, 0);
    };
    NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status < 0)
        throw WinError("BCryptOpenAlgorithmProvider failed", static_cast<DWORD>(status));
    DWORD object_size = 0;
    DWORD result_size = 0;
    status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &result_size, 0);
    if (status < 0)
    {
        cleanup();
        throw WinError("BCryptGetProperty failed", static_cast<DWORD>(status));
    }
    std::vector<UCHAR> object(object_size);
    status = BCryptCreateHash(algorithm, &hash, object.data(), object_size, nullptr, 0, 0);
    if (status < 0)
    {
        cleanup();
        throw WinError("BCryptCreateHash failed", static_cast<DWORD>(status));
    }
    std::array<UCHAR, 65536> bytes{};
    for (;;)
    {
        DWORD read = 0;
        if (!ReadFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr))
        {
            const DWORD error = GetLastError();
            cleanup();
            throw WinError("ReadFile(SHA-256) failed", error);
        }
        if (read == 0)
            break;
        status = BCryptHashData(hash, bytes.data(), read, 0);
        if (status < 0)
        {
            cleanup();
            throw WinError("BCryptHashData failed", static_cast<DWORD>(status));
        }
    }
    std::array<UCHAR, 32> digest{};
    status = BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0);
    cleanup();
    if (status < 0)
        throw WinError("BCryptFinishHash failed", static_cast<DWORD>(status));
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const UCHAR byte : digest)
        out << std::setw(2) << static_cast<unsigned>(byte);
    return out.str();
}

DWORD WaitMilliseconds(PH::Clock::time_point deadline)
{
    const auto now = PH::Clock::now();
    if (now >= deadline)
        return 0;
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - now).count();
    const auto rounded = (ns + 999999) / 1000000;
    return static_cast<DWORD>(std::min<std::int64_t>(rounded, 0xfffffffeLL));
}

struct JobProcessCount
{
    bool available{};
    DWORD active{};
    DWORD error{};
};

JobProcessCount QueryActiveJobProcesses(HANDLE job) noexcept
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting{};
    if (!QueryInformationJobObject(job, JobObjectBasicAccountingInformation,
            &accounting, sizeof(accounting), nullptr))
        return {false, 0, GetLastError()};
    return {true, accounting.ActiveProcesses, ERROR_SUCCESS};
}

struct JobEmptyResult
{
    bool empty{};
    bool timed_out{};
    DWORD error{};
    DWORD last_active{};
};

JobEmptyResult WaitForJobEmpty(HANDLE job, HANDLE completion_port,
    ULONG_PTR expected_key, PH::Clock::time_point deadline) noexcept
{
    for (;;)
    {
        const JobProcessCount count = QueryActiveJobProcesses(job);
        if (!count.available)
            return {false, false, count.error, 0};
        if (count.active == 0)
            return {true, false, ERROR_SUCCESS, 0};

        DWORD message = 0;
        ULONG_PTR key = 0;
        LPOVERLAPPED payload = nullptr;
        if (!GetQueuedCompletionStatus(completion_port, &message, &key, &payload,
                WaitMilliseconds(deadline)))
        {
            const DWORD error = GetLastError();
            return {false, error == WAIT_TIMEOUT, error, count.active};
        }
        if (key == expected_key && message == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO)
            return {true, false, ERROR_SUCCESS, 0};
    }
}

class BoundedBytes
{
public:
    explicit BoundedBytes(std::size_t limit = 0)
        : limit_(limit), prefix_limit_((limit + 1) / 2), suffix_limit_(limit / 2) {}

    bool Append(std::uint8_t const* data, std::size_t size)
    {
        const bool was_truncated = total_ > limit_;
        total_ = size > std::numeric_limits<std::uint64_t>::max() - total_
            ? std::numeric_limits<std::uint64_t>::max()
            : total_ + size;
        const std::size_t prefix_take = std::min(size, prefix_limit_ - prefix_.size());
        prefix_.insert(prefix_.end(), data, data + prefix_take);
        data += prefix_take;
        size -= prefix_take;
        AppendSuffix(data, size);
        return !was_truncated && total_ > limit_;
    }

    bool Append(std::uint8_t value) { return Append(&value, 1); }

    [[nodiscard]] bool truncated() const noexcept { return total_ > limit_; }
    [[nodiscard]] PH::ByteCapture Snapshot() const
    {
        PH::ByteCapture result;
        result.prefix = prefix_;
        result.total_bytes = total_;
        result.truncated = truncated();
        if (suffix_.size() < suffix_limit_ || suffix_limit_ == 0)
            result.suffix = suffix_;
        else
        {
            result.suffix.reserve(suffix_.size());
            result.suffix.insert(result.suffix.end(), suffix_.begin() + static_cast<std::ptrdiff_t>(suffix_next_), suffix_.end());
            result.suffix.insert(result.suffix.end(), suffix_.begin(), suffix_.begin() + static_cast<std::ptrdiff_t>(suffix_next_));
        }
        return result;
    }

private:
    void AppendSuffix(std::uint8_t const* data, std::size_t size)
    {
        if (suffix_limit_ == 0 || size == 0)
            return;
        if (size >= suffix_limit_)
        {
            suffix_.assign(data + (size - suffix_limit_), data + size);
            suffix_next_ = 0;
            return;
        }
        const std::size_t room = suffix_limit_ - suffix_.size();
        const std::size_t initial = std::min(room, size);
        suffix_.insert(suffix_.end(), data, data + initial);
        data += initial;
        size -= initial;
        if (suffix_.size() == suffix_limit_ && initial != 0)
            suffix_next_ = 0;
        while (size != 0)
        {
            const std::size_t chunk = std::min(size, suffix_limit_ - suffix_next_);
            std::copy(data, data + chunk, suffix_.begin() + static_cast<std::ptrdiff_t>(suffix_next_));
            suffix_next_ = (suffix_next_ + chunk) % suffix_limit_;
            data += chunk;
            size -= chunk;
        }
    }

    std::size_t limit_{};
    std::size_t prefix_limit_{};
    std::size_t suffix_limit_{};
    std::uint64_t total_{};
    std::vector<std::uint8_t> prefix_;
    std::vector<std::uint8_t> suffix_;
    std::size_t suffix_next_{};
};

struct StreamState
{
    explicit StreamState(std::size_t limit) : bytes(limit) {}
    BoundedBytes bytes;
    std::uint64_t lines{};
    std::uint64_t first_overflow{};
    bool eof{};
};

struct SharedState
{
    SharedState(PH::Clock::time_point start, PH::ResourceLimits const& limits)
        : started(start), max_events(limits.max_output_events),
          stdout_state(limits.max_stdout_retained_bytes),
          stderr_state(limits.max_stderr_retained_bytes) {}

    std::mutex mutex;
    std::condition_variable changed;
    PH::Clock::time_point started;
    PH::LaunchRecord launch;
    std::uint64_t next_ordinal{1};
    std::vector<PH::InputEvent> input;
    std::vector<PH::OutputEvent> event_prefix;
    std::deque<PH::OutputEvent> event_suffix;
    std::uint64_t total_events{};
    std::uint64_t first_event_overflow{};
    std::size_t max_events{};
    StreamState stdout_state;
    StreamState stderr_state;
    std::vector<PH::Diagnostic> diagnostics;
    std::optional<PH::Diagnostic> first_resource_failure;
    std::optional<std::uint32_t> exit_code;
    bool stdin_closed{};
    bool process_exited{};
    bool process_group_empty{};
    bool hard_cleanup_attempted{};
    bool hard_cleanup_completed{};
};

PH::Milliseconds Elapsed(SharedState const& state, PH::Clock::time_point when = PH::Clock::now())
{
    return std::chrono::duration_cast<PH::Milliseconds>(when - state.started);
}

PH::Diagnostic DiagnosticLocked(SharedState& state, PH::Outcome outcome, PH::Phase phase,
    std::string message, DWORD error = 0, std::uint64_t ordinal = 0)
{
    PH::Diagnostic result;
    result.outcome = outcome;
    result.phase = phase;
    result.event_ordinal = ordinal == 0 ? state.next_ordinal++ : ordinal;
    result.native_error = error;
    result.elapsed = Elapsed(state);
    result.message = std::move(message);
    state.diagnostics.push_back(result);
    if (outcome == PH::Outcome::ResourceLimit && !state.first_resource_failure)
        state.first_resource_failure = result;
    state.changed.notify_all();
    return result;
}

std::string LeadingToken(PH::ByteCapture const& bytes)
{
    const std::vector<std::uint8_t> exact = bytes.exact()
        ? bytes.ExactBytes() : std::vector<std::uint8_t>{};
    auto const& data = bytes.exact() ? exact : bytes.prefix;
    std::size_t at = 0;
    auto whitespace = [](std::uint8_t value) {
        return value == ' ' || (value >= 0x09 && value <= 0x0d);
    };
    while (at < data.size() && whitespace(data[at]))
        ++at;
    const std::size_t begin = at;
    while (at < data.size() && !whitespace(data[at]))
        ++at;
    return std::string(data.begin() + static_cast<std::ptrdiff_t>(begin),
        data.begin() + static_cast<std::ptrdiff_t>(at));
}

void AddEventLocked(SharedState& state, PH::OutputEvent event)
{
    ++state.total_events;
    const std::size_t prefix_limit = (state.max_events + 1) / 2;
    const std::size_t suffix_limit = state.max_events / 2;
    if (state.event_prefix.size() < prefix_limit)
        state.event_prefix.push_back(std::move(event));
    else if (suffix_limit != 0)
    {
        state.event_suffix.push_back(std::move(event));
        if (state.event_suffix.size() > suffix_limit)
            state.event_suffix.pop_front();
    }
    if (state.total_events == static_cast<std::uint64_t>(state.max_events) + 1)
    {
        const std::uint64_t ordinal = state.event_suffix.empty()
            ? state.next_ordinal - 1 : state.event_suffix.back().ordinal;
        state.first_event_overflow = ordinal;
        DiagnosticLocked(state, PH::Outcome::ResourceLimit, PH::Phase::Capture,
            "output event count exceeded max_output_events", 0, ordinal);
    }
}

struct ReaderControl
{
    ReaderControl(std::shared_ptr<SharedState> shared_state, PH::Channel stream,
        Handle input, std::size_t max_line)
        : shared(std::move(shared_state)), channel(stream), pipe(std::move(input)), line_limit(max_line) {}
    std::shared_ptr<SharedState> shared;
    PH::Channel channel;
    Handle pipe;
    std::size_t line_limit;
};

HANDLE NativeThreadHandle(std::thread& thread)
{
#if defined(__MINGW32__)
    // winpthreads exposes pthread_t as an integer token, not as the waitable
    // Win32 handle expected by WaitForSingleObject/CancelSynchronousIo.
    return static_cast<HANDLE>(pthread_gethandle(thread.native_handle()));
#else
    return thread.native_handle();
#endif
}

void ReaderLoop(std::shared_ptr<ReaderControl> control)
{
    BoundedBytes line(control->line_limit);
    bool pending_cr = false;
    auto record_resource = [&](std::string message) {
        std::lock_guard<std::mutex> lock(control->shared->mutex);
        DiagnosticLocked(*control->shared, PH::Outcome::ResourceLimit, PH::Phase::Capture,
            std::move(message));
    };
    auto emit = [&](PH::LineEnding ending) {
        PH::ByteCapture capture = line.Snapshot();
        PH::OutputEvent event;
        event.channel = control->channel;
        event.line_ending = ending;
        event.leading_token = LeadingToken(capture);
        std::lock_guard<std::mutex> lock(control->shared->mutex);
        StreamState& stream = control->channel == PH::Channel::Stdout
            ? control->shared->stdout_state : control->shared->stderr_state;
        event.ordinal = control->shared->next_ordinal++;
        event.elapsed = Elapsed(*control->shared);
        if (stream.bytes.truncated())
        {
            event.bytes.total_bytes = capture.total_bytes;
            event.bytes.truncated = capture.total_bytes != 0;
            event.payload_omitted_due_to_stream_limit = true;
        }
        else
            event.bytes = std::move(capture);
        ++stream.lines;
        AddEventLocked(*control->shared, std::move(event));
        control->shared->changed.notify_all();
        line = BoundedBytes(control->line_limit);
    };
    auto consume = [&](std::uint8_t value) {
        if (pending_cr)
        {
            if (value == '\n')
            {
                if (line.Append(value))
                    record_resource(std::string(PH::ToString(control->channel)) + " line exceeded max_line_bytes");
                pending_cr = false;
                emit(PH::LineEnding::CrLf);
                return;
            }
            pending_cr = false;
            emit(PH::LineEnding::Cr);
        }
        if (line.Append(value))
            record_resource(std::string(PH::ToString(control->channel)) + " line exceeded max_line_bytes");
        if (value == '\r')
            pending_cr = true;
        else if (value == '\n')
            emit(PH::LineEnding::Lf);
    };

    std::array<std::uint8_t, 16384> buffer{};
    for (;;)
    {
        DWORD count = 0;
        if (!ReadFile(control->pipe.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr))
        {
            const DWORD error = GetLastError();
            if (error != ERROR_BROKEN_PIPE && error != ERROR_OPERATION_ABORTED)
            {
                std::lock_guard<std::mutex> lock(control->shared->mutex);
                DiagnosticLocked(*control->shared, PH::Outcome::OsError, PH::Phase::Drain,
                    std::string("ReadFile(") + PH::ToString(control->channel) + ") failed", error);
            }
            break;
        }
        if (count == 0)
            break;
        {
            std::lock_guard<std::mutex> lock(control->shared->mutex);
            StreamState& stream = control->channel == PH::Channel::Stdout
                ? control->shared->stdout_state : control->shared->stderr_state;
            if (stream.bytes.Append(buffer.data(), count))
            {
                PH::Diagnostic d = DiagnosticLocked(*control->shared, PH::Outcome::ResourceLimit,
                    PH::Phase::Capture,
                    std::string(PH::ToString(control->channel)) + " exceeded retained-byte ceiling");
                stream.first_overflow = d.event_ordinal;
            }
        }
        for (DWORD index = 0; index < count; ++index)
            consume(buffer[index]);
    }
    if (pending_cr)
        emit(PH::LineEnding::Cr);
    else if (line.Snapshot().total_bytes != 0)
        emit(PH::LineEnding::None);
    {
        std::lock_guard<std::mutex> lock(control->shared->mutex);
        StreamState& stream = control->channel == PH::Channel::Stdout
            ? control->shared->stdout_state : control->shared->stderr_state;
        stream.eof = true;
        control->shared->changed.notify_all();
    }
}

} // namespace

class ProcessHarness::Impl
{
public:
    explicit Impl(LaunchOptions options)
        : options_(std::move(options)), state_(std::make_shared<SharedState>(Clock::now(), options_.limits)),
          total_deadline_(state_->started + options_.watchdogs.total_case)
    {
        try
        {
            Validate();
            Start();
        }
        catch (WinError const& error)
        {
            FailLaunch(Outcome::OsError, error.what(), error.code);
        }
        catch (std::exception const& error)
        {
            FailLaunch(Outcome::InvalidArgument, error.what(), 0);
        }
    }

    ~Impl() { (void)Terminate(); }

    [[nodiscard]] bool started() const noexcept { return started_; }
    [[nodiscard]] std::uint64_t pid() const noexcept { return state_->launch.process_id; }
    [[nodiscard]] std::optional<Diagnostic> launch_diagnostic() const { return launch_failure_; }

    SendResult Send(std::uint8_t const* bytes, std::size_t size)
    {
        std::unique_lock<std::mutex> operation(operation_mutex_);
        SendResult result;
        result.started_at = Clock::now();
        if (!started_ || cleanup_done_)
            return FailedSend(Outcome::Closed, Phase::StdinWrite, "process is not writable", 0, result.started_at);

        InputEvent event;
        event.kind = InputKind::Send;
        event.total_bytes = size;
        event.started_elapsed = Elapsed(*state_, result.started_at);
        if (size > options_.limits.max_stdin_event_bytes)
        {
            event.bytes_omitted_due_to_limit = true;
            event.outcome = Outcome::ResourceLimit;
            Diagnostic diagnostic;
            {
                std::lock_guard<std::mutex> lock(state_->mutex);
                event.ordinal = state_->next_ordinal++;
                state_->input.push_back(event);
                diagnostic = DiagnosticLocked(*state_, Outcome::ResourceLimit, Phase::Capture,
                    "SEND exceeded max_stdin_event_bytes", 0, event.ordinal);
            }
            result.outcome = Outcome::ResourceLimit;
            result.ordinal = event.ordinal;
            result.diagnostic = diagnostic;
            return result;
        }
        if (size != 0)
            event.bytes.assign(bytes, bytes + size);
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            event.ordinal = state_->next_ordinal++;
            state_->input.push_back(event);
        }
        result.ordinal = event.ordinal;

        if (!stdin_write_)
            return FinishSendFailure(result, Outcome::Closed, Phase::StdinWrite, "stdin is closed", 0);

        HANDLE duplicate = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), stdin_write_.get(), GetCurrentProcess(), &duplicate,
                0, FALSE, DUPLICATE_SAME_ACCESS))
            return FinishSendFailure(result, Outcome::OsError, Phase::StdinWrite,
                "DuplicateHandle(stdin) failed", GetLastError());

        struct WriteState
        {
            Handle handle;
            std::vector<std::uint8_t> bytes;
            std::mutex mutex;
            std::condition_variable changed;
            bool done{};
            std::atomic<std::size_t> written{};
            std::atomic<DWORD> error{};
        };
        auto write = std::make_shared<WriteState>();
        write->handle = Handle(duplicate);
        if (size != 0)
            write->bytes.assign(bytes, bytes + size);
        std::thread worker([write] {
            std::size_t offset = 0;
            DWORD terminal_error = ERROR_SUCCESS;
            while (offset < write->bytes.size())
            {
                DWORD count = 0;
                const DWORD wanted = static_cast<DWORD>(std::min<std::size_t>(
                    write->bytes.size() - offset, std::numeric_limits<DWORD>::max()));
                if (!WriteFile(write->handle.get(), write->bytes.data() + offset,
                        wanted, &count, nullptr))
                {
                    terminal_error = GetLastError();
                    break;
                }
                if (count == 0)
                {
                    terminal_error = ERROR_WRITE_FAULT;
                    break;
                }
                offset += count;
                write->written.store(offset, std::memory_order_release);
            }
            write->error.store(terminal_error, std::memory_order_release);
            write->handle.reset();
            {
                std::lock_guard<std::mutex> lock(write->mutex);
                write->done = true;
            }
            write->changed.notify_all();
        });

        const Clock::time_point write_deadline = result.started_at + options_.watchdogs.stdin_write;
        const Clock::time_point deadline = std::min(write_deadline, total_deadline_);
        bool done = false;
        {
            std::unique_lock<std::mutex> lock(write->mutex);
            done = write->changed.wait_until(lock, deadline, [&] { return write->done; });
        }
        if (!done)
        {
            const Phase phase = total_deadline_ <= write_deadline ? Phase::TotalCase : Phase::StdinWrite;
            const std::string message = phase == Phase::TotalCase
                ? "total-case deadline expired during SEND" : "stdin write deadline expired";
            CancelSynchronousIo(NativeThreadHandle(worker));
            CloseStdinUnlocked();
            if (job_)
                TerminateJobObject(job_.get(), 0x54444641U);
            const Clock::time_point cleanup_deadline = Clock::now() + options_.watchdogs.hard_cleanup;
            if (WaitForSingleObject(NativeThreadHandle(worker), WaitMilliseconds(cleanup_deadline)) == WAIT_OBJECT_0)
                worker.join();
            else
                worker.detach();
            result.bytes_written = write->written.load(std::memory_order_acquire);
            result = FinishSendFailure(result, Outcome::Timeout, phase, message, WAIT_TIMEOUT);
            (void)HardCleanupUnlocked(cleanup_deadline);
            return result;
        }
        worker.join();
        result.bytes_written = write->written.load(std::memory_order_acquire);
        const DWORD write_error = write->error.load(std::memory_order_acquire);
        if (write_error != ERROR_SUCCESS)
        {
            const Outcome outcome = write_error == ERROR_BROKEN_PIPE || write_error == ERROR_NO_DATA
                ? Outcome::Closed : Outcome::OsError;
            return FinishSendFailure(result, outcome, Phase::StdinWrite,
                "WriteFile(stdin) failed", write_error);
        }
        UpdateInput(result.ordinal, Outcome::Ok, 0);
        result.outcome = Outcome::Ok;
        return result;
    }

    OperationResult CloseStdin()
    {
        std::lock_guard<std::mutex> operation(operation_mutex_);
        if (!started_)
            return Failure(Outcome::Closed, Phase::StdinWrite, "process was not started");
        CloseStdinUnlocked();
        return {};
    }

    LineResult WaitLine(SendResult const& send, LinePredicate predicate, std::uint64_t after)
    {
        std::unique_lock<std::mutex> operation(operation_mutex_);
        LineResult result;
        if (!predicate)
        {
            auto failure = AddDiagnostic(Outcome::InvalidArgument, Phase::Response, "line predicate is empty");
            result.outcome = failure.outcome;
            result.diagnostic = failure;
            return result;
        }
        if (send.outcome != Outcome::Ok || send.ordinal == 0)
        {
            auto failure = AddDiagnostic(Outcome::InvalidArgument, Phase::Response,
                "response wait requires a successful SEND receipt");
            result.outcome = failure.outcome;
            result.diagnostic = failure;
            return result;
        }
        const Clock::time_point response_deadline = send.started_at + options_.watchdogs.response;
        const Clock::time_point deadline = std::min(response_deadline, total_deadline_);
        std::uint64_t cursor = std::max(send.ordinal, after);
        for (;;)
        {
            std::vector<OutputEvent> candidates;
            std::optional<Diagnostic> resource;
            bool eof = false;
            {
                std::unique_lock<std::mutex> lock(state_->mutex);
                resource = state_->first_resource_failure;
                auto collect = [&](auto const& container) {
                    for (auto const& event : container)
                        if (event.ordinal > cursor)
                            candidates.push_back(event);
                };
                collect(state_->event_prefix);
                collect(state_->event_suffix);
                eof = state_->stdout_state.eof && state_->stderr_state.eof;
                if (candidates.empty() && !resource && !eof && Clock::now() < deadline)
                {
                    state_->changed.wait_until(lock, deadline);
                    continue;
                }
            }
            if (resource)
            {
                result.outcome = Outcome::ResourceLimit;
                result.diagnostic = resource;
                (void)HardCleanupUnlocked(Clock::now() + options_.watchdogs.hard_cleanup);
                return result;
            }
            std::sort(candidates.begin(), candidates.end(),
                [](OutputEvent const& a, OutputEvent const& b) { return a.ordinal < b.ordinal; });
            for (auto const& event : candidates)
            {
                cursor = std::max(cursor, event.ordinal);
                if (predicate(event))
                {
                    result.outcome = Outcome::Ok;
                    result.event = event;
                    return result;
                }
            }
            if (eof)
            {
                auto failure = AddDiagnostic(Outcome::ProcessExited, Phase::Response,
                    "both output streams reached EOF before a matching line");
                result.outcome = failure.outcome;
                result.diagnostic = failure;
                return result;
            }
            if (Clock::now() >= deadline)
            {
                const Phase phase = total_deadline_ <= response_deadline ? Phase::TotalCase : Phase::Response;
                auto failure = AddDiagnostic(Outcome::Timeout, phase,
                    phase == Phase::TotalCase ? "total-case deadline expired while waiting for output"
                                              : "response deadline expired");
                result.outcome = failure.outcome;
                result.diagnostic = failure;
                (void)HardCleanupUnlocked(Clock::now() + options_.watchdogs.hard_cleanup);
                return result;
            }
        }
    }

    ExitResult WaitExit()
    {
        std::unique_lock<std::mutex> operation(operation_mutex_);
        ExitResult result;
        if (!started_ || !process_)
        {
            auto failure = AddDiagnostic(Outcome::Closed, Phase::GracefulExit, "process is unavailable");
            result.outcome = failure.outcome;
            result.diagnostic = failure;
            return result;
        }
        const Clock::time_point phase_deadline = Clock::now() + options_.watchdogs.graceful_exit;
        const Clock::time_point deadline = std::min(phase_deadline, total_deadline_);
        const DWORD wait = WaitForSingleObject(process_.get(), WaitMilliseconds(deadline));
        if (wait != WAIT_OBJECT_0)
        {
            const Phase phase = total_deadline_ <= phase_deadline ? Phase::TotalCase : Phase::GracefulExit;
            auto failure = AddDiagnostic(wait == WAIT_TIMEOUT ? Outcome::Timeout : Outcome::OsError, phase,
                wait == WAIT_TIMEOUT ? "process exit deadline expired" : "WaitForSingleObject(process) failed",
                wait == WAIT_TIMEOUT ? WAIT_TIMEOUT : GetLastError());
            result.outcome = failure.outcome;
            result.diagnostic = failure;
            (void)HardCleanupUnlocked(Clock::now() + options_.watchdogs.hard_cleanup);
            return result;
        }
        DWORD code = 0;
        if (!GetExitCodeProcess(process_.get(), &code))
        {
            auto failure = AddDiagnostic(Outcome::OsError, Phase::GracefulExit,
                "GetExitCodeProcess failed", GetLastError());
            result.outcome = failure.outcome;
            result.diagnostic = failure;
            (void)HardCleanupUnlocked(Clock::now() + options_.watchdogs.hard_cleanup);
            return result;
        }
        MarkExited(code);
        if (!JoinReader(stdout_reader_, deadline) || !JoinReader(stderr_reader_, deadline))
        {
            auto failure = AddDiagnostic(Outcome::Timeout, Phase::Drain,
                "output drain did not finish before the graceful deadline");
            result.outcome = failure.outcome;
            result.diagnostic = failure;
            (void)HardCleanupUnlocked(Clock::now() + options_.watchdogs.hard_cleanup);
            return result;
        }
        if (job_)
        {
            const JobEmptyResult group = WaitForJobEmpty(job_.get(), job_completion_.get(),
                reinterpret_cast<ULONG_PTR>(this), deadline);
            if (!group.empty)
            {
                auto failure = AddDiagnostic(
                    group.timed_out ? Outcome::Timeout : Outcome::OsError,
                    Phase::GracefulExit,
                    group.timed_out
                        ? "process-group exit deadline expired with "
                            + std::to_string(group.last_active) + " active process(es)"
                        : "process-group accounting/completion wait failed",
                    group.error);
                result.outcome = failure.outcome;
                result.diagnostic = failure;
                (void)HardCleanupUnlocked(Clock::now() + options_.watchdogs.hard_cleanup);
                return result;
            }
            std::lock_guard<std::mutex> lock(state_->mutex);
            state_->process_group_empty = true;
        }
        CloseStdinUnlocked();
        job_.reset();
        job_completion_.reset();
        process_.reset();
        cleanup_done_ = true;
        result.outcome = Outcome::Ok;
        result.exit_code = code;
        return result;
    }

    OperationResult Terminate()
    {
        std::lock_guard<std::mutex> operation(operation_mutex_);
        return HardCleanupUnlocked(Clock::now() + options_.watchdogs.hard_cleanup);
    }

    Transcript Snapshot() const
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        Transcript result;
        result.launch = state_->launch;
        result.input_events = state_->input;
        result.output_events = state_->event_prefix;
        result.output_events.insert(result.output_events.end(), state_->event_suffix.begin(), state_->event_suffix.end());
        std::sort(result.output_events.begin(), result.output_events.end(),
            [](OutputEvent const& a, OutputEvent const& b) { return a.ordinal < b.ordinal; });
        result.stdout_stream.bytes = state_->stdout_state.bytes.Snapshot();
        result.stdout_stream.line_events = state_->stdout_state.lines;
        result.stdout_stream.first_overflow_ordinal = state_->stdout_state.first_overflow;
        result.stdout_stream.eof = state_->stdout_state.eof;
        result.stderr_stream.bytes = state_->stderr_state.bytes.Snapshot();
        result.stderr_stream.line_events = state_->stderr_state.lines;
        result.stderr_stream.first_overflow_ordinal = state_->stderr_state.first_overflow;
        result.stderr_stream.eof = state_->stderr_state.eof;
        result.diagnostics = state_->diagnostics;
        result.total_output_events = state_->total_events;
        result.first_output_event_overflow_ordinal = state_->first_event_overflow;
        result.output_events_truncated = state_->total_events > state_->max_events;
        result.exit_code = state_->exit_code;
        result.stdin_closed = state_->stdin_closed;
        result.process_exited = state_->process_exited;
        result.process_group_empty = state_->process_group_empty;
        result.hard_cleanup_attempted = state_->hard_cleanup_attempted;
        result.hard_cleanup_completed = state_->hard_cleanup_completed;
        return result;
    }

private:
    struct Reader
    {
        std::shared_ptr<ReaderControl> control;
        std::thread thread;
    };

    void Validate()
    {
        if (options_.executable.empty())
            throw std::invalid_argument("executable path is empty (set TDFA_ENGINE_PATH or LaunchOptions::executable)");
        auto positive = [](Milliseconds value) { return value.count() > 0; };
        if (!positive(options_.watchdogs.stdin_write) || !positive(options_.watchdogs.response)
            || !positive(options_.watchdogs.graceful_exit) || !positive(options_.watchdogs.total_case)
            || !positive(options_.watchdogs.hard_cleanup))
            throw std::invalid_argument("all watchdog ceilings must be positive");
    }

    void Start()
    {
        options_.executable = std::filesystem::canonical(options_.executable);
        const std::filesystem::path working = options_.working_directory
            ? std::filesystem::canonical(*options_.working_directory)
            : std::filesystem::current_path();

        state_->launch.executable = options_.executable;
        state_->launch.executable_sha256 = Sha256(options_.executable);
        state_->launch.argument_vector.push_back(Utf8(options_.executable.native()));
        state_->launch.argument_vector.insert(state_->launch.argument_vector.end(),
            options_.arguments.begin(), options_.arguments.end());
        state_->launch.working_directory = working;
        state_->launch.locale = LocaleName();
        state_->launch.operating_system = "Windows";
        state_->launch.architecture = Architecture();
#if defined(_DEBUG)
        state_->launch.build_mode = "debug";
#else
        state_->launch.build_mode = "release-or-unspecified";
#endif
        state_->launch.instrumentation = options_.instrumentation;

        std::map<std::wstring, std::wstring, CaseInsensitiveLess> environment;
        for (auto const& name_utf8 : options_.inherited_environment_names)
        {
            const std::wstring name = Wide(name_utf8);
            if (name.empty() || name.find(L'=') != std::wstring::npos)
                throw std::invalid_argument("invalid inherited environment name");
            if (auto value = EnvironmentValue(name))
                environment[name] = *value;
        }
        for (auto const& entry : options_.environment_overrides)
        {
            const std::wstring name = Wide(entry.name);
            if (name.empty() || name.find(L'=') != std::wstring::npos)
                throw std::invalid_argument("invalid environment override name");
            environment[name] = Wide(entry.value);
        }
        std::vector<wchar_t> environment_block;
        for (auto const& entry : environment)
        {
            state_->launch.inherited_environment.push_back({Utf8(entry.first), Utf8(entry.second)});
            const std::wstring item = entry.first + L"=" + entry.second;
            environment_block.insert(environment_block.end(), item.begin(), item.end());
            environment_block.push_back(L'\0');
        }
        environment_block.push_back(L'\0');
        if (environment.empty())
            environment_block.push_back(L'\0');

        SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
        Handle child_stdin;
        Handle parent_stdin;
        Handle parent_stdout;
        Handle child_stdout;
        Handle parent_stderr;
        Handle child_stderr;
        HANDLE first = nullptr;
        HANDLE second = nullptr;
        if (!CreatePipe(&first, &second, &security, 65536)) ThrowLast("CreatePipe(stdin)");
        child_stdin = Handle(first); parent_stdin = Handle(second);
        if (!CreatePipe(&first, &second, &security, 65536)) ThrowLast("CreatePipe(stdout)");
        parent_stdout = Handle(first); child_stdout = Handle(second);
        if (!CreatePipe(&first, &second, &security, 65536)) ThrowLast("CreatePipe(stderr)");
        parent_stderr = Handle(first); child_stderr = Handle(second);
        if (!SetHandleInformation(parent_stdin.get(), HANDLE_FLAG_INHERIT, 0)
            || !SetHandleInformation(parent_stdout.get(), HANDLE_FLAG_INHERIT, 0)
            || !SetHandleInformation(parent_stderr.get(), HANDLE_FLAG_INHERIT, 0))
            ThrowLast("SetHandleInformation");

        Handle job(CreateJobObjectW(nullptr, nullptr));
        if (!job) ThrowLast("CreateJobObjectW");
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info{};
        job_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation,
                &job_info, sizeof(job_info)))
            ThrowLast("SetInformationJobObject");
        Handle job_completion(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1));
        if (!job_completion)
            ThrowLast("CreateIoCompletionPort(job)");
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT completion_info{};
        completion_info.CompletionKey = this;
        completion_info.CompletionPort = job_completion.get();
        if (!SetInformationJobObject(job.get(), JobObjectAssociateCompletionPortInformation,
                &completion_info, sizeof(completion_info)))
            ThrowLast("SetInformationJobObject(completion port)");

        SIZE_T attribute_size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_size);
        std::vector<std::uint8_t> attribute_storage(attribute_size);
        auto* attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attribute_storage.data());
        if (!InitializeProcThreadAttributeList(attributes, 1, 0, &attribute_size))
            ThrowLast("InitializeProcThreadAttributeList");
        struct AttributeGuard
        {
            LPPROC_THREAD_ATTRIBUTE_LIST value;
            ~AttributeGuard() { DeleteProcThreadAttributeList(value); }
        } attribute_guard{attributes};
        HANDLE inherited[] = {child_stdin.get(), child_stdout.get(), child_stderr.get()};
        if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                inherited, sizeof(inherited), nullptr, nullptr))
            ThrowLast("UpdateProcThreadAttribute");

        STARTUPINFOEXW startup{};
        startup.StartupInfo.cb = sizeof(startup);
        startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        startup.StartupInfo.hStdInput = child_stdin.get();
        startup.StartupInfo.hStdOutput = child_stdout.get();
        startup.StartupInfo.hStdError = child_stderr.get();
        startup.lpAttributeList = attributes;

        std::wstring command = QuoteArgument(options_.executable.native());
        for (auto const& argument : options_.arguments)
            command += L" " + QuoteArgument(Wide(argument));
        std::vector<wchar_t> mutable_command(command.begin(), command.end());
        mutable_command.push_back(L'\0');
        PROCESS_INFORMATION process_info{};
        constexpr DWORD flags = CREATE_SUSPENDED | CREATE_NO_WINDOW
            | CREATE_UNICODE_ENVIRONMENT | EXTENDED_STARTUPINFO_PRESENT;
        if (!CreateProcessW(options_.executable.c_str(), mutable_command.data(), nullptr, nullptr,
                TRUE, flags, environment_block.data(), working.c_str(),
                &startup.StartupInfo, &process_info))
            ThrowLast("CreateProcessW");
        process_ = Handle(process_info.hProcess);
        process_thread_ = Handle(process_info.hThread);
        child_stdin.reset(); child_stdout.reset(); child_stderr.reset();
        if (!AssignProcessToJobObject(job.get(), process_.get()))
        {
            const DWORD error = GetLastError();
            TerminateProcess(process_.get(), 0x54444641U);
            WaitForSingleObject(process_.get(), 10000);
            throw WinError("AssignProcessToJobObject failed", error);
        }
        job_ = std::move(job);
        job_completion_ = std::move(job_completion);
        stdin_write_ = std::move(parent_stdin);
        state_->launch.process_id = process_info.dwProcessId;
        state_->launch.job_identity = reinterpret_cast<std::uintptr_t>(job_.get());
        state_->launch.creation_flags = flags;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            InputEvent start;
            start.ordinal = state_->next_ordinal++;
            start.kind = InputKind::Start;
            start.started_elapsed = Elapsed(*state_);
            start.completed_elapsed = start.started_elapsed;
            state_->input.push_back(start);
        }
        stdout_reader_.control = std::make_shared<ReaderControl>(state_, Channel::Stdout,
            std::move(parent_stdout), options_.limits.max_line_bytes);
        stderr_reader_.control = std::make_shared<ReaderControl>(state_, Channel::Stderr,
            std::move(parent_stderr), options_.limits.max_line_bytes);
        stdout_reader_.thread = std::thread(ReaderLoop, stdout_reader_.control);
        stderr_reader_.thread = std::thread(ReaderLoop, stderr_reader_.control);
        if (ResumeThread(process_thread_.get()) == static_cast<DWORD>(-1))
            ThrowLast("ResumeThread");
        process_thread_.reset();
        started_ = true;
    }

    void FailLaunch(Outcome outcome, std::string message, DWORD error)
    {
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            launch_failure_ = DiagnosticLocked(*state_, outcome, Phase::Launch, std::move(message), error);
        }
        if (process_ || job_ || stdout_reader_.thread.joinable() || stderr_reader_.thread.joinable())
            (void)HardCleanupUnlocked(Clock::now() + options_.watchdogs.hard_cleanup);
    }

    Diagnostic AddDiagnostic(Outcome outcome, Phase phase, std::string message, DWORD error = 0)
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return DiagnosticLocked(*state_, outcome, phase, std::move(message), error);
    }

    OperationResult Failure(Outcome outcome, Phase phase, std::string message, DWORD error = 0)
    {
        OperationResult result;
        result.outcome = outcome;
        result.diagnostic = AddDiagnostic(outcome, phase, std::move(message), error);
        return result;
    }

    SendResult FailedSend(Outcome outcome, Phase phase, std::string message, DWORD error,
        Clock::time_point started)
    {
        SendResult result;
        result.started_at = started;
        result.outcome = outcome;
        result.diagnostic = AddDiagnostic(outcome, phase, std::move(message), error);
        return result;
    }

    SendResult FinishSendFailure(SendResult result, Outcome outcome, Phase phase,
        std::string message, DWORD error)
    {
        UpdateInput(result.ordinal, outcome, error);
        result.outcome = outcome;
        result.diagnostic = AddDiagnostic(outcome, phase, std::move(message), error);
        return result;
    }

    void UpdateInput(std::uint64_t ordinal, Outcome outcome, DWORD error)
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        for (auto& event : state_->input)
        {
            if (event.ordinal == ordinal)
            {
                event.outcome = outcome;
                event.native_error = error;
                event.completed_elapsed = Elapsed(*state_);
                break;
            }
        }
    }

    void CloseStdinUnlocked()
    {
        if (!stdin_write_)
            return;
        stdin_write_.reset();
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->stdin_closed = true;
        InputEvent event;
        event.ordinal = state_->next_ordinal++;
        event.kind = InputKind::CloseStdin;
        event.started_elapsed = Elapsed(*state_);
        event.completed_elapsed = event.started_elapsed;
        state_->input.push_back(event);
        state_->changed.notify_all();
    }

    void MarkExited(DWORD code)
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->process_exited = true;
        state_->exit_code = code;
        state_->changed.notify_all();
    }

    bool JoinReader(Reader& reader, Clock::time_point deadline)
    {
        if (!reader.thread.joinable())
            return true;
        if (WaitForSingleObject(NativeThreadHandle(reader.thread), WaitMilliseconds(deadline)) != WAIT_OBJECT_0)
            return false;
        reader.thread.join();
        reader.control.reset();
        return true;
    }

    bool StopReader(Reader& reader, Clock::time_point deadline)
    {
        if (!reader.thread.joinable())
            return true;
        const DWORD wait = WaitForSingleObject(NativeThreadHandle(reader.thread), WaitMilliseconds(deadline));
        if (wait == WAIT_OBJECT_0)
        {
            reader.thread.join();
            reader.control.reset();
            return true;
        }
        CancelSynchronousIo(NativeThreadHandle(reader.thread));
        reader.thread.detach();
        reader.control.reset();
        return false;
    }

    OperationResult HardCleanupUnlocked(Clock::time_point deadline)
    {
        if (cleanup_done_)
            return {};
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            state_->hard_cleanup_attempted = true;
        }
        CloseStdinUnlocked();
        bool process_done = true;
        bool group_done = true;
        if (job_)
            (void)TerminateJobObject(job_.get(), 0x54444641U);
        if (process_)
        {
            process_done = WaitForSingleObject(process_.get(), WaitMilliseconds(deadline)) == WAIT_OBJECT_0;
            if (process_done)
            {
                DWORD code = 0;
                if (GetExitCodeProcess(process_.get(), &code))
                    MarkExited(code);
            }
        }
        const bool stdout_done = StopReader(stdout_reader_, deadline);
        const bool stderr_done = StopReader(stderr_reader_, deadline);
        if (job_)
        {
            const JobEmptyResult group = WaitForJobEmpty(job_.get(), job_completion_.get(),
                reinterpret_cast<ULONG_PTR>(this), deadline);
            group_done = group.empty;
            {
                std::lock_guard<std::mutex> lock(state_->mutex);
                state_->process_group_empty = group_done;
            }
            job_.reset(); // KILL_ON_JOB_CLOSE is the backstop.
            job_completion_.reset();
        }
        process_.reset();
        process_thread_.reset();
        cleanup_done_ = true;
        const bool complete = group_done && process_done && stdout_done && stderr_done;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            state_->hard_cleanup_completed = complete;
        }
        if (!complete)
            return Failure(Outcome::Timeout, Phase::HardCleanup,
                "hard cleanup deadline expired; a cancelled reader was detached", WAIT_TIMEOUT);
        return {};
    }

    LaunchOptions options_;
    std::shared_ptr<SharedState> state_;
    Clock::time_point total_deadline_;
    mutable std::mutex operation_mutex_;
    Handle process_;
    Handle process_thread_;
    Handle job_;
    Handle job_completion_;
    Handle stdin_write_;
    Reader stdout_reader_;
    Reader stderr_reader_;
    std::optional<Diagnostic> launch_failure_;
    bool started_{};
    bool cleanup_done_{};
};

bool ProcessHarness::IsSupported() noexcept { return true; }

ProcessHarness::LaunchOptions ProcessHarness::FromEnvironment(std::string const& variable)
{
    LaunchOptions result;
    try
    {
        if (auto value = EnvironmentValue(Wide(variable)))
            result.executable = *value;
    }
    catch (...)
    {
        // Constructor validation supplies a stable diagnostic if lookup failed.
    }
    return result;
}

#else

class ProcessHarness::Impl
{
public:
    explicit Impl(LaunchOptions options)
    {
        transcript_.launch.executable = std::move(options.executable);
        Diagnostic diagnostic;
        diagnostic.outcome = Outcome::Unsupported;
        diagnostic.phase = Phase::Launch;
        diagnostic.message = "ProcessHarness currently requires Windows";
        launch_ = diagnostic;
        transcript_.diagnostics.push_back(diagnostic);
    }
    [[nodiscard]] bool started() const noexcept { return false; }
    [[nodiscard]] std::uint64_t pid() const noexcept { return 0; }
    [[nodiscard]] std::optional<Diagnostic> launch_diagnostic() const { return launch_; }
    SendResult Send(std::uint8_t const*, std::size_t)
    {
        SendResult result; result.outcome = Outcome::Unsupported; result.diagnostic = launch_; return result;
    }
    OperationResult CloseStdin() { return Unsupported(); }
    LineResult WaitLine(SendResult const&, LinePredicate, std::uint64_t)
    {
        LineResult result; result.outcome = Outcome::Unsupported; result.diagnostic = launch_; return result;
    }
    ExitResult WaitExit()
    {
        ExitResult result; result.outcome = Outcome::Unsupported; result.diagnostic = launch_; return result;
    }
    OperationResult Terminate() { return Unsupported(); }
    Transcript Snapshot() const { return transcript_; }
private:
    OperationResult Unsupported()
    {
        OperationResult result; result.outcome = Outcome::Unsupported; result.diagnostic = launch_; return result;
    }
    Transcript transcript_;
    std::optional<Diagnostic> launch_;
};

bool ProcessHarness::IsSupported() noexcept { return false; }

ProcessHarness::LaunchOptions ProcessHarness::FromEnvironment(std::string const& variable)
{
    LaunchOptions result;
    if (char const* value = std::getenv(variable.c_str()))
        result.executable = value;
    return result;
}

#endif

ProcessHarness::ProcessHarness(LaunchOptions options)
    : impl_(std::make_unique<Impl>(std::move(options))) {}

ProcessHarness::~ProcessHarness() = default;
ProcessHarness::ProcessHarness(ProcessHarness&&) noexcept = default;
ProcessHarness& ProcessHarness::operator=(ProcessHarness&&) noexcept = default;

bool ProcessHarness::started() const noexcept { return impl_ && impl_->started(); }

std::optional<ProcessHarness::Diagnostic> ProcessHarness::launch_diagnostic() const
{
    return impl_ ? impl_->launch_diagnostic() : std::nullopt;
}

std::uint64_t ProcessHarness::process_id() const noexcept { return impl_ ? impl_->pid() : 0; }

ProcessHarness::SendResult ProcessHarness::Send(std::string_view bytes)
{
    return impl_->Send(reinterpret_cast<std::uint8_t const*>(bytes.data()), bytes.size());
}

ProcessHarness::SendResult ProcessHarness::Send(std::vector<std::uint8_t> const& bytes)
{
    return impl_->Send(bytes.data(), bytes.size());
}

ProcessHarness::OperationResult ProcessHarness::CloseStdin() { return impl_->CloseStdin(); }

ProcessHarness::LineResult ProcessHarness::WaitForLineAfter(
    SendResult const& send, LinePredicate predicate, std::uint64_t after_ordinal)
{
    return impl_->WaitLine(send, std::move(predicate), after_ordinal);
}

ProcessHarness::LineResult ProcessHarness::WaitForTokenAfter(
    SendResult const& send, std::string_view leading_token, Channel channel,
    std::uint64_t after_ordinal)
{
    const std::string token(leading_token);
    return WaitForLineAfter(send, [token, channel](OutputEvent const& event) {
        return event.channel == channel && event.leading_token == token;
    }, after_ordinal);
}

ProcessHarness::ExitResult ProcessHarness::WaitForExit() { return impl_->WaitExit(); }
ProcessHarness::OperationResult ProcessHarness::TerminateAndDrain() { return impl_->Terminate(); }
ProcessHarness::Transcript ProcessHarness::Snapshot() const { return impl_->Snapshot(); }

} // namespace tdfa::test
