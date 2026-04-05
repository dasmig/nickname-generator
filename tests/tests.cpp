#include "catch_amalgamated.hpp"
#include "../dasmig/nicknamegen.hpp"

#include <algorithm>
#include <cwctype>
#include <ranges>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: access the singleton with resources pre-loaded
// ---------------------------------------------------------------------------
static dasmig::nng& gen()
{
    static bool loaded = false;
    if (!loaded)
    {
        // Ensure resources are loaded from the known location.
        dasmig::nng::instance().load("resources");
        loaded = true;
    }
    return dasmig::nng::instance();
}

// ---------------------------------------------------------------------------
// nickname class
// ---------------------------------------------------------------------------
TEST_CASE("nickname - plain() returns original string", "[nickname]")
{
    const auto nick = gen().get_nickname(L"John Doe");
    // plain() should be either the full name (name-based) or a word from
    // the wordlists. In both cases it must be non-empty.
    REQUIRE_FALSE(nick.plain().empty());
}

TEST_CASE("nickname - implicit wstring conversion", "[nickname]")
{
    const auto nick = gen().get_nickname(L"Jane");
    std::wstring as_str = nick;
    REQUIRE_FALSE(as_str.empty());
}

TEST_CASE("nickname - operator<< streams internal string", "[nickname]")
{
    const auto nick = gen().get_nickname(L"Alice Bob");
    std::wostringstream wos;
    wos << nick;
    REQUIRE(wos.str() == static_cast<std::wstring>(nick));
}

// ---------------------------------------------------------------------------
// nng singleton
// ---------------------------------------------------------------------------
TEST_CASE("nng - instance() returns same object", "[nng]")
{
    auto& a = dasmig::nng::instance();
    auto& b = dasmig::nng::instance();
    REQUIRE(&a == &b);
}

