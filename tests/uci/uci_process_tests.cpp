#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include "support/ProcessHarness.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef TDFA_ENGINE_PATH
#error "TDFA_ENGINE_PATH must name the built TDFA executable"
#endif

namespace
{

using Harness = tdfa::test::ProcessHarness;
using namespace std::chrono_literals;

constexpr std::string_view kCanonicalUci{"uci\n"};
constexpr std::string_view kCanonicalReady{"isready\n"};
constexpr std::string_view kCanonicalQuit{"quit\n"};

std::string InputKindName(Harness::InputKind kind)
{
  switch (kind)
  {
  case Harness::InputKind::Start:
    return "start";
  case Harness::InputKind::Send:
    return "send";
  case Harness::InputKind::CloseStdin:
    return "close-stdin";
  }
  return "unknown";
}

std::string Bounded(std::string text, std::size_t limit = 512)
{
  if (text.size() <= limit)
    return text;
  const std::size_t retained = limit / 2;
  return text.substr(0, retained) + "<...diagnostic text omitted...>" +
         text.substr(text.size() - retained);
}

Harness::ByteCapture ExactCapture(std::vector<std::uint8_t> const &bytes)
{
  Harness::ByteCapture capture;
  capture.prefix = bytes;
  capture.total_bytes = bytes.size();
  return capture;
}

std::string DumpTranscript(Harness::Transcript const &transcript)
{
  std::ostringstream out;
  out << "launch executable=" << transcript.launch.executable.string()
      << " sha256=" << transcript.launch.executable_sha256
      << " pid=" << transcript.launch.process_id
      << " job=" << transcript.launch.job_identity
      << " cwd=" << transcript.launch.working_directory.string()
      << " os=" << transcript.launch.operating_system
      << " arch=" << transcript.launch.architecture
      << " build=" << transcript.launch.build_mode
      << " instrumentation=" << transcript.launch.instrumentation << '\n';

  for (auto const &event : transcript.input_events)
  {
    out << "input ordinal=" << event.ordinal
        << " kind=" << InputKindName(event.kind)
        << " bytes=" << event.total_bytes
        << " outcome=" << Harness::ToString(event.outcome)
        << " elapsed=" << event.started_elapsed.count() << ".."
        << event.completed_elapsed.count() << "ms";
    if (!event.bytes_omitted_due_to_limit)
      out << " data=\""
          << Bounded(Harness::EscapeBytes(ExactCapture(event.bytes))) << '\"';
    out << '\n';
  }

  constexpr std::size_t kMaxDiagnosticEvents = 64;
  const std::size_t shown =
      std::min(transcript.output_events.size(), kMaxDiagnosticEvents);
  for (std::size_t index = 0; index < shown; ++index)
  {
    auto const &event = transcript.output_events[index];
    out << "output ordinal=" << event.ordinal
        << " channel=" << Harness::ToString(event.channel)
        << " ending=" << Harness::ToString(event.line_ending) << " token=\""
        << Bounded(event.leading_token, 128) << "\""
        << " elapsed=" << event.elapsed.count() << "ms"
        << " bytes=" << event.bytes.total_bytes << " data=\""
        << Bounded(Harness::EscapeBytes(event.bytes)) << "\"\n";
  }
  if (transcript.output_events.size() > shown)
    out << "<" << transcript.output_events.size() - shown
        << " retained output events omitted from diagnostic>\n";

  for (auto const &diagnostic : transcript.diagnostics)
  {
    out << "diagnostic ordinal=" << diagnostic.event_ordinal
        << " outcome=" << Harness::ToString(diagnostic.outcome)
        << " phase=" << Harness::ToString(diagnostic.phase)
        << " native=" << diagnostic.native_error
        << " elapsed=" << diagnostic.elapsed.count() << "ms"
        << " message=\"" << Bounded(diagnostic.message) << "\"\n";
  }

  out << "summary total-output-events=" << transcript.total_output_events
      << " output-events-truncated=" << transcript.output_events_truncated
      << " stdout-bytes=" << transcript.stdout_stream.bytes.total_bytes
      << " stdout-truncated=" << transcript.stdout_stream.bytes.truncated
      << " stdout-eof=" << transcript.stdout_stream.eof
      << " stderr-bytes=" << transcript.stderr_stream.bytes.total_bytes
      << " stderr-truncated=" << transcript.stderr_stream.bytes.truncated
      << " stderr-eof=" << transcript.stderr_stream.eof
      << " stdin-closed=" << transcript.stdin_closed
      << " process-exited=" << transcript.process_exited << " exit-code=";
  if (transcript.exit_code)
    out << *transcript.exit_code;
  else
    out << "none";
  out << " hard-cleanup-attempted=" << transcript.hard_cleanup_attempted
      << " hard-cleanup-completed=" << transcript.hard_cleanup_completed
      << " process-group-empty=" << transcript.process_group_empty;
  return out.str();
}

template <typename Result>
void RequireOk(Harness &harness, Result const &result,
               std::string_view operation)
{
  INFO("operation=" << operation);
  std::ostringstream evidence;
  if (result.diagnostic)
  {
    evidence << "outcome=" << Harness::ToString(result.diagnostic->outcome)
             << " phase=" << Harness::ToString(result.diagnostic->phase)
             << " native=" << result.diagnostic->native_error
             << " diagnostic=" << result.diagnostic->message << '\n';
  }
  if (result.outcome != Harness::Outcome::Ok)
  {
    const auto cleanup = harness.TerminateAndDrain();
    const auto final_transcript = harness.Snapshot();
    evidence << "cleanup-outcome=" << Harness::ToString(cleanup.outcome) << '\n'
             << DumpTranscript(final_transcript);
    INFO(evidence.str());
    CHECK(cleanup.outcome == Harness::Outcome::Ok);
    CHECK(final_transcript.hard_cleanup_attempted);
    CHECK(final_transcript.hard_cleanup_completed);
    CHECK(final_transcript.process_group_empty);
  }
  else
  {
    evidence << DumpTranscript(harness.Snapshot());
  }
  INFO(evidence.str());
  REQUIRE(result.outcome == Harness::Outcome::Ok);
}

Harness MakeHarness()
{
  Harness::LaunchOptions options;
  options.executable = std::filesystem::path{TDFA_ENGINE_PATH};
  options.instrumentation = "ordinary-process/protocol-standard";
  options.watchdogs.stdin_write = 10s;
  options.watchdogs.response = 10s;
  options.watchdogs.graceful_exit = 10s;
  options.watchdogs.total_case = 60s;
  options.watchdogs.hard_cleanup = 30s;
  options.limits.max_stdin_event_bytes = 1024 * 1024;
  options.limits.max_stdout_retained_bytes = 8 * 1024 * 1024;
  options.limits.max_stderr_retained_bytes = 8 * 1024 * 1024;
  options.limits.max_line_bytes = 1024 * 1024;
  options.limits.max_output_events = 100000;

  Harness harness(std::move(options));
  INFO(DumpTranscript(harness.Snapshot()));
  REQUIRE(harness.started());
  return harness;
}

Harness::SendResult SendExact(Harness &harness, std::string_view bytes,
                              std::string_view label)
{
  auto result = harness.Send(bytes);
  RequireOk(harness, result, label);
  INFO("expected-byte-count=" << bytes.size());
  REQUIRE(result.bytes_written == bytes.size());
  return result;
}

struct DecodedLine
{
  bool valid{false};
  std::string text;
  std::vector<std::string> tokens;
  std::string error;
};

DecodedLine DecodeLine(Harness::OutputEvent const &event)
{
  DecodedLine result;
  if (event.channel != Harness::Channel::Stdout)
  {
    result.error = "event is not stdout";
    return result;
  }
  if (event.payload_omitted_due_to_stream_limit || !event.bytes.exact())
  {
    result.error = "line bytes are not retained exactly";
    return result;
  }
  if (event.line_ending == Harness::LineEnding::None)
  {
    result.error = "stdout command has no complete line ending";
    return result;
  }

  auto bytes = event.bytes.ExactBytes();
  std::size_t ending_size = 0;
  switch (event.line_ending)
  {
  case Harness::LineEnding::Lf:
    if (bytes.empty() || bytes.back() != '\n')
    {
      result.error = "LF metadata does not match retained bytes";
      return result;
    }
    ending_size = 1;
    break;
  case Harness::LineEnding::Cr:
    if (bytes.empty() || bytes.back() != '\r')
    {
      result.error = "CR metadata does not match retained bytes";
      return result;
    }
    ending_size = 1;
    break;
  case Harness::LineEnding::CrLf:
    if (bytes.size() < 2 || bytes[bytes.size() - 2] != '\r' ||
        bytes.back() != '\n')
    {
      result.error = "CRLF metadata does not match retained bytes";
      return result;
    }
    ending_size = 2;
    break;
  case Harness::LineEnding::None:
    break;
  }
  bytes.resize(bytes.size() - ending_size);

  for (std::uint8_t byte : bytes)
  {
    if ((byte < 0x20 && byte != '\t') || byte >= 0x7f)
    {
      result.error = "stdout command contains a non-text control byte";
      return result;
    }
  }
  result.text.assign(bytes.begin(), bytes.end());

  std::size_t at = 0;
  auto spacing = [](char value) { return value == ' ' || value == '\t'; };
  while (at < result.text.size())
  {
    while (at < result.text.size() && spacing(result.text[at]))
      ++at;
    if (at == result.text.size())
      break;
    const std::size_t begin = at;
    while (at < result.text.size() && !spacing(result.text[at]))
      ++at;
    result.tokens.emplace_back(result.text.substr(begin, at - begin));
  }
  if (result.tokens.empty())
  {
    result.error = "stdout line contains no UCI command token";
    return result;
  }
  result.valid = true;
  return result;
}

bool HasExactCommand(Harness::OutputEvent const &event,
                     std::string_view command)
{
  const DecodedLine line = DecodeLine(event);
  return line.valid && line.tokens.size() == 1 &&
         line.tokens.front() == command;
}

Harness::OutputEvent WaitForExactCommand(Harness &harness,
                                         Harness::SendResult const &send,
                                         std::string_view command)
{
  auto result = harness.WaitForTokenAfter(send, command);
  RequireOk(harness, result, std::string("wait-for-") + std::string(command));
  REQUIRE(result.event.has_value());
  INFO("response-ordinal=" << result.event->ordinal);
  INFO("response-bytes=" << Harness::EscapeBytes(result.event->bytes));
  REQUIRE(HasExactCommand(*result.event, command));
  return *result.event;
}

bool IsInteger(std::string_view token)
{
  if (token.empty())
    return false;
  std::int64_t value = 0;
  const char *const begin = token.data();
  const char *const end = begin + token.size();
  const auto parsed = std::from_chars(begin, end, value);
  return parsed.ec == std::errc{} && parsed.ptr == end;
}

bool IsUciMove(std::string_view move)
{
  if (move == "0000")
    return true;
  if (move.size() != 4 && move.size() != 5)
    return false;
  if (move[0] < 'a' || move[0] > 'h' || move[1] < '1' || move[1] > '8' ||
      move[2] < 'a' || move[2] > 'h' || move[3] < '1' || move[3] > '8')
    return false;
  return move.size() == 4 || move[4] == 'q' || move[4] == 'r' ||
         move[4] == 'b' || move[4] == 'n';
}

bool IsOptionType(std::string_view token)
{
  return token == "check" || token == "spin" || token == "combo" ||
         token == "button" || token == "string";
}

std::optional<std::string>
ValidateOption(std::vector<std::string> const &tokens)
{
  if (tokens.size() < 4 || tokens[0] != "option" || tokens[1] != "name")
    return "option line must begin with option name and contain a nonempty id";

  std::size_t type_at = tokens.size();
  for (std::size_t index = 3; index + 1 < tokens.size(); ++index)
  {
    if (tokens[index] == "type" && IsOptionType(tokens[index + 1]))
    {
      type_at = index;
      break;
    }
  }
  if (type_at == tokens.size())
    return "option line has no unambiguous standard type field";

  const std::string &type = tokens[type_at + 1];
  const std::size_t tail = type_at + 2;
  if (type == "button")
  {
    if (tail != tokens.size())
      return "button option must not advertise value fields";
    return std::nullopt;
  }
  if (type == "check")
  {
    if (tokens.size() != tail + 2 || tokens[tail] != "default" ||
        (tokens[tail + 1] != "true" && tokens[tail + 1] != "false"))
      return "check option requires exactly default true or default false";
    return std::nullopt;
  }
  if (type == "spin")
  {
    if (tokens.size() != tail + 6 || tokens[tail] != "default" ||
        tokens[tail + 2] != "min" || tokens[tail + 4] != "max" ||
        !IsInteger(tokens[tail + 1]) || !IsInteger(tokens[tail + 3]) ||
        !IsInteger(tokens[tail + 5]))
      return "spin option requires numeric default, min, and max fields in "
             "standard order";
    return std::nullopt;
  }
  if (type == "string")
  {
    if (tokens.size() < tail + 2 || tokens[tail] != "default")
      return "string option requires a nonempty default field; empty is "
             "spelled <empty>";
    return std::nullopt;
  }

  if (tokens.size() < tail + 4 || tokens[tail] != "default")
    return "combo option requires a default followed by one or more var fields";
  std::size_t first_var = tail + 1;
  while (first_var < tokens.size() && tokens[first_var] != "var")
    ++first_var;
  if (first_var == tail + 1 || first_var == tokens.size())
    return "combo option has an empty default or no var field";
  std::size_t at = first_var;
  while (at < tokens.size())
  {
    if (tokens[at] != "var")
      return "combo option value list is not introduced by var";
    const std::size_t value_begin = ++at;
    while (at < tokens.size() && tokens[at] != "var")
      ++at;
    if (value_begin == at)
      return "combo option has an empty var value";
  }
  return std::nullopt;
}

bool IsInfoKeyword(std::string_view token)
{
  constexpr std::array<std::string_view, 17> keywords{
      "depth",      "seldepth", "time",     "nodes",          "pv",
      "multipv",    "score",    "currmove", "currmovenumber", "hashfull",
      "nps",        "tbhits",   "sbhits",   "cpuload",        "string",
      "refutation", "currline"};
  return std::find(keywords.begin(), keywords.end(), token) != keywords.end();
}

std::optional<std::string> ValidateInfo(std::vector<std::string> const &tokens)
{
  if (tokens.size() < 2 || tokens[0] != "info")
    return "info line must contain at least one standard information field";

  bool saw_depth = false;
  bool saw_seldepth = false;
  std::size_t at = 1;
  while (at < tokens.size())
  {
    const std::string field = tokens[at++];
    if (field == "string")
    {
      if (saw_seldepth && !saw_depth)
        return "seldepth requires depth in the same info line";
      return std::nullopt;
    }
    if (field == "depth" || field == "seldepth" || field == "time" ||
        field == "nodes" || field == "multipv" || field == "currmovenumber" ||
        field == "hashfull" || field == "nps" || field == "tbhits" ||
        field == "sbhits" || field == "cpuload")
    {
      if (at == tokens.size() || !IsInteger(tokens[at]))
        return field + " requires one integer";
      if (field == "depth")
        saw_depth = true;
      if (field == "seldepth")
        saw_seldepth = true;
      ++at;
      continue;
    }
    if (field == "currmove")
    {
      if (at == tokens.size() || !IsUciMove(tokens[at++]))
        return "currmove requires one UCI move";
      continue;
    }
    if (field == "score")
    {
      if (at + 1 >= tokens.size() ||
          (tokens[at] != "cp" && tokens[at] != "mate") ||
          !IsInteger(tokens[at + 1]))
        return "score requires cp or mate followed by one integer";
      at += 2;
      if (at < tokens.size() &&
          (tokens[at] == "lowerbound" || tokens[at] == "upperbound"))
        ++at;
      continue;
    }
    if (field == "pv" || field == "refutation")
    {
      const std::size_t begin = at;
      while (at < tokens.size() && !IsInfoKeyword(tokens[at]))
      {
        if (!IsUciMove(tokens[at]))
          return field + " contains a non-UCI move";
        ++at;
      }
      if (begin == at)
        return field + " requires at least one move";
      continue;
    }
    if (field == "currline")
    {
      if (at < tokens.size() && IsInteger(tokens[at]))
        ++at;
      const std::size_t begin = at;
      while (at < tokens.size() && !IsInfoKeyword(tokens[at]))
      {
        if (!IsUciMove(tokens[at]))
          return "currline contains a non-UCI move";
        ++at;
      }
      if (begin == at)
        return "currline requires at least one move";
      continue;
    }
    return "unknown UCI info field " + field;
  }
  if (saw_seldepth && !saw_depth)
    return "seldepth requires depth in the same info line";
  return std::nullopt;
}

std::optional<std::string>
ValidateBestmove(std::vector<std::string> const &tokens)
{
  if ((tokens.size() != 2 && tokens.size() != 4) || tokens[0] != "bestmove" ||
      !IsUciMove(tokens[1]))
    return "bestmove requires a primary UCI move and optional ponder move";
  if (tokens.size() == 4 && (tokens[2] != "ponder" || !IsUciMove(tokens[3])))
    return "bestmove ponder suffix is malformed";
  return std::nullopt;
}

enum class UciLineClass
{
  IdName,
  IdAuthor,
  Option,
  UciOk,
  ReadyOk,
  Info,
  Bestmove,
  Registration,
  Copyprotection
};

std::string_view ClassName(UciLineClass value)
{
  switch (value)
  {
  case UciLineClass::IdName:
    return "id-name";
  case UciLineClass::IdAuthor:
    return "id-author";
  case UciLineClass::Option:
    return "option";
  case UciLineClass::UciOk:
    return "uciok";
  case UciLineClass::ReadyOk:
    return "readyok";
  case UciLineClass::Info:
    return "info";
  case UciLineClass::Bestmove:
    return "bestmove";
  case UciLineClass::Registration:
    return "registration";
  case UciLineClass::Copyprotection:
    return "copyprotection";
  }
  return "unknown";
}

struct ParsedUciLine
{
  DecodedLine line;
  std::optional<UciLineClass> classification;
  std::string error;
};

ParsedUciLine ParseUciLine(Harness::OutputEvent const &event)
{
  ParsedUciLine result;
  result.line = DecodeLine(event);
  if (!result.line.valid)
  {
    result.error = result.line.error;
    return result;
  }
  auto const &tokens = result.line.tokens;
  const std::string &command = tokens.front();
  if (command == "id")
  {
    if (tokens.size() < 3 || (tokens[1] != "name" && tokens[1] != "author"))
    {
      result.error = "id requires name or author and a nonempty value";
      return result;
    }
    result.classification =
        tokens[1] == "name" ? UciLineClass::IdName : UciLineClass::IdAuthor;
    return result;
  }
  if (command == "option")
  {
    if (auto error = ValidateOption(tokens))
    {
      result.error = *error;
      return result;
    }
    result.classification = UciLineClass::Option;
    return result;
  }
  if (command == "uciok" || command == "readyok")
  {
    if (tokens.size() != 1)
    {
      result.error = command + " must be the only token on its line";
      return result;
    }
    result.classification =
        command == "uciok" ? UciLineClass::UciOk : UciLineClass::ReadyOk;
    return result;
  }
  if (command == "info")
  {
    if (auto error = ValidateInfo(tokens))
    {
      result.error = *error;
      return result;
    }
    result.classification = UciLineClass::Info;
    return result;
  }
  if (command == "bestmove")
  {
    if (auto error = ValidateBestmove(tokens))
    {
      result.error = *error;
      return result;
    }
    result.classification = UciLineClass::Bestmove;
    return result;
  }
  if (command == "registration" || command == "copyprotection")
  {
    if (tokens.size() != 2 ||
        (tokens[1] != "checking" && tokens[1] != "ok" && tokens[1] != "error"))
    {
      result.error = command + " requires exactly checking, ok, or error";
      return result;
    }
    result.classification = command == "registration"
                                ? UciLineClass::Registration
                                : UciLineClass::Copyprotection;
    return result;
  }
  result.error = "unknown UCI engine-to-GUI command " + command;
  return result;
}

ParsedUciLine RequireParsed(Harness::OutputEvent const &event)
{
  ParsedUciLine parsed = ParseUciLine(event);
  INFO("stdout-ordinal=" << event.ordinal
                         << " ending=" << Harness::ToString(event.line_ending)
                         << " bytes=" << Harness::EscapeBytes(event.bytes));
  INFO("parse-error=" << parsed.error);
  REQUIRE(parsed.classification.has_value());
  INFO("line-class=" << ClassName(*parsed.classification));
  return parsed;
}

void RequireHarnessSupport()
{
  if (!Harness::IsSupported())
    SKIP("ProcessHarness subprocess tests currently require Windows");
}

void RequireBoundedCapture(Harness::Transcript const &transcript)
{
  INFO(DumpTranscript(transcript));
  REQUIRE_FALSE(transcript.output_events_truncated);
  REQUIRE_FALSE(transcript.stdout_stream.bytes.truncated);
  REQUIRE_FALSE(transcript.stderr_stream.bytes.truncated);
  for (auto const &diagnostic : transcript.diagnostics)
    REQUIRE(diagnostic.outcome != Harness::Outcome::ResourceLimit);
}

struct FinishedTranscript
{
  Harness::SendResult quit;
  Harness::Transcript transcript;
};

FinishedTranscript FinishIdleSession(Harness &harness)
{
  auto quit = SendExact(harness, kCanonicalQuit, "send-quit-lf");
  // WaitForExit has its own absolute graceful-exit deadline. It is invoked
  // immediately after the successful SEND; no sleep or output polling can
  // extend either harness deadline.
  auto exit = harness.WaitForExit();
  RequireOk(harness, exit, "wait-for-idle-exit");
  auto transcript = harness.Snapshot();
  RequireBoundedCapture(transcript);
  REQUIRE(transcript.process_exited);
  REQUIRE(transcript.stdin_closed);
  REQUIRE(transcript.stdout_stream.eof);
  REQUIRE(transcript.stderr_stream.eof);
  REQUIRE(transcript.process_group_empty);
  REQUIRE_FALSE(transcript.hard_cleanup_attempted);
  return FinishedTranscript{quit, std::move(transcript)};
}

void RequireHandshakeInterval(Harness const &harness,
                              Harness::SendResult const &request,
                              Harness::OutputEvent const &uciok)
{
  const auto transcript = harness.Snapshot();
  INFO(DumpTranscript(transcript));
  bool saw_name = false;
  bool saw_author = false;
  bool saw_option = false;
  bool saw_terminator = false;
  for (auto const &event : transcript.output_events)
  {
    if (event.channel != Harness::Channel::Stdout ||
        event.ordinal <= request.ordinal || event.ordinal > uciok.ordinal)
      continue;
    const ParsedUciLine parsed = ParseUciLine(event);
    if (!parsed.classification)
    {
      INFO("retained unclassified handshake line ordinal=" << event.ordinal
           << " token=" << event.leading_token
           << " parse-error=" << parsed.error);
      const std::set<std::string_view> reserved{
          "id", "option", "uciok", "readyok", "info", "bestmove",
          "registration", "copyprotection"};
      REQUIRE_FALSE(reserved.contains(event.leading_token));
      continue;
    }
    switch (*parsed.classification)
    {
    case UciLineClass::IdName:
      REQUIRE_FALSE(saw_option);
      saw_name = true;
      break;
    case UciLineClass::IdAuthor:
      REQUIRE_FALSE(saw_option);
      saw_author = true;
      break;
    case UciLineClass::Option:
      REQUIRE(saw_name);
      REQUIRE(saw_author);
      saw_option = true;
      break;
    case UciLineClass::Info:
      break;
    case UciLineClass::UciOk:
      REQUIRE(event.ordinal == uciok.ordinal);
      REQUIRE(saw_name);
      REQUIRE(saw_author);
      REQUIRE_FALSE(saw_terminator);
      saw_terminator = true;
      break;
    case UciLineClass::ReadyOk:
    case UciLineClass::Bestmove:
    case UciLineClass::Registration:
    case UciLineClass::Copyprotection:
      FAIL("non-handshake command appeared before the first uciok");
    }
  }
  REQUIRE(saw_name);
  REQUIRE(saw_author);
  REQUIRE(saw_terminator);
}

struct HandshakeReceipt
{
  Harness::SendResult request;
  Harness::OutputEvent uciok;
};

HandshakeReceipt
CompleteHandshake(Harness &harness,
                  std::string_view request_bytes = kCanonicalUci,
                  std::string_view label = "send-uci-lf")
{
  auto request = SendExact(harness, request_bytes, label);
  auto uciok = WaitForExactCommand(harness, request, "uciok");
  RequireHandshakeInterval(harness, request, uciok);
  return HandshakeReceipt{request, uciok};
}

struct ReadyReceipt
{
  Harness::SendResult request;
  Harness::OutputEvent readyok;
};

ReadyReceipt CompleteReadiness(Harness &harness,
                               std::string_view request_bytes = kCanonicalReady,
                               std::string_view label = "send-isready-lf")
{
  auto request = SendExact(harness, request_bytes, label);
  auto readyok = WaitForExactCommand(harness, request, "readyok");
  REQUIRE(readyok.ordinal > request.ordinal);
  return ReadyReceipt{request, readyok};
}

std::vector<std::uint64_t>
RequireCommandOrdinals(Harness::Transcript const &transcript,
                       std::string_view command)
{
  std::vector<std::uint64_t> ordinals;
  for (auto const &event : transcript.output_events)
  {
    if (event.channel != Harness::Channel::Stdout ||
        event.leading_token != command)
      continue;
    INFO("command=" << command << " ordinal=" << event.ordinal
                    << " bytes=" << Harness::EscapeBytes(event.bytes));
    REQUIRE(HasExactCommand(event, command));
    ordinals.push_back(event.ordinal);
  }
  return ordinals;
}

enum class PairState
{
  Unseen,
  Checking,
  Terminal
};

void AdvanceRegistration(PairState &state, ParsedUciLine const &parsed)
{
  const std::string &value = parsed.line.tokens[1];
  if (value == "checking")
  {
    REQUIRE(state == PairState::Unseen);
    state = PairState::Checking;
  }
  else if (value == "error")
  {
    // UCI explicitly permits the startup registration error result; it need
    // not be preceded by a visible checking notification.
    REQUIRE(state != PairState::Terminal);
    state = PairState::Terminal;
  }
  else
  {
    REQUIRE(value == "ok");
    REQUIRE(state == PairState::Checking);
    state = PairState::Terminal;
  }
}

void AdvanceCopyprotection(PairState &state, ParsedUciLine const &parsed)
{
  const std::string &value = parsed.line.tokens[1];
  if (value == "checking")
  {
    REQUIRE(state == PairState::Unseen);
    state = PairState::Checking;
  }
  else
  {
    REQUIRE(state == PairState::Checking);
    state = PairState::Terminal;
  }
}

struct ProtocolAnchors
{
  HandshakeReceipt handshake;
  std::optional<ReadyReceipt> readiness;
  Harness::SendResult quit;
};

void RequireProtocolClean(Harness::Transcript const &transcript,
                          ProtocolAnchors const &anchors)
{
  RequireBoundedCapture(transcript);
  REQUIRE(anchors.quit.ordinal > anchors.handshake.uciok.ordinal);
  if (anchors.readiness)
  {
    REQUIRE(anchors.readiness->request.ordinal >
            anchors.handshake.uciok.ordinal);
    REQUIRE(anchors.quit.ordinal > anchors.readiness->readyok.ordinal);
  }
  bool saw_name = false;
  bool saw_author = false;
  bool saw_option = false;
  std::size_t uciok_count = 0;
  std::size_t readyok_count = 0;
  PairState registration = PairState::Unseen;
  PairState copyprotection = PairState::Unseen;

  for (auto const &event : transcript.output_events)
  {
    if (event.channel != Harness::Channel::Stdout)
      continue;
    const ParsedUciLine parsed = RequireParsed(event);
    switch (*parsed.classification)
    {
    case UciLineClass::IdName:
      REQUIRE(event.ordinal > anchors.handshake.request.ordinal);
      REQUIRE(event.ordinal < anchors.handshake.uciok.ordinal);
      REQUIRE_FALSE(saw_option);
      saw_name = true;
      break;
    case UciLineClass::IdAuthor:
      REQUIRE(event.ordinal > anchors.handshake.request.ordinal);
      REQUIRE(event.ordinal < anchors.handshake.uciok.ordinal);
      REQUIRE_FALSE(saw_option);
      saw_author = true;
      break;
    case UciLineClass::Option:
      REQUIRE(event.ordinal > anchors.handshake.request.ordinal);
      REQUIRE(event.ordinal < anchors.handshake.uciok.ordinal);
      REQUIRE(saw_name);
      REQUIRE(saw_author);
      saw_option = true;
      break;
    case UciLineClass::UciOk:
      ++uciok_count;
      REQUIRE(event.ordinal == anchors.handshake.uciok.ordinal);
      REQUIRE(saw_name);
      REQUIRE(saw_author);
      break;
    case UciLineClass::ReadyOk:
      ++readyok_count;
      REQUIRE(anchors.readiness.has_value());
      REQUIRE(event.ordinal == anchors.readiness->readyok.ordinal);
      REQUIRE(event.ordinal > anchors.readiness->request.ordinal);
      break;
    case UciLineClass::Info:
      // The standard card deliberately imposes no extra phase restriction
      // on an otherwise grammar-valid info line.
      break;
    case UciLineClass::Bestmove:
      FAIL("bestmove is not causal in a transcript containing no go command");
    case UciLineClass::Registration:
      REQUIRE(event.ordinal > anchors.handshake.uciok.ordinal);
      AdvanceRegistration(registration, parsed);
      break;
    case UciLineClass::Copyprotection:
      REQUIRE(event.ordinal > anchors.handshake.uciok.ordinal);
      AdvanceCopyprotection(copyprotection, parsed);
      break;
    }
  }

  REQUIRE(saw_name);
  REQUIRE(saw_author);
  REQUIRE(uciok_count == 1);
  REQUIRE(readyok_count == (anchors.readiness ? 1 : 0));
  REQUIRE(registration != PairState::Checking);
  REQUIRE(copyprotection != PairState::Checking);
}

} // namespace

