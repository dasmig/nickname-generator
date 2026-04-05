#ifndef DASMIG_NICKNAMEGEN_HPP
#define DASMIG_NICKNAMEGEN_HPP

#include "random.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/// @file nicknamegen.hpp
/// @brief Nickname generator library — gamer-style nickname generation for C++23.
/// @author Diego Dasso Migotto (diegomigotto at hotmail dot com)
/// @see See doc/usage.md for the narrative tutorial.

struct nng_test_access;

namespace dasmig
{

/// @brief Return type for nickname generation, holding both the transformed
/// and original strings.
///
/// Supports implicit conversion to std::wstring and streaming via operator<<.
class nickname
{
  public:
    /// @brief Return the original word or name used to generate this nickname.
    /// @return Original source string before any transforms.
    [[nodiscard]] std::wstring plain() const
    {
        return _original_string;
    }

    /// @brief Retrieve the random seed used to generate this nickname.
    /// @return The per-call seed for replay.
    /// @see nng::get_nickname(const std::wstring&, std::uint64_t)
    [[nodiscard]] std::uint64_t seed() const
    {
        return _seed;
    }

    /// @brief Implicit conversion to std::wstring.
    /// @return The transformed nickname string.
    operator std::wstring() const // NOLINT(hicpp-explicit-conversions)
    {
        return _internal_string;
    }

    /// @brief Stream the nickname to a wide output stream.
    /// @param wos Output stream.
    /// @param nickname Nickname to stream.
    /// @return Reference to the output stream.
    friend std::wostream& operator<<(std::wostream& wos,
                                     const nickname& nickname)
    {
        wos << nickname._internal_string;
        return wos;
    }

  private:
    /// @brief Private constructor — nicknames are created only by nng.
    /// @param nickname_str The transformed nickname text.
    /// @param original_str The original source text.
    nickname(std::wstring nickname_str, std::wstring original_str,
             std::uint64_t seed = 0)
        : _internal_string(std::move(nickname_str)),
          _original_string(std::move(original_str)),
          _seed(seed)
    {
    }

    std::wstring _internal_string; ///< Current nickname after transforms.

    std::wstring _original_string; ///< Original source string.

    std::uint64_t _seed{0}; ///< Random seed used to generate this nickname.

    friend class nng; ///< Allows nng to construct nicknames.
};

/// @brief Nickname generator that produces gamer-style nicknames.
///
/// Generates realistic gamer nicknames akin to professional players, optionally
/// based on a player's real name. Applies random "leetifying" transforms and
/// case formatting for variety.
///
/// Can be used as a singleton via instance() or constructed independently.
/// Independent instances own their own word lists and random engine,
/// making them safe for concurrent use without shared state.
///
/// @par Thread safety
/// Each instance is independent. Concurrent calls to get_nickname() on
/// the **same** instance require external synchronization. load() mutates
/// internal state and must not be called concurrently with get_nickname()
/// on the same instance.
class nng
{
  public:
    /// @brief Default constructor — creates an empty generator with no word lists.
    ///
    /// Call load() to populate word lists before generating nicknames.
    nng() = default;

    nng(const nng&) = delete;            ///< Not copyable.
    nng& operator=(const nng&) = delete; ///< Not copyable.
    nng(nng&&) = default;                ///< Move constructor.
    nng& operator=(nng&&) = default;     ///< Move assignment.
    ~nng() = default;                    ///< Default destructor.

    /// @brief Access the global singleton instance.
    ///
    /// The singleton auto-probes common resource paths on first access.
    /// For independent generators (e.g., inside an entity-generator component),
    /// prefer constructing a separate nng instance.
    /// @return Reference to the global nng instance.
    static nng& instance()
    {
        static nng inst{auto_probe_tag{}};
        return inst;
    }

    /// @brief Generate a nickname, optionally based on a name.
    /// @param name Optional full name to base the nickname on. If empty, a
    ///        random word from the loaded word lists is used instead.
    /// @return A nickname object with both transformed and original strings.
    /// @throws std::invalid_argument If name is empty and no word lists loaded.
    [[nodiscard]] nickname get_nickname(const std::wstring& name = L"")
    {
        auto call_seed = static_cast<std::uint64_t>(_engine());
        effolkronium::random_local call_engine;
        call_engine.seed(static_cast<std::mt19937::result_type>(
            (call_seed ^ (call_seed >> 32U))));
        auto result = solver(name, call_engine);
        result._seed = call_seed;
        return result;
    }

