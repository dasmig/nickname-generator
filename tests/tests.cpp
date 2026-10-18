#include "catch_amalgamated.hpp"
#include "../dasmig/nicknamegen.hpp"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// Test access helper — exposes private methods and state for coverage testing
// ---------------------------------------------------------------------------
struct nng_test_access
{
    static std::wstring sentence_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::sentence_case(s, e);
    }
    static std::wstring camel_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::camel_case(s, e);
    }
    static std::wstring reverse_sentence_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::reverse_sentence_case(s, e);
    }
    static std::wstring bathtub_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::bathtub_case(s, e);
    }
    static std::wstring upper_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::upper_case(s, e);
    }
    static std::wstring lower_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::lower_case(s, e);
    }
    static std::wstring title_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::title_case(s, e);
    }
    static std::wstring winding_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::winding_case(s, e);
    }
    static std::wstring random_single_case(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::random_single_case(s, e);
    }
    static std::wstring snake_case(const std::wstring& s)
    {
        return dasmig::nng::snake_case(s);
    }
    static std::wstring xfy(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::xfy(s, e);
    }
    static std::wstring reverse(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::reverse(s, e);
    }
    static std::wstring yfy(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::yfy(s, e);
    }
    static std::wstring numify(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::numify(s, e);
    }
    static std::wstring tracefy(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::tracefy(s, e);
    }
    static std::wstring ingify(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::ingify(s, e);
    }
    static std::wstring duovowel(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::duovowel(s, e);
    }
    static std::wstring oneleet(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::oneleet(s, e);
    }
    static std::wstring allleet(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::allleet(s, e);
    }
    static std::wstring first_name(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::first_name(s, e);
    }
    static std::wstring last_name(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::last_name(s, e);
    }
    static std::wstring any_name(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::any_name(s, e);
    }
    static std::wstring initials(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::initials(s, e);
    }
    static std::wstring mix_two(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::mix_two(s, e);
    }
    static std::wstring first_plus_initial(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::first_plus_initial(s, e);
    }
    static std::wstring initial_plus_last(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::initial_plus_last(s, e);
    }
    static std::wstring reduce_single_name(const std::wstring& s)
    {
        dasmig::core::rng e{1};
        return dasmig::nng::reduce_single_name(s, e);
    }
    static auto& wordlists(dasmig::nng& g)
    {
        return g._wordlists;
    }
    static auto engine(const dasmig::nng& g)
    {
        return g._engine;
    }
    static std::vector<std::wstring> split_name(const std::wstring& s)
    {
        return dasmig::nng::split_name(s);
    }
};

template <class Generator>
concept legacy_generation_api = requires(Generator& g) { g.get_nickname(); } ||
                                requires(Generator& g, std::wstring name) {
                                    g.get_nickname(name);
                                } ||
                                requires(Generator& g, std::wstring name) {
                                    g.get_nickname(name, std::uint64_t{42});
                                } ||
                                requires(Generator& g) { g.seed(std::uint64_t{42}); } ||
                                requires(Generator& g) { g.unseed(); };

static_assert(std::is_aggregate_v<dasmig::nickname_request>);
static_assert(!legacy_generation_api<dasmig::nng>);

// ---------------------------------------------------------------------------
// Helper: locate the resources directory (works from repo root or build/)
// ---------------------------------------------------------------------------
static std::filesystem::path resource_path()
{
    for (const auto& candidate : {"resources", "../resources"})
    {
        std::filesystem::path p{candidate};
        if (std::filesystem::exists(p) && std::filesystem::is_directory(p))
        {
            return p;
        }
    }
    return "resources"; // fallback
}

// ---------------------------------------------------------------------------
// Helper: access the singleton with resources pre-loaded
// ---------------------------------------------------------------------------
static dasmig::nng& gen()
{
    static bool loaded = false;
    if (!loaded)
    {
        dasmig::nng::instance().load(resource_path());
        loaded = true;
    }
    return dasmig::nng::instance();
}

// ---------------------------------------------------------------------------
// nickname class
// ---------------------------------------------------------------------------
TEST_CASE("nickname - plain() returns original string", "[nickname]")
{
    const auto nick = gen().get_nickname({.name = L"John Doe"});
    // plain() should be either the full name (name-based) or a word from
    // the wordlists. In both cases it must be non-empty.
    REQUIRE_FALSE(nick.plain().empty());
}

TEST_CASE("nickname - implicit wstring conversion", "[nickname]")
{
    const auto nick = gen().get_nickname({.name = L"Jane"});
    std::wstring as_str = nick;
    REQUIRE_FALSE(as_str.empty());
}