TEST_CASE("T-APP-001.standard-slice canonical first-command handshake",
          "[uci][protocol][T-APP-001]")
{
  RequireHarnessSupport();
  auto harness = MakeHarness();
  const HandshakeReceipt handshake = CompleteHandshake(harness);
  const FinishedTranscript finished = FinishIdleSession(harness);

  INFO(DumpTranscript(finished.transcript));
  REQUIRE(RequireCommandOrdinals(finished.transcript, "uciok") ==
          std::vector<std::uint64_t>{handshake.uciok.ordinal});
}

TEST_CASE(
    "T-APP-002.standard-slice repeated idle isready is causal and one-to-one",
    "[uci][protocol][T-APP-002]")
{
  RequireHarnessSupport();
  auto harness = MakeHarness();
  const HandshakeReceipt handshake = CompleteHandshake(harness);
  std::vector<std::uint64_t> expected_readyok_ordinals;
  for (std::size_t request_index = 0; request_index < 4; ++request_index)
  {
    INFO("request-index=" << request_index);
    const ReadyReceipt ready = CompleteReadiness(harness);
    expected_readyok_ordinals.push_back(ready.readyok.ordinal);
  }
  const FinishedTranscript finished = FinishIdleSession(harness);

  INFO(DumpTranscript(finished.transcript));
  REQUIRE(RequireCommandOrdinals(finished.transcript, "uciok") ==
          std::vector<std::uint64_t>{handshake.uciok.ordinal});
  REQUIRE(RequireCommandOrdinals(finished.transcript, "readyok") ==
          expected_readyok_ordinals);
}