    /// @brief Generate a deterministic nickname using a specific seed.
    ///
    /// Given the same name and seed, this method always produces the same
    /// nickname. Retrieve the seed from a previous nickname via
    /// nickname::seed().
    ///
    /// @param name Full name to base the nickname on (may be empty).
    /// @param call_seed Seed for reproducible results.
    /// @return A nickname object with both transformed and original strings.
    /// @throws std::invalid_argument If name is empty and no word lists loaded.
    [[nodiscard]] nickname get_nickname(const std::wstring& name,
                                        std::uint64_t call_seed) const
    {
        effolkronium::random_local call_engine;
        call_engine.seed(static_cast<std::mt19937::result_type>(
            (call_seed ^ (call_seed >> 32U))));
        auto result = solver(name, call_engine);
        result._seed = call_seed;
        return result;
    }

    /// @name Seeding
    /// @{

    /// @brief Seed the internal random engine for deterministic sequences.
    ///
    /// Subsequent get_nickname() calls (without an explicit seed) draw
    /// per-call seeds from this engine, producing a reproducible sequence.
    ///
    /// @param seed_value The seed value.
    /// @return `*this` for chaining.
    nng& seed(std::uint64_t seed_value)
    {
        _engine.seed(seed_value);
        return *this;
    }

    /// @brief Reseed the engine with a non-deterministic source.
    ///
    /// Subsequent get_nickname() calls will produce non-reproducible results.
    /// @return `*this` for chaining.
    nng& unseed()
    {
        _engine.seed(std::random_device{}());
        return *this;
    }

    /// @}

    /// @brief Check whether any word lists have been loaded.
    /// @return `true` if at least one word list is available.
    [[nodiscard]] bool has_wordlists() const
    {
        return !_wordlists.empty();
    }

    /// @brief Load word list files from a directory.
    ///
    /// Recursively scans @p resource_path for `.words` files and indexes them.
    /// Safe to call multiple times to add from different directories.
    ///
    /// @param resource_path Directory containing `.words` files.
    void load(const std::filesystem::path& resource_path)
    {
        if (std::filesystem::exists(resource_path) &&
            std::filesystem::is_directory(resource_path))
        {
            for (const auto& entry :
                 std::filesystem::recursive_directory_iterator(resource_path))
            {
                if (entry.is_regular_file() &&
                    (entry.path().extension() == ".words"))
                {
                    parse_file(entry);
                }
            }
        };
    }

  private:
    // Container of words.
    using word_container = std::vector<std::wstring>;

    // Function pointer type for nickname transform methods.
    using generator_fn = std::wstring (*)(const std::wstring&,
                                          effolkronium::random_local&);

    // Container of methods used to modify the nickname.
    using generators = std::vector<generator_fn>;

    // Contains all vowel characters.
    // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
    static const inline std::wstring _vowels{
        L"aeiouáàâãäåæçèéêëìíîïðñòóôõöøšùúûüýÿ"};

    // Maps letters to their leet-speak digit equivalents.
    // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
    static const inline std::map<wchar_t, wchar_t> _leet_map{
        {L'o', L'0'}, {L'O', L'0'}, {L'i', L'1'}, {L'I', L'1'}, {L's', L'2'},
        {L'S', L'2'}, {L'e', L'3'}, {L'E', L'3'}, {L'a', L'4'}, {L'A', L'4'},
        {L'g', L'6'}, {L'G', L'6'}, {L't', L'7'}, {L'T', L'7'}, {L'b', L'8'},
        {L'B', L'8'}, {L'q', L'9'}, {L'Q', L'9'}};

    // Vector for randomly accessing wordlists.
    std::vector<word_container> _wordlists;

    // Per-instance random engine for seed drawing.
    std::mt19937_64 _engine{std::random_device{}()};

    // Tag type for the auto-probing singleton constructor.
    struct auto_probe_tag {};

    // Singleton constructor: auto-probes common resource locations.
    explicit nng(auto_probe_tag /*tag*/)
    {
        for (const auto& candidate : {"resources", "../resources",
                                       "nickname-generator/resources"})
        {
            const std::filesystem::path p{candidate};
            if (std::filesystem::exists(p) && std::filesystem::is_directory(p))
            {
                load(p);
                break;
            }
        }
    }