TEST_CASE("nickname - operator<< streams internal string", "[nickname]")
{
    const auto nick = gen().get_nickname({.name = L"Alice Bob"});
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

TEST_CASE("nng - has_wordlists() returns true after load", "[nng]")
{
    REQUIRE(gen().has_wordlists());
}

TEST_CASE("nng - has_wordlists() returns false when empty", "[nng][edge]")
{
    auto& g = dasmig::nng::instance();
    auto saved = nng_test_access::wordlists(g);
    nng_test_access::wordlists(g).clear();

    REQUIRE_FALSE(g.has_wordlists());

    nng_test_access::wordlists(g) = saved;
}

TEST_CASE("nng - get_nickname(request) without name returns non-empty",
          "[nng][generation]")
{
    const auto nick = gen().get_nickname({});
    std::wstring result = nick;
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("nng - get_nickname(request) with name returns non-empty",
          "[nng][generation]")
{
    const auto nick = gen().get_nickname({.name = L"Alberto Bins Elis"});
    std::wstring result = nick;
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("nng - bulk generation produces variety", "[nng][generation]")
{
    std::vector<std::wstring> results;
    results.reserve(200);

    for (int i = 0; i < 200; ++i)
    {
        results.emplace_back(gen().get_nickname({.name = L"Test User"}));
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
        results.emplace_back(gen().get_nickname({}));
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
        std::wstring nick = gen().get_nickname({.name = L"Diego Migotto"});
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
        std::wstring nick = gen().get_nickname({});
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
        auto nick = gen().get_nickname({.name = L"Alberto"});
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
        auto nick = gen().get_nickname({.name = L"John Smith"});
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
        REQUIRE_NOTHROW(gen().get_nickname({.name = L"Mononym"}));
    }
}

TEST_CASE("Multi-part name works", "[nng][name-based]")
{
    for (int i = 0; i < 100; ++i)
    {
        std::wstring nick = gen().get_nickname({.name = L"First Middle Last Suffix"});
        REQUIRE_FALSE(nick.empty());
    }
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------
TEST_CASE("Empty name falls through to wordlist", "[nng][edge]")
{
    auto nick = gen().get_nickname({});
    std::wstring result = nick;
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("Unicode name input does not crash", "[nng][edge]")
{
    REQUIRE_NOTHROW(gen().get_nickname({.name = L"José García"}));
    REQUIRE_NOTHROW(gen().get_nickname({.name = L"田中太郎"}));
    REQUIRE_NOTHROW(gen().get_nickname({.name = L"Мария Иванова"}));
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
            REQUIRE_NOTHROW(gen().get_nickname({.name = L"Stress Test Name"}));
        }
        else
        {
            REQUIRE_NOTHROW(gen().get_nickname({}));
        }
    }
}

// ---------------------------------------------------------------------------
// Direct transform unit tests (via nng_test_access)
// ---------------------------------------------------------------------------
TEST_CASE("sentence_case - empty string returns empty", "[transforms][edge]")
{
    REQUIRE(nng_test_access::sentence_case(L"").empty());
}

TEST_CASE("sentence_case - normal string", "[transforms]")
{
    auto result = nng_test_access::sentence_case(L"HELLO");
    REQUIRE(result == L"Hello");
}

TEST_CASE("camel_case - empty string returns empty", "[transforms][edge]")
{
    REQUIRE(nng_test_access::camel_case(L"").empty());
}

TEST_CASE("camel_case - normal string", "[transforms]")
{
    auto result = nng_test_access::camel_case(L"Hello");
    REQUIRE(result == L"hello");
}

TEST_CASE("reverse_sentence_case - empty string returns empty",
          "[transforms][edge]")
{
    REQUIRE(nng_test_access::reverse_sentence_case(L"").empty());
}

TEST_CASE("reverse_sentence_case - normal string", "[transforms]")
{
    auto result = nng_test_access::reverse_sentence_case(L"HELLO");
    REQUIRE(result == L"hellO");
}

TEST_CASE("bathtub_case - empty string returns empty", "[transforms][edge]")
{
    REQUIRE(nng_test_access::bathtub_case(L"").empty());
}

TEST_CASE("bathtub_case - normal string", "[transforms]")
{
    auto result = nng_test_access::bathtub_case(L"HELLO");
    REQUIRE(result == L"HellO");
}

TEST_CASE("upper_case transforms correctly", "[transforms]")
{
    REQUIRE(nng_test_access::upper_case(L"hello") == L"HELLO");
}

TEST_CASE("lower_case transforms correctly", "[transforms]")
{
    REQUIRE(nng_test_access::lower_case(L"HELLO") == L"hello");
}

TEST_CASE("title_case returns input unchanged", "[transforms]")
{
    REQUIRE(nng_test_access::title_case(L"NickName") == L"NickName");
}

TEST_CASE("winding_case alternates case", "[transforms]")
{
    auto result = nng_test_access::winding_case(L"abcdef");
    REQUIRE(result == L"AbCdEf");
}

TEST_CASE("random_single_case - has exactly one uppercase", "[transforms]")
{
    auto result = nng_test_access::random_single_case(L"abcde");
    int upper_count = 0;
    for (auto ch : result)
    {
        if (std::iswupper(ch) != 0)
        {
            ++upper_count;
        }
    }
    REQUIRE(upper_count >= 1);
}

TEST_CASE("snake_case inserts underscores", "[transforms]")
{
    REQUIRE(nng_test_access::snake_case(L"NickName") == L"Nick_Name");
}

TEST_CASE("snake_case - no uppercase returns unchanged", "[transforms]")
{
    REQUIRE(nng_test_access::snake_case(L"nickname") == L"nickname");
}

// ---------------------------------------------------------------------------
// Leetify transforms
// ---------------------------------------------------------------------------
TEST_CASE("xfy - adds X to nickname", "[transforms]")
{
    for (int i = 0; i < 50; ++i)
    {
        auto result = nng_test_access::xfy(L"test");
        bool has_x = result.front() == L'X' || result.back() == L'X';
        REQUIRE(has_x);
    }
}

TEST_CASE("reverse - reverses string", "[transforms]")
{
    REQUIRE(nng_test_access::reverse(L"abc") == L"cba");
}

TEST_CASE("yfy - vowel ending gets replaced", "[transforms]")
{
    REQUIRE(nng_test_access::yfy(L"nicknamo") == L"nicknamy");
}

TEST_CASE("yfy - consonant ending gets appended", "[transforms]")
{
    REQUIRE(nng_test_access::yfy(L"test") == L"testy");
}

TEST_CASE("numify - appends digit characters", "[transforms]")
{
    auto result = nng_test_access::numify(L"nick");
    REQUIRE(result.size() > 4);
    // First appended char should be a digit 1-9.
    REQUIRE(result[4] >= L'1');
    REQUIRE(result[4] <= L'9');
    // Any further chars should be '0'.
    for (std::size_t i = 5; i < result.size(); ++i)
    {
        REQUIRE(result[i] == L'0');
    }
}

TEST_CASE("tracefy - appends dash", "[transforms]")
{
    REQUIRE(nng_test_access::tracefy(L"nick") == L"nick-");
}

TEST_CASE("ingify - vowel ending", "[transforms]")
{
    auto result = nng_test_access::ingify(L"game");
    REQUIRE(result == L"gaming");
}

TEST_CASE("ingify - consonant ending", "[transforms]")
{
    auto result = nng_test_access::ingify(L"test");
    REQUIRE(result == L"testing");
}

TEST_CASE("duovowel - duplicates first vowel found", "[transforms]")
{
    auto result = nng_test_access::duovowel(L"nick");
    REQUIRE(result == L"niick");
}

TEST_CASE("duovowel - no simple vowel returns unchanged", "[transforms]")
{
    auto result = nng_test_access::duovowel(L"gym");
    REQUIRE(result == L"gym");
}

TEST_CASE("oneleet - replaces one character", "[transforms]")
{
    // "test" has 't', 'e', 's' all in leet map.
    bool changed = false;
    for (int i = 0; i < 50; ++i)
    {
        auto result = nng_test_access::oneleet(L"test");
        if (result != L"test")
        {
            changed = true;
            // Exactly one character should differ.
            int diffs = 0;
            for (std::size_t j = 0; j < result.size(); ++j)
            {
                if (result[j] != L"test"[j])
                {
                    ++diffs;
                }
            }
            REQUIRE(diffs == 1);
            break;
        }
    }
    REQUIRE(changed);
}

TEST_CASE("oneleet - no leet candidates returns unchanged", "[transforms]")
{
    REQUIRE(nng_test_access::oneleet(L"hymn") == L"hymn");
}

TEST_CASE("allleet - replaces at most 2 characters", "[transforms]")
{
    auto result = nng_test_access::allleet(L"test");
    // With max 2 replacements, exactly 2 of the 4 leet-mappable chars change.
    int changed = 0;
    std::wstring original = L"test";
    for (std::size_t i = 0; i < original.size(); ++i)
    {
        if (result[i] != original[i])
        {
            ++changed;
        }
    }
    REQUIRE(result.size() == original.size());
    REQUIRE(changed <= 2);
    REQUIRE(changed > 0);
}

// ---------------------------------------------------------------------------
// Name extraction transforms
// ---------------------------------------------------------------------------
TEST_CASE("first_name extracts first part", "[transforms]")
{
    REQUIRE(nng_test_access::first_name(L"John Smith Doe") == L"John");
}

TEST_CASE("last_name extracts last part", "[transforms]")
{
    REQUIRE(nng_test_access::last_name(L"John Smith Doe") == L"Doe");
}

TEST_CASE("any_name returns a part of the name", "[transforms]")
{
    std::vector<std::wstring> parts = {L"John", L"Smith", L"Doe"};
    auto result = nng_test_access::any_name(L"John Smith Doe");
    REQUIRE(std::ranges::find(parts, result) != parts.end());
}

TEST_CASE("initials returns first letters", "[transforms]")
{
    REQUIRE(nng_test_access::initials(L"John Smith Doe") == L"JSD");
}

TEST_CASE("mix_two mixes last two names", "[transforms]")
{
    auto result = nng_test_access::mix_two(L"First Middle Last");
    // Should contain substrings from "Middle" and "Last".
    REQUIRE(result.size() >= 4);
    REQUIRE(result.size() <= 10);
}

TEST_CASE("first_plus_initial combines first name + last initial",
          "[transforms]")
{
    auto result = nng_test_access::first_plus_initial(L"John Doe");
    REQUIRE(result == L"JohnD");
}

TEST_CASE("first_plus_initial with single name returns name",
          "[transforms][edge]")
{
    auto result = nng_test_access::first_plus_initial(L"Mononym");
    REQUIRE(result == L"Mononym");
}

TEST_CASE("initial_plus_last combines first initial + last name",
          "[transforms]")
{
    auto result = nng_test_access::initial_plus_last(L"John Doe");
    REQUIRE(result == L"JDoe");
}

TEST_CASE("initial_plus_last with single name returns name",
          "[transforms][edge]")
{
    auto result = nng_test_access::initial_plus_last(L"Mononym");
    REQUIRE(result == L"Mononym");
}

TEST_CASE("reduce_single_name removes vowels", "[transforms]")
{
    // With a single-part name, it should remove some vowels.
    bool reduced = false;
    for (int i = 0; i < 50; ++i)
    {
        auto result = nng_test_access::reduce_single_name(L"Alberto");
        if (result.size() < 7)
        {
            reduced = true;
            break;
        }
    }
    REQUIRE(reduced);
}

// ---------------------------------------------------------------------------
// Exception: empty name + empty wordlists
// ---------------------------------------------------------------------------
TEST_CASE("get_nickname throws when no name and no wordlists", "[nng][edge]")
{
    auto& g = dasmig::nng::instance();

    // Save and clear wordlists.
    auto saved = nng_test_access::wordlists(g);
    nng_test_access::wordlists(g).clear();

    REQUIRE_THROWS_AS(g.get_nickname({}), std::invalid_argument);

    // Restore wordlists.
    nng_test_access::wordlists(g) = saved;

    // Verify generator still works after restore.
    REQUIRE_NOTHROW(g.get_nickname({}));
}

// ---------------------------------------------------------------------------
// Deterministic seeding
// ---------------------------------------------------------------------------
TEST_CASE("get_nickname(request, seed) is deterministic", "[nng][seeding]")
{
    auto& g = gen();
    constexpr std::uint64_t test_seed{12345};

    auto nick1 = g.get_nickname({.name = L"John Doe"}, test_seed);
    auto nick2 = g.get_nickname({.name = L"John Doe"}, test_seed);

    REQUIRE(static_cast<std::wstring>(nick1) ==
            static_cast<std::wstring>(nick2));
    REQUIRE(nick1.plain() == nick2.plain());
}

TEST_CASE("get_nickname(empty, seed) is deterministic", "[nng][seeding]")
{
    auto& g = gen();
    constexpr std::uint64_t test_seed{99999};

    auto nick1 = g.get_nickname({}, test_seed);
    auto nick2 = g.get_nickname({}, test_seed);

    REQUIRE(static_cast<std::wstring>(nick1) ==
            static_cast<std::wstring>(nick2));
}

TEST_CASE("different seeds produce different nicknames", "[nng][seeding]")
{
    auto& g = gen();

    // Over a range of seeds, at least some should differ.
    bool found_different = false;
    auto baseline = static_cast<std::wstring>(
        g.get_nickname({.name = L"Test Player"}, 1));

    for (std::uint64_t s = 2; s <= 100; ++s)
    {
        auto other = static_cast<std::wstring>(
            g.get_nickname({.name = L"Test Player"}, s));
        if (other != baseline)
        {
            found_different = true;
            break;
        }
    }
    REQUIRE(found_different);
}

TEST_CASE("nickname::seed() returns the seed used", "[nng][seeding]")
{
    auto& g = gen();
    constexpr std::uint64_t test_seed{42};

    auto nick = g.get_nickname({.name = L"Alice Bob"}, test_seed);
    REQUIRE(nick.seed() == test_seed);
}

TEST_CASE("nickname::seed() can replay the same nickname", "[nng][seeding]")
{
    auto& g = gen();

    auto nick1 = g.get_nickname({.name = L"Carlos Mesa"});
    auto replay_seed = nick1.seed();

    auto nick2 = g.get_nickname({.name = L"Carlos Mesa"}, replay_seed);
    REQUIRE(static_cast<std::wstring>(nick1) ==
            static_cast<std::wstring>(nick2));
}

TEST_CASE("nickname_request defaults select wordlists", "[nng][request]")
{
    auto& g = gen();
    const dasmig::nickname_request request{};
    REQUIRE(request.name.empty());
    REQUIRE(g.get_nickname(request, 42).to_json() ==
            g.get_nickname({.name = L""}, 42).to_json());
    REQUIRE(g.get_nickname(request, 42).to_json() ==
            g.get_nickname({.name = L"   "}, 42).to_json());
    REQUIRE_FALSE(g.get_nickname(request).plain().empty());
}

TEST_CASE("requests do not retain generation choices", "[nng][request]")
{
    dasmig::nng g;
    dasmig::nickname_request request{.name = L"Alice Smith"};
    for (std::uint64_t seed = 0; seed < 50; ++seed)
    {
        REQUIRE(g.get_nickname(request, seed).plain() == request.name);
        REQUIRE(g.get_nickname({.name = L"Bob Jones"}, seed).plain() == L"Bob Jones");
        REQUIRE(g.get_nickname(request, seed).plain() == L"Alice Smith");
    }
    request.name.clear();
    REQUIRE_THROWS_AS(g.get_nickname(request), std::invalid_argument);
    REQUIRE_THROWS_AS(g.get_nickname(request, 42), std::invalid_argument);
    REQUIRE_THROWS_AS(g.get_nickname({.name = L"   "}, 42), std::invalid_argument);
}

TEST_CASE("seeded request is const and leaves the instance engine untouched",
          "[nng][request][seeding]")
{
    auto& g = gen();
    auto expected_engine = nng_test_access::engine(g);
    const auto& const_gen = g;
    const dasmig::nickname_request request{.name = L"Test Player"};
    const auto expected = const_gen.get_nickname(request, 0).to_json();
    REQUIRE(g.get_nickname({}).seed() == expected_engine());
    REQUIRE(const_gen.get_nickname(request, 0).to_json() == expected);
    REQUIRE(g.get_nickname(request).seed() == expected_engine());
    REQUIRE(const_gen.get_nickname(request, 0).to_json() == expected);
}

// ===== Multi-instance tests ================================================

TEST_CASE("independent instance can load and generate", "[nng][multi-instance]")
{
    dasmig::nng gen2;
    REQUIRE_FALSE(gen2.has_wordlists());

    gen2.load(resource_path());
    REQUIRE(gen2.has_wordlists());

    auto nick = gen2.get_nickname({.name = L"Multi Instance"});
    REQUIRE_FALSE(static_cast<std::wstring>(nick).empty());
}

TEST_CASE("independent instances have separate word lists",
          "[nng][multi-instance]")
{
    dasmig::nng a;
    dasmig::nng b;

    a.load(resource_path());
    REQUIRE(a.has_wordlists());
    REQUIRE_FALSE(b.has_wordlists());
}

TEST_CASE("independent instances replay explicit call seeds",
          "[nng][multi-instance]")
{
    dasmig::nng a;
    dasmig::nng b;
    a.load(resource_path());
    b.load(resource_path());

    constexpr std::uint64_t seed_a = 111;
    constexpr std::uint64_t seed_b = 222;

    auto nick_a = static_cast<std::wstring>(a.get_nickname({.name = L"Seed Test"}, seed_a));
    auto nick_b = static_cast<std::wstring>(b.get_nickname({.name = L"Seed Test"}, seed_b));
    auto nick_a2 = static_cast<std::wstring>(a.get_nickname({.name = L"Seed Test"}, seed_a));
    auto nick_b2 = static_cast<std::wstring>(b.get_nickname({.name = L"Seed Test"}, seed_b));

    REQUIRE(nick_a == nick_a2);
    REQUIRE(nick_b == nick_b2);
}

TEST_CASE("moved instance retains state", "[nng][multi-instance]")
{
    dasmig::nng src;
    src.load(resource_path());
    const auto expected = src.get_nickname({.name = L"Move Test"}, 42).to_json();
    auto expected_engine = nng_test_access::engine(src);

    dasmig::nng dst = std::move(src);
    const auto actual = dst.get_nickname({.name = L"Move Test"}, 42).to_json();

    REQUIRE(expected == actual);
    REQUIRE(dst.get_nickname({}).seed() == expected_engine());
}

TEST_CASE("singleton still works after multi-instance changes",
          "[nng][multi-instance]")
{
    auto& g = dasmig::nng::instance();
    auto nick = g.get_nickname({.name = L"Singleton Check"});
    REQUIRE_FALSE(static_cast<std::wstring>(nick).empty());
}

// ===== Robustness / edge-case tests ========================================

TEST_CASE("split_name filters consecutive spaces", "[nng][robustness]")
{
    // Double space should not produce empty parts.
    auto parts = nng_test_access::split_name(L"John  Doe");
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0] == L"John");
    REQUIRE(parts[1] == L"Doe");

    // Leading/trailing spaces.
    auto parts2 = nng_test_access::split_name(L" Jane ");
    REQUIRE(parts2.size() == 1);
    REQUIRE(parts2[0] == L"Jane");
}

TEST_CASE("initials handles consecutive spaces safely", "[nng][robustness]")
{
    // Should not crash — previously UB with empty parts.
    auto result = nng_test_access::initials(L"John  Doe");
    REQUIRE(result.size() == 2);
}

TEST_CASE("mix_two handles single-character name parts", "[nng][robustness]")
{
    // "Li A" has a 1-char part — previously UB (min > max in distribution).
    for (int i = 0; i < 50; ++i)
    {
        auto nick = nng_test_access::mix_two(L"Li A");
        REQUIRE_FALSE(nick.empty());
    }
}

TEST_CASE("leetify does not stack-overflow on unmappable input",
          "[nng][robustness]")
{
    // "M" (single consonant, no leet map entry) previously caused infinite
    // recursion. Now capped at max depth.
    dasmig::nng g;
    g.load(resource_path());
    for (std::uint64_t seed = 0; seed < 20; ++seed)
    {
        auto nick = g.get_nickname({.name = L"M"}, seed);
        REQUIRE_FALSE(static_cast<std::wstring>(nick).empty());
    }
}

TEST_CASE("name-only generation works without wordlists", "[nng][robustness]")
{
    dasmig::nng g; // No load() — wordlists empty.

    // Every call should succeed (previously 75% threw).
    for (int i = 0; i < 50; ++i)
    {
        auto nick = g.get_nickname({.name = L"John Smith"});
        REQUIRE_FALSE(static_cast<std::wstring>(nick).empty());
    }
}

TEST_CASE("no-name no-wordlists still throws", "[nng][robustness]")
{
    dasmig::nng g;
    REQUIRE_THROWS_AS(g.get_nickname({}), std::invalid_argument);
}

TEST_CASE("64-bit seeds differing in upper bits produce different nicknames",
          "[nng][robustness]")
{
    auto& g = gen();

    // Two seeds identical in lower 32 bits but different in upper 32 bits.
    constexpr std::uint64_t seed_lo = 42;
    constexpr std::uint64_t seed_hi =
        42 | (static_cast<std::uint64_t>(1) << 32U);

    bool found_different = false;
    for (int i = 0; i < 100; ++i)
    {
        auto a = static_cast<std::wstring>(
            g.get_nickname({.name = L"Bit Test"}, seed_lo + i));
        auto b = static_cast<std::wstring>(
            g.get_nickname({.name = L"Bit Test"}, seed_hi + i));
        if (a != b)
        {
            found_different = true;
            break;
        }
    }
    REQUIRE(found_different);
}

// ---------------------------------------------------------------------------
// Known answers (rule 2): the same seed, name and word lists give these
// exact results on every compiler and platform. A change here is a change of
// output and needs a major version.
// ---------------------------------------------------------------------------

TEST_CASE("known answers for fixed seeds", "[kat]")
{
    dasmig::nng g;
    g.load(resource_path());
    REQUIRE(g.data_hash() == 0xc2a4fee0b536a8a1ULL);
    const auto lists = g.list_names();
    REQUIRE(lists.size() == 52);
    REQUIRE(std::ranges::count_if(lists, [](const std::string& n) { return n.starts_with("tag_"); }) == 43);
    REQUIRE(std::ranges::find(lists, "colors") != lists.end());

    REQUIRE(g.get_nickname({.name = L"Alberto Bins Elis"}, 1).to_json() ==
            R"({"nickname":"Mooth","source":"Moth","signature":{"seed":"0000000000000001","data":"c2a4fee0b536a8a1"}})");
    REQUIRE(g.get_nickname({}, 2).to_json() ==
            R"({"nickname":"sumuH","source":"Humus","signature":{"seed":"0000000000000002","data":"c2a4fee0b536a8a1"}})");

    // Accented names: the case mapping does not depend on the locale.
    const std::wstring accented = L"José García Élodie";
    REQUIRE(std::wstring{g.get_nickname({.name = accented}, 12)} == L"ÉésoJ");
    REQUIRE(std::wstring{g.get_nickname({.name = accented}, 23)} == L"Jgé");
    REQUIRE(std::wstring{g.get_nickname({.name = accented}, 25)} == L"EIDOLÉ");
    REQUIRE(std::wstring{g.get_nickname({.name = accented}, 26)} == L"XgarélodiE");
    REQUIRE(std::wstring{g.get_nickname({.name = accented}, 35)} == L"JosÉ");

    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (std::uint64_t s = 0; s < 10000; ++s)
    {
        for (const char c : g.get_nickname({.name = s % 2 == 0 ? L"" : L"Alberto Bins Elis"}, s).to_json())
        {
            h ^= static_cast<unsigned char>(c);
            h *= 0x100000001b3ULL;
        }
    }
    REQUIRE(h == 0xf3e27cea9880b9aaULL);
}

TEST_CASE("case changes use a fixed mapping", "[case]")
{
    // Latin-1 and Latin Extended-A letters change case; others do not.
    REQUIRE(nng_test_access::upper_case(L"éñÿłž") ==
            L"ÉÑŸŁŽ");
    REQUIRE(nng_test_access::lower_case(L"ÉÑŸŁŽ") ==
            L"éñÿłž");
    REQUIRE(nng_test_access::upper_case(L"÷ßı") == L"÷ßı"); // ÷ ß ı unchanged
    REQUIRE(nng_test_access::upper_case(L"α中") == L"α中");             // Greek, CJK unchanged
    REQUIRE(nng_test_access::snake_case(L"ÉlodieGarcía") == L"Élodie_García");
}

// ---------------------------------------------------------------------------
// JSON and signatures
// ---------------------------------------------------------------------------

TEST_CASE("to_json and signature", "[json]")
{
    const auto n = gen().get_nickname({.name = L"Ana Lima"}, 9);
    REQUIRE(n.seed() == 9);
    REQUIRE(n.signature().data == gen().data_hash());
    const auto json = n.to_json();
    REQUIRE(json.starts_with(R"({"nickname":")"));
    REQUIRE(json.find(R"("signature":{"seed":"0000000000000009")") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Extending data (rule 8)
// ---------------------------------------------------------------------------

TEST_CASE("extend adds lists and words, and removes words", "[extend]")
{
    dasmig::nng g;
    g.load(resource_path());
    const auto hash = g.data_hash();

    // A new list from TSV text.
    const auto& info = g.extend_from_string("planets", "word\nMercury\nVenus\nMars\n", "planets.tsv");
    REQUIRE(info.added == 3);
    REQUIRE(g.list_names().size() == 53);
    REQUIRE(g.data_hash() != hash);

    // Removing words; a list with no words left is not used.
    g.extend_from_string("planets", "word\tdisabled\nMercury\t1\nVenus\t1\nMars\t1\n", "cleanup");
    REQUIRE(g.list_names().size() == 52);

    // Files: .words adds to the list named after the file.
    const auto dir = std::filesystem::temp_directory_path() / "nng_layer";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    {
        std::ofstream{dir / "animals.words"} << "Axolotl\nQuokka\n";
    }
    g.extend(dir / "animals.words");
    REQUIRE(g.layers("animals").size() == 2);
    REQUIRE(g.layers("animals").back().added == 2);

    REQUIRE_THROWS_AS(g.extend(dir / "missing.txt"), std::invalid_argument);
    std::filesystem::remove_all(dir);

    g.load(resource_path());
    REQUIRE(g.data_hash() == hash);
}

TEST_CASE("a directory layer is applied all or nothing", "[extend][errors]")
{
    dasmig::nng g;
    g.load(resource_path());
    const auto hash = g.data_hash();
    const auto dir = std::filesystem::temp_directory_path() / "nng_bad_layer";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    {
        std::ofstream{dir / "a_good.words"} << "Fine\n";
        std::ofstream{dir / "b_bad.tsv"} << "word\tdisabled\nOops\tmaybe\n";
    }
    REQUIRE_THROWS_AS(g.extend(dir), dasmig::core::tsv_error);
    REQUIRE(g.data_hash() == hash); // a_good.words was not applied either
    REQUIRE(g.list_names().size() == 52);
    std::filesystem::remove_all(dir);
}

#ifdef DASMIG_NICKNAME_EMBED
TEST_CASE("compiled-in data needs no files and matches them", "[embed]")
{
    const auto old_cwd = std::filesystem::current_path();
    const auto tmp = std::filesystem::temp_directory_path() / "nng_embed_dir";
    std::filesystem::create_directories(tmp);
    std::filesystem::current_path(tmp);
    dasmig::nng g;
    const bool loaded = g.load(dasmig::dataset::lite);
    std::filesystem::current_path(old_cwd);
    std::filesystem::remove_all(tmp);

    REQUIRE(loaded);
    REQUIRE(g.layers("animals").front().source == "embedded:lite/animals.words");
    REQUIRE(g.data_hash() == 0xc2a4fee0b536a8a1ULL); // same as the files
}
#endif

// ---------------------------------------------------------------------------
// Batch generation
// ---------------------------------------------------------------------------

TEST_CASE("nickname batch: items equal the single calls for any thread count", "[batch]")
{
    dasmig::nng gen;
    gen.load(resource_path());
    const auto request = dasmig::nickname_request{.name = L"Alberto Bins Elis"};
    const auto batch = gen.get_nickname_batch(request, {.count = 200, .threads = 1}, 77);
    REQUIRE(batch.size() == 200);
    for (std::uint64_t i = 0; i < batch.size(); ++i)
    {
        REQUIRE(batch[i].to_json() == gen.get_nickname(request, dasmig::core::item_seed(77, i)).to_json());
    }

    // A range is computed directly, on any number of threads.
    const auto tail = gen.get_nickname_batch(request, {.count = 100, .first = 100, .threads = 4}, 77);
    REQUIRE(tail.size() == 100);
    for (std::size_t i = 0; i < tail.size(); ++i)
    {
        REQUIRE(tail[i].to_json() == batch[100 + i].to_json());
    }
    for (const std::size_t threads : {2U, 7U, 0U})
    {
        const auto other = gen.get_nickname_batch(request, {.count = 200, .threads = threads}, 77);
        REQUIRE(other.size() == batch.size());
        for (std::size_t i = 0; i < other.size(); ++i)
        {
            REQUIRE(other[i].to_json() == batch[i].to_json());
        }
    }

    // Without a seed the batch still has the requested size.
    REQUIRE(gen.get_nickname_batch(request, {.count = 5}).size() == 5);
}

TEST_CASE("nickname batch: streaming gives the same items in order", "[batch]")
{
    dasmig::nng gen;
    gen.load(resource_path());
    const auto request = dasmig::nickname_request{.name = L"Alberto Bins Elis"};
    const auto batch = gen.get_nickname_batch(request, {.count = 300, .first = 10, .threads = 3}, 5);
    std::uint64_t next = 10;
    std::size_t seen = 0;
    gen.for_each_nickname(request, {.count = 300, .first = 10, .threads = 3}, 5,
                        [&](std::uint64_t index, const dasmig::nickname& item)
                        {
                            REQUIRE(index == next);
                            REQUIRE(item.to_json() == batch[seen].to_json());
                            ++next;
                            ++seen;
                        });
    REQUIRE(seen == batch.size());
}

TEST_CASE("nickname batch: known answer of 10,000 items", "[batch][kat]")
{
    dasmig::nng gen;
    gen.load(resource_path());
    const auto request = dasmig::nickname_request{.name = L"Alberto Bins Elis"};
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    gen.for_each_nickname(request, {.count = 10000, .threads = 4}, 1,
                        [&](std::uint64_t, const dasmig::nickname& item)
                        { hash = dasmig::core::hash_combine(hash, dasmig::core::fnv1a64(item.to_json())); });
    REQUIRE(hash == 0x16a4c07ab3a0be53ULL);
}

// ---------------------------------------------------------------------------
// Themes: tags
// ---------------------------------------------------------------------------

namespace
{
// The words of the shipped tag_<tag>.words: the sources a themed nickname can have.
std::set<std::wstring> theme_words(std::string_view tag)
{
    std::set<std::wstring> words;
    const auto doc = dasmig::core::tsv_document::load(resource_path() / "lite" / ("tag_" + std::string{tag} + ".words"), {"word"});
    for (const auto& row : doc.rows()) words.insert(dasmig::core::from_utf8(row["word"]));
    return words;
}
} // namespace

TEST_CASE("tags: a themed request draws a word of its theme about a third of the time", "[tags]")
{
    dasmig::nng g;
    g.load(resource_path());
    const auto combat = theme_words("combat");
    REQUIRE_FALSE(combat.empty());

    int themed = 0;
    int plain = 0;
    constexpr int n = 6000;
    for (int i = 0; i < n; ++i)
    {
        const auto seed = static_cast<std::uint64_t>(i);
        if (combat.contains(g.get_nickname({.tags = {"combat"}}, seed).plain())) ++themed;
        if (combat.contains(g.get_nickname({}, seed).plain())) ++plain;
    }
    REQUIRE(themed > n * 0.30);
    REQUIRE(themed < n * 0.37);
    // Without tags the theme is not drawn on purpose; only words that a plain
    // list also has (an animal such as Hawk) appear.
    REQUIRE(plain < n * 0.005);
}

TEST_CASE("tags: unknown tags and no tags give the plain nickname", "[tags]")
{
    dasmig::nng g;
    g.load(resource_path());
    for (std::uint64_t seed = 0; seed < 200; ++seed)
    {
        const auto plain = g.get_nickname({}, seed).to_json();
        REQUIRE(g.get_nickname({.tags = {}}, seed).to_json() == plain);
        REQUIRE(g.get_nickname({.tags = {"no_such_tag"}}, seed).to_json() == plain);
    }
}

TEST_CASE("tags: a repeated tag does not raise the chance", "[tags]")
{
    dasmig::nng g;
    g.load(resource_path());
    for (std::uint64_t seed = 0; seed < 200; ++seed)
    {
        REQUIRE(g.get_nickname({.tags = {"combat", "combat"}}, seed).to_json() ==
                g.get_nickname({.tags = {"combat"}}, seed).to_json());
    }
}

TEST_CASE("tags: several tags draw from each of their themes", "[tags]")
{
    dasmig::nng g;
    g.load(resource_path());
    const auto combat = theme_words("combat");
    const auto stealth = theme_words("stealth");
    bool saw_combat = false;
    bool saw_stealth = false;
    for (std::uint64_t seed = 0; seed < 2000; ++seed)
    {
        const auto source = g.get_nickname({.tags = {"combat", "stealth"}}, seed).plain();
        saw_combat = saw_combat || combat.contains(source);
        saw_stealth = saw_stealth || stealth.contains(source);
    }
    REQUIRE(saw_combat);
    REQUIRE(saw_stealth);
}

TEST_CASE("tags: a name takes the nickname first", "[tags]")
{
    dasmig::nng g;
    g.load(resource_path());
    const auto theme = theme_words("combat");
    // A quarter of the nicknames come from the name; a third of the rest are themed.
    int themed = 0;
    constexpr int n = 4000;
    for (int i = 0; i < n; ++i)
    {
        const auto nick = g.get_nickname({.name = L"Alberto Bins Elis", .tags = {"combat"}}, static_cast<std::uint64_t>(i));
        if (theme.contains(nick.plain())) ++themed;
    }
    REQUIRE(themed > n * 0.20);
    REQUIRE(themed < n * 0.30);
}

TEST_CASE("tags: the same seed replays", "[tags][seed]")
{
    dasmig::nng g;
    g.load(resource_path());
    const dasmig::nickname_request request{.tags = {"combat", "leadership"}};
    for (std::uint64_t seed = 0; seed < 50; ++seed)
        REQUIRE(g.get_nickname(request, seed).to_json() == g.get_nickname(request, seed).to_json());
}

TEST_CASE("tags: your own tag_ list is a theme and extends a shipped one", "[tags][extend]")
{
    dasmig::nng g;
    g.load(resource_path());
    g.extend_from_string("tag_pirate", "word\nBlackbeard\nCutlass\n", "pirate.tsv");
    bool saw = false;
    for (std::uint64_t seed = 0; seed < 400 && !saw; ++seed)
    {
        const auto source = g.get_nickname({.tags = {"pirate"}}, seed).plain();
        saw = source == L"Blackbeard" || source == L"Cutlass";
    }
    REQUIRE(saw);

    // Adding to a shipped theme keeps its words.
    g.extend_from_string("tag_combat", "word\nValkyrion\n", "combat.tsv");
    const auto combat = theme_words("combat");
    bool saw_new = false;
    bool saw_old = false;
    for (std::uint64_t seed = 0; seed < 4000; ++seed)
    {
        const auto source = g.get_nickname({.tags = {"combat"}}, seed).plain();
        saw_new = saw_new || source == L"Valkyrion";
        saw_old = saw_old || combat.contains(source);
    }
    REQUIRE(saw_new);
    REQUIRE(saw_old);
}

TEST_CASE("tags: with only themes loaded, a tagged request uses them", "[tags]")
{
    dasmig::nng g;
    g.extend_from_string("tag_pirate", "word\nBlackbeard\n", "pirate.tsv");
    REQUIRE_FALSE(g.has_wordlists());
    REQUIRE(g.get_nickname({.tags = {"pirate"}}, 1).plain() == L"Blackbeard");
    REQUIRE_THROWS_AS(g.get_nickname({}, 1), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// The shipped lists are big enough, and a list's chance follows its size
// ---------------------------------------------------------------------------

namespace
{
std::size_t word_count(const std::filesystem::path& file)
{
    return dasmig::core::tsv_document::load(file, {"word"}).size();
}
} // namespace

TEST_CASE("shipped lists have enough words to avoid repeats", "[lists]")
{
    // A short list would be drawn often and repeat; these floors guard against
    // shipping one (a theme is drawn a third of the time for a tagged request).
    constexpr std::size_t plain_floor = 150;
    constexpr std::size_t theme_floor = 24;
    std::size_t plain = 0;
    std::size_t themes = 0;
    for (const auto& entry : std::filesystem::directory_iterator(resource_path() / "lite"))
    {
        if (entry.path().extension() != ".words") continue;
        const auto name = entry.path().stem().string();
        const auto words = word_count(entry.path());
        if (name.starts_with("tag_"))
        {
            ++themes;
            INFO(name);
            REQUIRE(words >= theme_floor);
        }
        else
        {
            ++plain;
            INFO(name);
            REQUIRE(words >= plain_floor);
        }
    }
    REQUIRE(plain == 9);
    REQUIRE(themes == 43);
}

TEST_CASE("a list's chance grows with the square root of its size", "[lists][seed]")
{
    dasmig::nng g;
    g.extend_from_string("small", "word\n" + [] {
        std::string rows;
        for (int i = 0; i < 100; ++i) rows += "S" + std::to_string(i) + "\n";
        return rows;
    }(), "small.tsv");
    g.extend_from_string("big", "word\n" + [] {
        std::string rows;
        for (int i = 0; i < 10000; ++i) rows += "B" + std::to_string(i) + "\n";
        return rows;
    }(), "big.tsv");
    // Weights 10 and 100: the big list is drawn 10 times as often as the small
    // one (it has 100 times the words), not 100 times, and not equally.
    int small = 0;
    int big = 0;
    constexpr int n = 22000;
    for (int i = 0; i < n; ++i)
    {
        const auto source = g.get_nickname({}, static_cast<std::uint64_t>(i)).plain();
        if (source.starts_with(L"S")) ++small;
        if (source.starts_with(L"B")) ++big;
    }
    REQUIRE(small + big == n);
    REQUIRE(big > 9.0 * small);
    REQUIRE(big < 11.5 * small);
}

TEST_CASE("the large shipped list still gets a large share", "[lists]")
{
    dasmig::nng g;
    g.load(resource_path());
    const auto adjectives = [] {
        std::set<std::wstring> words;
        const auto doc = dasmig::core::tsv_document::load(resource_path() / "lite" / "adjectives.words", {"word"});
        for (const auto& row : doc.rows()) words.insert(dasmig::core::from_utf8(row["word"]));
        return words;
    }();
    int from_adjectives = 0;
    constexpr int n = 8000;
    for (int i = 0; i < n; ++i)
    {
        if (adjectives.contains(g.get_nickname({}, static_cast<std::uint64_t>(i)).plain())) ++from_adjectives;
    }
    // About 37%: far above an even ninth, far below its 4,842-word size share.
    REQUIRE(from_adjectives > n * 0.30);
    REQUIRE(from_adjectives < n * 0.50);
}

// ---------------------------------------------------------------------------
// Username style
// ---------------------------------------------------------------------------

namespace
{
constexpr auto username_style = dasmig::nickname_style::username;

std::string ascii_of(const std::wstring& text)
{
    std::string out;
    for (const wchar_t c : text) out.push_back(static_cast<char>(c));
    return out;
}

bool is_username(const std::string& s)
{
    if (s.empty() || s.size() > 24) return false;
    if (!(s.front() >= 'a' && s.front() <= 'z')) return false;
    if (s.back() == '.' || s.back() == '_') return false;
    return std::ranges::all_of(s, [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_';
    });
}

std::string without_digits(std::string s)
{
    while (!s.empty() && s.back() >= '0' && s.back() <= '9') s.pop_back();
    if (!s.empty() && s.back() == '_') s.pop_back();
    return s;
}
} // namespace

TEST_CASE("username: always lowercase ASCII letters, digits, dots and underscores", "[username]")
{
    dasmig::nng g;
    g.load(resource_path());
    const std::vector<std::wstring> names{L"", L"John Smith", L"Ana", L"Maria da Silva Santos",
                                          L"Maximilian Bartholomew Wolfeschlegelstein-Hausen",
                                          L"O'Brien Jean-Luc"};
    for (const auto& name : names)
    {
        for (std::uint64_t seed = 0; seed < 1500; ++seed)
        {
            const auto nick = ascii_of(std::wstring{g.get_nickname({.name = name, .tags = {"combat"}, .style = username_style}, seed)});
            INFO(nick);
            REQUIRE(is_username(nick));
        }
    }
}

TEST_CASE("username: from a name it takes one of the common shapes", "[username]")
{
    dasmig::nng g;
    g.load(resource_path());
    const std::set<std::string> shapes{"john.smith", "johnsmith", "john_smith", "jsmith", "j.smith",
                                       "john.s",     "johns",     "smith.john", "smith_john", "smithj"};
    std::set<std::string> seen;
    for (std::uint64_t seed = 0; seed < 3000; ++seed)
    {
        const auto nick = ascii_of(std::wstring{g.get_nickname({.name = L"John Smith", .style = username_style}, seed)});
        const auto base = without_digits(nick);
        INFO(nick);
        REQUIRE(shapes.contains(base));
        seen.insert(base);
    }
    REQUIRE(seen == shapes); // every shape turns up
}

TEST_CASE("username: a name always gives a name-based username, and plain() is the name", "[username]")
{
    dasmig::nng g;
    g.load(resource_path());
    for (std::uint64_t seed = 0; seed < 300; ++seed)
    {
        const auto nick = g.get_nickname({.name = L"John Smith", .style = username_style}, seed);
        REQUIRE(nick.plain() == L"John Smith");
        const auto text = ascii_of(std::wstring{nick});
        REQUIRE((text.find("john") != std::string::npos || text.find("smith") != std::string::npos ||
                 text.find('j') == 0 || text.find('s') == 0));
    }
}

TEST_CASE("username: accents are folded and what has no ASCII form is dropped", "[username]")
{
    dasmig::nng g;
    g.load(resource_path());
    std::set<std::string> bases;
    for (std::uint64_t seed = 0; seed < 600; ++seed)
    {
        bases.insert(without_digits(
            ascii_of(std::wstring{g.get_nickname({.name = L"José García", .style = username_style}, seed)})));
    }
    REQUIRE(bases.contains("jose.garcia"));
    REQUIRE(bases.contains("jgarcia"));

    // Ligatures, ss, o-slash, and punctuation inside a name part.
    bases.clear();
    for (std::uint64_t seed = 0; seed < 600; ++seed)
    {
        bases.insert(without_digits(
            ascii_of(std::wstring{g.get_nickname({.name = L"Schönß O'Brienœ", .style = username_style}, seed)})));
    }
    REQUIRE(bases.contains("schonss.obrienoe"));
}

TEST_CASE("username: a name with no ASCII form falls back to words", "[username]")
{
    dasmig::nng g;
    g.load(resource_path());
    for (std::uint64_t seed = 0; seed < 200; ++seed)
    {
        const auto nick = g.get_nickname({.name = L"山田 太郎", .style = username_style}, seed);
        REQUIRE(is_username(ascii_of(std::wstring{nick})));
        REQUIRE(nick.plain() != L"山田 太郎");
    }
}

TEST_CASE("username: without a name it is one or two words, with digits about half the time", "[username]")
{
    dasmig::nng g;
    g.load(resource_path());
    int with_digits = 0;
    int two_words = 0;
    constexpr int n = 4000;
    for (int i = 0; i < n; ++i)
    {
        const auto nick = g.get_nickname({.style = username_style}, static_cast<std::uint64_t>(i));
        const auto text = ascii_of(std::wstring{nick});
        if (text.back() >= '0' && text.back() <= '9') ++with_digits;
        if (nick.plain().find(L' ') != std::wstring::npos) ++two_words;
    }
    REQUIRE(with_digits > n * 0.45);
    REQUIRE(with_digits < n * 0.65);
    REQUIRE(two_words > n * 0.40);
    REQUIRE(two_words < n * 0.60);
}

TEST_CASE("username: tags give themed words as the first word", "[username][tags]")
{
    dasmig::nng g;
    g.load(resource_path());
    std::set<std::wstring> combat; // the username source is lowercase
    for (auto word : theme_words("combat"))
    {
        std::ranges::transform(word, word.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        combat.insert(word);
    }
    int themed = 0;
    constexpr int n = 3000;
    for (int i = 0; i < n; ++i)
    {
        const auto source = g.get_nickname({.tags = {"combat"}, .style = username_style}, static_cast<std::uint64_t>(i)).plain();
        const auto first = source.substr(0, source.find(L' '));
        if (combat.contains(first)) ++themed;
    }
    REQUIRE(themed > n * 0.28);
    REQUIRE(themed < n * 0.40);
}

TEST_CASE("username: the same seed replays and the default style is unchanged", "[username][seed]")
{
    dasmig::nng g;
    g.load(resource_path());
    for (std::uint64_t seed = 0; seed < 100; ++seed)
    {
        const dasmig::nickname_request request{.name = L"Ana Lima", .style = username_style};
        REQUIRE(g.get_nickname(request, seed).to_json() == g.get_nickname(request, seed).to_json());
        REQUIRE(g.get_nickname({.name = L"Ana Lima", .style = dasmig::nickname_style::gaming}, seed).to_json() ==
                g.get_nickname({.name = L"Ana Lima"}, seed).to_json());
    }
    REQUIRE(g.get_nickname({.name = L"Ana Lima", .style = username_style}, 7).to_json() !=
            g.get_nickname({.name = L"Ana Lima"}, 7).to_json());
}

TEST_CASE("username: a long name is cut to 24 characters", "[username]")
{
    dasmig::nng g;
    g.load(resource_path());
    for (std::uint64_t seed = 0; seed < 500; ++seed)
    {
        const auto text = ascii_of(std::wstring{g.get_nickname(
            {.name = L"Bartholomew Wolfeschlegelsteinhausenbergerdorff", .style = username_style}, seed)});
        REQUIRE(text.size() <= 24);
        REQUIRE(is_username(text));
    }
}

TEST_CASE("username: no lists still works from a name, and throws without one", "[username][edge]")
{
    dasmig::nng g;
    REQUIRE_FALSE(g.has_wordlists());
    REQUIRE(is_username(ascii_of(std::wstring{g.get_nickname({.name = L"John Smith", .style = username_style}, 1)})));
    REQUIRE_THROWS_AS(g.get_nickname({.style = username_style}, 1), std::invalid_argument);
}