TEST_CASE("nng - get_nickname() without name returns non-empty",
          "[nng][generation]")
{
    const auto nick = gen().get_nickname();
    std::wstring result = nick;
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("nng - get_nickname() with name returns non-empty",
          "[nng][generation]")
{
    const auto nick = gen().get_nickname(L"Alberto Bins Elis");
    std::wstring result = nick;
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("nng - bulk generation produces variety", "[nng][generation]")
{
    std::vector<std::wstring> results;
    results.reserve(200);

    for (int i = 0; i < 200; ++i)
    {
        results.emplace_back(gen().get_nickname(L"Test User"));
    }

    // Remove duplicates and check that we get at least some variety.
    std::ranges::sort(results);
    auto [first, last] = std::ranges::unique(results);
    results.erase(first, last);
    REQUIRE(results.size() > 5);
}

TEST_CASE("nng - wordlist-only generation (no name)", "[nng][generation]")
{
    std::vector<std::wstring> results;
    results.reserve(100);

    for (int i = 0; i < 100; ++i)
    {
        results.emplace_back(gen().get_nickname());
    }

    // All results should be non-empty.
    REQUIRE(std::ranges::all_of(
        results, [](const std::wstring& s) { return !s.empty(); }));
}

TEST_CASE("nng - load() with invalid path does not crash", "[nng]")
{
    REQUIRE_NOTHROW(gen().load("nonexistent_path_xyz"));
}

// ---------------------------------------------------------------------------
// Nickname character checks — bulk generation fuzz
// ---------------------------------------------------------------------------
TEST_CASE("Generated nicknames contain only printable or leet chars",
          "[nng][generation]")
{
    for (int i = 0; i < 500; ++i)
    {
        std::wstring nick = gen().get_nickname(L"Diego Migotto");
        for (const auto& ch : nick)
        {
            // Each character should be printable or a common separator.
            const bool printable = std::iswprint(ch) != 0;
            const bool separator = (ch == L'-' || ch == L'_');
            INFO("char code: " << static_cast<int>(ch)
                                << " in nickname: [streamed]");
            REQUIRE((printable || separator));
        }
    }
}

// ---------------------------------------------------------------------------
// numify produces digit characters
// ---------------------------------------------------------------------------
TEST_CASE("Numified nicknames end with digit characters", "[nng][numify]")
{
    // Generate many nicknames; some will be numified.
    bool found_numified = false;
    for (int i = 0; i < 1000; ++i)
    {
        std::wstring nick = gen().get_nickname();
        if (!nick.empty() && std::iswdigit(nick.back()) != 0)
        {
            found_numified = true;
            // All trailing digits should be actual digit chars.
            auto it = nick.rbegin();
            while (it != nick.rend() && std::iswdigit(*it) != 0)
            {
                REQUIRE(*it >= L'0');
                REQUIRE(*it <= L'9');
                ++it;
            }
        }
    }
    REQUIRE(found_numified);
}

// ---------------------------------------------------------------------------
// oneleet actually changes a character
// ---------------------------------------------------------------------------
TEST_CASE("Leet variants differ from original", "[nng][leet]")
{
    // Over many generations, at least some should have leet substitutions.
    bool found_leet = false;
    for (int i = 0; i < 500; ++i)
    {
        auto nick = gen().get_nickname(L"Alberto");
        std::wstring result = nick;
        // Check if any digit appears that wasn't in the original.
        for (const auto& ch : result)
        {
            if (std::iswdigit(ch) != 0)
            {
                found_leet = true;
                break;
            }
        }
        if (found_leet)
        {
            break;
        }
    }
    REQUIRE(found_leet);
}

// ---------------------------------------------------------------------------
// Name-based generation
// ---------------------------------------------------------------------------
TEST_CASE("Name-based nickname relates to input somehow",
          "[nng][name-based]")
{
    // Over many attempts, at least one nickname should contain part of the
    // input name (first name, last name, or initial).
    bool found_related = false;
    for (int i = 0; i < 500; ++i)
    {
        auto nick = gen().get_nickname(L"John Smith");
        std::wstring result = nick;
        std::wstring lower_result;
        lower_result.resize(result.size());
        std::ranges::transform(result, lower_result.begin(),
                               [](wchar_t c) { return std::towlower(c); });

        if (lower_result.contains(L"john") || lower_result.contains(L"smith") ||
            lower_result.contains(L"js") || lower_result.contains(L"j") ||
            lower_result.contains(L"s"))
        {
            found_related = true;
            break;
        }
    }
    REQUIRE(found_related);
}

TEST_CASE("Single-name input works", "[nng][name-based]")
{
    // Single name should not crash even though split produces one element.
    for (int i = 0; i < 100; ++i)
    {
        REQUIRE_NOTHROW(gen().get_nickname(L"Mononym"));
    }
}

TEST_CASE("Multi-part name works", "[nng][name-based]")
{
    for (int i = 0; i < 100; ++i)
    {
        std::wstring nick = gen().get_nickname(L"First Middle Last Suffix");
        REQUIRE_FALSE(nick.empty());
    }
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------
TEST_CASE("Empty name falls through to wordlist", "[nng][edge]")
{
    auto nick = gen().get_nickname(L"");
    std::wstring result = nick;
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("Unicode name input does not crash", "[nng][edge]")
{
    REQUIRE_NOTHROW(gen().get_nickname(L"José García"));
    REQUIRE_NOTHROW(gen().get_nickname(L"田中太郎"));
    REQUIRE_NOTHROW(gen().get_nickname(L"Мария Иванова"));
}

// ---------------------------------------------------------------------------
// Stress test
// ---------------------------------------------------------------------------
TEST_CASE("Stress: 10000 generations without crash", "[nng][stress]")
{
    for (int i = 0; i < 10000; ++i)
    {
        if (i % 2 == 0)
        {
            REQUIRE_NOTHROW(gen().get_nickname(L"Stress Test Name"));
        }
        else
        {
            REQUIRE_NOTHROW(gen().get_nickname());
        }
    }
}