    // Add an x to either the front or back of the nickname - or both.
    static std::wstring xfy(const std::wstring& nickname,
                             effolkronium::random_local& engine)
    {
        // Xfied nickname.
        std::wstring x_nickname{nickname};

        // Distribution of possible xy, yx, xyx probability.
        switch (engine.get(0, 2))
        {
        case 0:
            x_nickname.push_back(L'X');
            break;
        case 1:
            x_nickname.insert(0, 1, L'X');
            break;
        default: // case 2
            x_nickname.push_back(L'X');
            x_nickname.insert(0, 1, L'X');
            break;
        }

        return x_nickname;
    }

    // Writes the nickname backwards such as emankcin.
    static std::wstring reverse(
        const std::wstring& nickname,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Original nickname reversed.
        std::wstring reverse_nickname{nickname};

        std::ranges::reverse(reverse_nickname);

        return reverse_nickname;
    }

    // Adds an y to the end of nickname or replace the last character if it's a
    // vowel.
    static std::wstring yfy(
        const std::wstring& nickname,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Nickname with an y a the end.
        std::wstring nicknamy{nickname};

        if (_vowels.contains(nickname.back()))
        {
            nicknamy.back() = L'y';
        }
        else
        {
            nicknamy.push_back(L'y');
        }

        return nicknamy;
    }

    // Adds a number to the end of nickname.
    static std::wstring numify(const std::wstring& nickname,
                                effolkronium::random_local& engine)
    {
        // Nickname with a number at the end.
        std::wstring nickname_with_number{nickname};

        // Maximum single digit value for numified nicknames.
        static constexpr int max_digit{9};

        // Append a random digit character (1-9).
        nickname_with_number.push_back(
            L'0' + engine.get(1, max_digit));

        // Append n zeroes to the end of the nickname.
        for (auto i = engine.get(0, 3); i > 0;
             i--)
        {
            nickname_with_number.push_back(L'0');
        }

        return nickname_with_number;
    }

    // Adds a trace to the end of the nickname.
    static std::wstring tracefy(
        const std::wstring& nickname,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        return nickname + L"-";
    }

    // Adds an ing to the end of nickname or replace the last character if it's
    // a vowel.
    static std::wstring ingify(
        const std::wstring& nickname,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Nickname with an ing a the end.
        std::wstring nicknaming{nickname};

        if (_vowels.contains(nickname.back()))
        {
            nicknaming.back() = L'i';
        }
        else
        {
            nicknaming.push_back(L'i');
        }

        nicknaming.append(L"ng");

        return nicknaming;
    }

    // Finds an aeio vowel and duplicates it, returns same nickname if no
    // available vowel.
    static std::wstring duovowel(
        const std::wstring& nickname,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        static const std::wstring simple_vowels = L"aeio";

        // Nickname with duplicated vowel.
        std::wstring nicknamee{nickname};

        // Try duplicating each vowel in the nickname, stop after succeeding
        // once.
        for (const auto& vowel : simple_vowels)
        {
            if (auto it = std::ranges::find(nicknamee, vowel);
                it != nicknamee.end())
            {
                nicknamee.insert(it, *it);
                break;
            }
        }

        return nicknamee;
    }

    // Replaces a letter by a numerical character.
    static std::wstring oneleet(const std::wstring& nickname,
                                 effolkronium::random_local& engine)
    {
        // Nickname with leetified letter.
        std::wstring leet_nickname{nickname};

        // Collect indices of leet-mappable characters.
        std::vector<std::size_t> candidate_positions;

        for (std::size_t i = 0; i < leet_nickname.size(); ++i)
        {
            if (_leet_map.contains(leet_nickname[i]))
            {
                candidate_positions.push_back(i);
            }
        }

        // Pick one position at random and replace it.
        if (!candidate_positions.empty())
        {
            auto pos = *engine.get(candidate_positions);
            leet_nickname[pos] = _leet_map.at(leet_nickname[pos]);
        }

        return leet_nickname;
    }

    // Replaces as much letters as possible in the nickname by numerical
    // characters.
    static std::wstring allleet(
        const std::wstring& nickname,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Nickname leetified.
        std::wstring leet_nickname{nickname};

        // Replace all possible characters.
        for (auto& character : leet_nickname)
        {
            if (_leet_map.contains(character))
            {
                character = _leet_map.at(character);
            }
        }

        return leet_nickname;
    }

