// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_common.h"

#include <string>
#include <vector>

#include "app/cli.h"

using namespace rm;

namespace {

// Builds an argv array from wide string literals. The strings outlive the
// call because the test owns the vector.
std::vector<wchar_t*> argv_for(const std::vector<std::wstring>& args) {
    std::vector<wchar_t*> argv;
    argv.reserve(args.size());
    for (const auto& arg : args) {
        argv.push_back(const_cast<wchar_t*>(arg.c_str()));
    }
    return argv;
}

CliOptions parse(const std::vector<std::wstring>& args, bool& ok, std::string& error) {
    std::vector<std::wstring> copy = args;
    std::vector<wchar_t*> argv = argv_for(copy);
    CliOptions options;
    ok = CliOptions::parse(static_cast<int>(argv.size()), argv.data(), options, error);
    return options;
}

}  // namespace

TEST_CASE("no arguments yields the defaults") {
    bool ok = false;
    std::string error;
    const CliOptions options = parse({L"RearMarch.exe"}, ok, error);
    CHECK(ok);
    CHECK_FALSE(options.minimized);
    CHECK_FALSE(options.safe_mode);
    CHECK_FALSE(options.no_autostart);
    CHECK(options.locale.empty());
    CHECK(options.config_path.empty());
}

TEST_CASE("flags parse in any order") {
    bool ok = false;
    std::string error;
    const CliOptions options =
        parse({L"RearMarch.exe", L"--minimized", L"--safe-mode", L"--no-autostart"},
              ok, error);
    CHECK(ok);
    CHECK(options.minimized);
    CHECK(options.safe_mode);
    CHECK(options.no_autostart);
}

TEST_CASE("--locale takes the next argument") {
    bool ok = false;
    std::string error;
    const CliOptions options = parse({L"RearMarch.exe", L"--locale", L"id"}, ok, error);
    CHECK(ok);
    CHECK(options.locale == "id");
}

TEST_CASE("--config takes the next argument") {
    bool ok = false;
    std::string error;
    const CliOptions options =
        parse({L"RearMarch.exe", L"--config", L"C:\\temp\\config.json"}, ok, error);
    CHECK(ok);
    CHECK(options.config_path == L"C:\\temp\\config.json");
}

TEST_CASE("a missing value is an error") {
    bool ok = false;
    std::string error;
    parse({L"RearMarch.exe", L"--locale"}, ok, error);
    CHECK_FALSE(ok);
    CHECK_FALSE(error.empty());

    error.clear();
    parse({L"RearMarch.exe", L"--config"}, ok, error);
    CHECK_FALSE(ok);
    CHECK_FALSE(error.empty());
}

TEST_CASE("an unknown option is an error") {
    bool ok = false;
    std::string error;
    parse({L"RearMarch.exe", L"--turbo"}, ok, error);
    CHECK_FALSE(ok);
    CHECK(error.find("--turbo") != std::string::npos);
}

TEST_CASE("a non-ASCII locale is reported as empty") {
    // Only bundled codes are accepted; anything else means "detect".
    bool ok = false;
    std::string error;
    const CliOptions options =
        parse({L"RearMarch.exe", L"--locale", L"\u65e5\u672c\u8a9e"}, ok, error);
    CHECK(ok);
    CHECK(options.locale.empty());
}

TEST_CASE("help and version are recognised") {
    bool ok = false;
    std::string error;
    CHECK(parse({L"RearMarch.exe", L"--help"}, ok, error).show_help);
    CHECK(parse({L"RearMarch.exe", L"-h"}, ok, error).show_help);
    CHECK(parse({L"RearMarch.exe", L"--version"}, ok, error).show_version);
    CHECK(parse({L"RearMarch.exe", L"-v"}, ok, error).show_version);
}

TEST_CASE("the usage text names the executable") {
    const std::string usage = usage_text();
    CHECK(usage.find("RearMarch.exe") != std::string::npos);
    CHECK(usage.find("--safe-mode") != std::string::npos);
}