TEST_CASE("T-APP-003.handshaken-idle-quit-split terminates cleanly without EOF "
          "policy",
          "[uci][protocol][T-APP-003]")
{
  RequireHarnessSupport();
  for (std::size_t process_index = 0; process_index < 8; ++process_index)
  {
    INFO("process-index=" << process_index);
    auto harness = MakeHarness();
    const HandshakeReceipt handshake = CompleteHandshake(harness);
    const FinishedTranscript finished = FinishIdleSession(harness);
    INFO(DumpTranscript(finished.transcript));
    REQUIRE(RequireCommandOrdinals(finished.transcript, "uciok") ==
            std::vector<std::uint64_t>{handshake.uciok.ordinal});
  }
}

TEST_CASE("T-APP-017.standard-slice exhausts the eight frozen input variants",
          "[uci][protocol][T-APP-017]")
{
  RequireHarnessSupport();

  enum class Flow
  {
    FirstUci,
    IgnoredIdle,
    IdleReady
  };
  struct Variant
  {
    std::string_view id;
    std::string_view bytes;
    Flow flow;
  };
  constexpr std::array<Variant, 8> variants{
      Variant{"blank-lf", "\x0a", Flow::IgnoredIdle},
      Variant{"spaces-lf", "\x20\x09\x20\x0a", Flow::IgnoredIdle},
      Variant{"unknown-lf", "x-unknown\x0a", Flow::IgnoredIdle},
      Variant{"uci-extra-lf", "uci extra\x0a", Flow::FirstUci},
      Variant{"uci-padded-lf", "\x09\x20uci\x20\x09\x0a", Flow::FirstUci},
      Variant{"uci-cr", "uci\x0d", Flow::FirstUci},
      Variant{"uci-crlf", "uci\x0d\x0a", Flow::FirstUci},
      Variant{"isready-padded", "\x20\x09isready\x09\x20\x0a", Flow::IdleReady},
  };

  for (Variant const &variant : variants)
  {
    DYNAMIC_SECTION("variant=" << variant.id)
    {
      INFO("variant-id=" << variant.id);
      INFO("variant-bytes=" << Harness::EscapeBytes(
               ExactCapture(std::vector<std::uint8_t>(variant.bytes.begin(),
                                                      variant.bytes.end()))));
      auto harness = MakeHarness();
      HandshakeReceipt handshake;
      ReadyReceipt readiness;
      if (variant.flow == Flow::FirstUci)
      {
        handshake = CompleteHandshake(harness, variant.bytes,
                                      std::string("send-variant-") +
                                          std::string(variant.id));
        readiness = CompleteReadiness(harness);
      }
      else
      {
        handshake = CompleteHandshake(harness);
        if (variant.flow == Flow::IgnoredIdle)
        {
          (void)SendExact(harness, variant.bytes,
                          std::string("send-variant-") + std::string(variant.id));
          readiness = CompleteReadiness(harness);
        }
        else
        {
          readiness = CompleteReadiness(harness, variant.bytes,
                                        std::string("send-variant-") +
                                            std::string(variant.id));
        }
      }
      const FinishedTranscript finished = FinishIdleSession(harness);

      INFO(DumpTranscript(finished.transcript));
      REQUIRE(RequireCommandOrdinals(finished.transcript, "uciok") ==
              std::vector<std::uint64_t>{handshake.uciok.ordinal});
      REQUIRE(RequireCommandOrdinals(finished.transcript, "readyok") ==
              std::vector<std::uint64_t>{readiness.readyok.ordinal});
    }
  }
}