    // Slightly modify the nickname to add some flavor.
    // NOLINTNEXTLINE(misc-no-recursion)
    static nickname leetify(nickname nickname,
                             effolkronium::random_local& engine,
                             bool force = false, int depth = 0)
    {
        // Maximum recursion depth to prevent stack overflow on unmappable
        // inputs.
        static constexpr int max_depth{7};

        // We have 1/2 chance of leetifying, force parameter overrides this.
        if (force || engine.get<bool>())
        {
            // When leetifying, there's 1/2 chance of using a finalizer or a
            // random leetifier.
            if (engine.get<bool>())
            {
                // Possible methods utilized to leetify the nickname.
                static const generators possible_generators{
                    reverse,  // emanckin
                    duovowel, // nicknamee
                    oneleet,  // n1ckname
                    allleet   // n1ckn4m3
                };

                // Capture pre-transform value to detect no-ops.
                auto before = nickname._internal_string;

                // New leetified nickname.
                nickname._internal_string =
                    (*engine.get(
                        possible_generators))(nickname, engine);

                // If the new nickname didn't change and we haven't exceeded
                // the depth limit, force leetify again.
                if (depth < max_depth)
                {
                    return leetify(nickname, engine,
                                   nickname._internal_string == before,
                                   depth + 1);
                }
            }
            else
            {
                // Possible methods utilized to leetify the nickname.
                static const generators possible_generators{
                    xfy,     // nicknameX
                    reverse, // emanckin
                    yfy,     // nicknamy
                    numify,  // nickname2000
                    tracefy, // nickname-
                    ingify,  // nicknaming
                };

                // New leetified nickname.
                nickname._internal_string =
                    (*engine.get(possible_generators))(
                        nickname, engine);
            }
        }

        return nickname;
    }

    // Returns the nickname with an underscore separating its original parts.
    static std::wstring snake_case(const std::wstring& name)
    {
        std::wstring result;
        result.reserve(name.size() + 4);

        for (auto&& [i, ch] : name | std::views::enumerate)
        {
            if (i > 0 && iswupper(ch) != 0)
            {
                result.push_back(L'_');
            }
            result.push_back(ch);
        }

        return result;
    }

    // Returns the nickname in all uppercase.
    static std::wstring upper_case(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Name in all upper case characters.
        std::wstring upper_name{name};

        // Transform every character to uppercase if possible.
        std::ranges::for_each(
            upper_name,
            [](wchar_t& character) { character = std::towupper(character); });

        return upper_name;
    }

    // Returns the nickname in all lowercase.
    static std::wstring lower_case(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Name in all lower case characters.
        std::wstring lower_name{name};

        // Transform every character to lower if possible.
        std::ranges::for_each(
            lower_name,
            [](wchar_t& character) { character = std::towlower(character); });

        return lower_name;
    }

    // Returns the nickname in title case.
    static std::wstring title_case(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Well, titlecase is actually the default.
        return name;
    }

    // Returns the nickname in sentence case.
    static std::wstring sentence_case(const std::wstring& name,
                                       effolkronium::random_local& engine)
    {
        if (name.empty())
        {
            return name;
        }

        // Sentence case formatted nickname.
        std::wstring sentence_name{lower_case(name, engine)};

        // Transform the first character to upper case.
        sentence_name.at(0) = std::towupper(sentence_name.at(0));

        return sentence_name;
    }

    // Returns the nickname in camel case.
    static std::wstring camel_case(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        if (name.empty())
        {
            return name;
        }

        // Camel case formatted nickname.
        std::wstring camel_name{name};

        // Transform the first character to lower case.
        camel_name.at(0) = std::towlower(camel_name.at(0));

        return camel_name;
    }

    // Returns the nickname in reverse sentence case.
    static std::wstring reverse_sentence_case(const std::wstring& name,
                                               effolkronium::random_local& engine)
    {
        if (name.empty())
        {
            return name;
        }

        // Reverse sentence case formatted nickname.
        std::wstring rsentence_name{lower_case(name, engine)};

        // Transform the last character to upper case.
        rsentence_name.back() = std::towupper(rsentence_name.back());

        return rsentence_name;
    }

    // Returns the nickname in bathtub case.
    static std::wstring bathtub_case(const std::wstring& name,
                                      effolkronium::random_local& engine)
    {
        if (name.empty())
        {
            return name;
        }

        // Bathtub case formatted nickname.
        std::wstring bathtub_name{lower_case(name, engine)};

        // Transform the first character to upper case.
        bathtub_name.at(0) = std::towupper(bathtub_name.at(0));

        // Transform the last character to upper case.
        bathtub_name.back() = std::towupper(bathtub_name.back());

        return bathtub_name;
    }

    // Returns the nickname in winding case.
    static std::wstring winding_case(const std::wstring& name,
                                      effolkronium::random_local& engine)
    {
        // Winding case formatted nickname.
        std::wstring winding_name{lower_case(name, engine)};

        // Transform to uppercase every other character.
        for (auto&& [i, ch] : winding_name | std::views::enumerate)
        {
            if ((i % 2) == 0)
            {
                ch = std::towupper(ch);
            }
        }

        return winding_name;
    }

    // Returns the nickname with case in a random fashion.
    static std::wstring random_case(const std::wstring& name,
                                     effolkronium::random_local& engine)
    {
        // Name in all lower case characters.
        std::wstring random_name{lower_case(name, engine)};

        // Randomly uppercase each character.
        std::ranges::for_each(random_name, [&engine](wchar_t& character) {
            if (engine.get<bool>())
            {
                character = std::towupper(character);
            }
        });

        return random_name;
    }

    // Returns the nickname all lower case with a single random character
    // uppercase.
    static std::wstring random_single_case(const std::wstring& name,
                                            effolkronium::random_local& engine)
    {
        // Name in all lower case characters.
        std::wstring random_name{lower_case(name, engine)};

        // Position of single random character to be uppercased.
        auto random_char{engine.get(random_name)};

        *random_char = std::towupper(*random_char);

        return random_name;
    }

    // Format nickname utilizing one of the possible cases.
    static nickname format(nickname nickname,
                            effolkronium::random_local& engine)
    {
        // 1% chance of snake case. nick_name
        // Probability of applying snake_case formatting.
        static constexpr double snake_case_probability{0.01};

        if (engine.get<bool>(snake_case_probability))
        {
            nickname._internal_string = snake_case(nickname);
        }

        // Possible methods utilized to format the nickname.
        // Repeat functions to enforce a distribution.
        static const generators possible_generators = {
            upper_case,
            upper_case,
            upper_case,
            upper_case, // NICKNAME
            lower_case,
            lower_case,
            lower_case,
            lower_case,
            lower_case,
            lower_case,
            lower_case,
            lower_case, // nickname
            title_case,
            title_case, // NickName
            sentence_case,
            sentence_case,
            sentence_case,
            sentence_case,
            sentence_case, // Nickname
            camel_case,
            camel_case, // nickName
            reverse_sentence_case,
            reverse_sentence_case, // nicknamE
            bathtub_case,
            bathtub_case,
            bathtub_case,      // NicknamE
            winding_case,      // nIcKnAmE
            random_case,       // niCKnaMe
            random_single_case // nicknaMe
        };

        nickname._internal_string = (*engine.get(
            possible_generators))(nickname, engine);

        return nickname;
    }

    // Split a full name into a vector containing each name/surname.
    // Empty parts (from consecutive spaces) are filtered out.
    [[nodiscard]] static std::vector<std::wstring> split_name(
        const std::wstring& name)
    {
        std::vector<std::wstring> parts;

        for (auto part : name | std::views::split(L' '))
        {
            std::wstring s(std::ranges::begin(part),
                           std::ranges::end(part));
            if (!s.empty())
            {
                parts.push_back(std::move(s));
            }
        }

        return parts;
    }

    // Returns the first name.
    static std::wstring first_name(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        return *(split_name(name).cbegin());
    }

    // Returns the last surname.
    static std::wstring last_name(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        return *(split_name(name).crbegin());
    }

    // Returns any name (until it hits a space character).
    static std::wstring any_name(const std::wstring& name,
                                  effolkronium::random_local& engine)
    {
        // Container with names/surnames that compose the received name.
        std::vector<std::wstring> names_list{split_name(name)};

        return *engine.get(names_list);
    }

    // Returns only the name initials.
    static std::wstring initials(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Generated nickname containing each name first letter.
        std::wstring nickname;

        // Retrieve first letter from each part of the name.
        for (const auto& part : split_name(name))
        {
            nickname.push_back(part.front());
        }

        return nickname;
    }

    // Mix the last two names.
    static std::wstring mix_two(const std::wstring& name,
                                 effolkronium::random_local& engine)
    {
        // Generated nickname containing a part the last two names.
        std::wstring nickname;

        // Container with names/surnames that compose the received name.
        auto names_list{split_name(name)};

        // Reduce name list to two names.
        if (names_list.size() > 2)
        {
            names_list.erase(names_list.begin(),
                             names_list.end() - 2);
        }

        // Iterate through each name retrieving random number of letters.
        for (const auto& name : names_list)
        {
            if (name.size() < 2)
            {
                nickname.append(name);
            }
            else
            {
                nickname.append(name.substr(
                    0, engine.get<std::size_t>(
                           2, name.size())));
            }
        }

        return nickname;
    }