TEST_CASE("T-APP-018.liveness-only-split ignored idle unknown input remains "
          "responsive",
          "[uci][protocol][T-APP-018]")
{
  RequireHarnessSupport();
  auto harness = MakeHarness();
  const HandshakeReceipt handshake = CompleteHandshake(harness);
  (void)SendExact(harness, "x-unknown\n", "send-standalone-unknown-lf");
  const ReadyReceipt readiness = CompleteReadiness(harness);
  const FinishedTranscript finished = FinishIdleSession(harness);

  INFO("This split asserts only ignore/liveness; it makes no chess-position "
       "observation");
  INFO(DumpTranscript(finished.transcript));
  REQUIRE(RequireCommandOrdinals(finished.transcript, "uciok") ==
          std::vector<std::uint64_t>{handshake.uciok.ordinal});
  REQUIRE(RequireCommandOrdinals(finished.transcript, "readyok") ==
          std::vector<std::uint64_t>{readiness.readyok.ordinal});
}

TEST_CASE("T-APP-019.handshake-readiness-split keeps stdout protocol-clean",
          "[uci][protocol][T-APP-019]")
{
  RequireHarnessSupport();
  for (bool include_readiness : {false, true})
  {
    INFO(
        "transcript=" << (include_readiness ? "uci-isready-quit" : "uci-quit"));
    auto harness = MakeHarness();
    const HandshakeReceipt handshake = CompleteHandshake(harness);
    std::optional<ReadyReceipt> readiness;
    if (include_readiness)
      readiness = CompleteReadiness(harness);
    const FinishedTranscript finished = FinishIdleSession(harness);

    RequireProtocolClean(finished.transcript,
                         ProtocolAnchors{handshake, readiness, finished.quit});
  }
}