    // Mix first name with last name initial.
    static std::wstring first_plus_initial(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Container with names/surnames that compose the received name.
        const std::vector<std::wstring> names_list{split_name(name)};

        if (names_list.size() < 2)
        {
            return names_list.front();
        }

        return names_list.front() + names_list.back().front();
    }

    // Mix last name with first name initial.
    static std::wstring initial_plus_last(
        const std::wstring& name,
        [[maybe_unused]] effolkronium::random_local& engine)
    {
        // Container with names/surnames that compose the received name.
        const auto names_list{split_name(name)};

        if (names_list.size() < 2)
        {
            return names_list.front();
        }

        return names_list.front().front() + names_list.back();
    }

    // Reduce a random part of the name.
    static std::wstring reduce_single_name(const std::wstring& name,
                                            effolkronium::random_local& engine)
    {
        // Random part of name.
        std::wstring single_name{any_name(name, engine)};

        if (single_name.size() > 3)
        {
            // Remove all vowel characters from the name unless it's already
            // small enough.
            single_name.erase(
                std::remove_if(single_name.begin() + 1, single_name.end() - 1,
                               [&single_name](const wchar_t& character) {
                                   return (_vowels.contains(character) &&
                                           (single_name.size() > 3));
                               }),
                single_name.end() - 1);
        }

        return single_name;
    }

    // Contains logic to generate a random nickname optionally based on the
    // player full name.
    [[nodiscard]] nickname solver(const std::wstring& name,
                                   effolkronium::random_local& engine) const
    {
        // Holds the original word used to generate the nickname.
        std::wstring original;

        // Holds the modified version.
        std::wstring nick;

        // 1/4 chance of nickname being name related.
        static constexpr double name_related_probability{0.25};

        // When name is provided, use name-based generation with
        // name_related_probability — but always use the name if there are
        // no wordlists to fall back to.
        const bool use_name =
            !name.empty() &&
            (_wordlists.empty() ||
             engine.get<bool>(name_related_probability));

        // Proceed to generate nickname based on name.
        if (use_name)
        {
            // Possible methods utilized to generate a nickname.
            // Purposefully adds redundancy to first and last name with any name
            // to add double weight to them.
            static const generators possible_generators{
                first_name,         // John
                last_name,          // Doe
                any_name,           // Smith
                initials,           // JSD
                mix_two,            // DoSmi
                initial_plus_last,  // JSmith
                first_plus_initial, // JohnS
                reduce_single_name  // Jhn
            };

            original = name;

            // Return a nickname from one of the name based possibilities.
            nick = (*engine.get(
                possible_generators))(name, engine);
        }
        // Proceed to generate nickname based on a word list.
        else if (!_wordlists.empty())
        {
            // Randomly select a wordlist.
            const word_container& drawn_wordlist =
                *engine.get(_wordlists);

            // Randomly selects a word from the wordlist.
            nick = original =
                *engine.get(drawn_wordlist);
        }
        else
        {
            throw(std::invalid_argument(
                "Received no name and word lists are empty"));
        }

        return format(leetify({nick, original}, engine), engine);
    }

    // Try parsing the wordlist file and index it into our container.
    void parse_file(const std::filesystem::path& file)
    {
        // Expected wordlist file format is content type string, list of words.
        std::wifstream tentative_file{file};

        // If managed to open the file proceed.
        if (tentative_file.is_open())
        {
            // Expected delimiter character.
            const wchar_t delimiter{'\n'};

            // Line being read from the file.
            std::wstring file_line;

            // List of parsed words.
            word_container words_read{std::vector<std::wstring>()};

            // Retrieves list of words.
            while (std::getline(tentative_file, file_line, delimiter))
            {
                // Strip trailing carriage return for cross-platform
                // compatibility.
                if (!file_line.empty() && file_line.back() == L'\r')
                {
                    file_line.pop_back();
                }

                // Skip empty lines (including blank trailing lines).
                if (!file_line.empty())
                {
                    words_read.push_back(file_line);
                }
            }

            // Index our container.
            if (!words_read.empty())
            {
                _wordlists.push_back(std::move(words_read));
            }
        }
    }

    friend struct ::nng_test_access;
};
} // namespace dasmig

#endif // DASMIG_NICKNAMEGEN_HPP
