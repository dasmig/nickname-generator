#include "random.hpp"
#include <atomic>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// Written by Diego Dasso Migotto - diegomigotto at hotmail dot com
namespace dasmig
{

// Internal class used by nickname generator to allow for chained calls when
// building a nickname.
class nickname
{
  public:
    // Return original word used as source for the nickname, or name that
    // originated it.
    [[nodiscard]] std::wstring plain() const
    {
        return _original_string;
    }

    // Operator wstring to allow for implicit conversion to string.
    operator std::wstring() const // NOLINT(hicpp-explicit-conversions)
    {
        return _internal_string;
    }

    // Operator ostream streaming internal string.
    friend std::wostream& operator<<(std::wostream& wos,
                                     const nickname& nickname)
    {
        wos << nickname._internal_string;
        return wos;
    }

  private:
    // Private constructor, this is mostly a helper class to the nickname
    // generator, not the intended API.
    nickname(std::wstring nickname_str, std::wstring original_str)
        : _internal_string(std::move(nickname_str)),
          _original_string(std::move(original_str))
    {
    }

    // Current nickname after all modifications/formatting.
    std::wstring _internal_string;

    // Original string used to generated the nickname.
    std::wstring _original_string;

    // Allows nickname generator to construct nicknames.
    friend class nng;
};

// The nickname generator generates as realistic as possible gamers nicknames
// akin to professional players, allowing requests specifying the player name.
class nng
{
  public:
    // Copy/move constructors can be deleted since they are not going to be
    // used due to singleton pattern.
    nng(const nng&) = delete;
    nng(nng&&) = delete;
    nng& operator=(const nng&) = delete;
    nng& operator=(nng&&) = delete;

    // Thread safe access to nickname generator singleton.
    static nng& instance()
    {
        static nng instance;
        return instance;
    }

    // Generates a nickname based on requested name(optional).
    [[nodiscard]] nickname get_nickname(const std::wstring& name = L"") const
    {
        return solver(name);
    };

    // Try loading every possible wordlists file from the received resource
    // path.
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

    // Container of methods used to modify the nickname.
    using generators =
        std::vector<std::function<std::wstring(const std::wstring&)>>;

    // Default folder to look for wordlists resources.
    static const inline std::filesystem::path _default_resources_path{
        R"(C:\Repos\nickname-generator\resources\)"};

    // Contains all vowel characters.
    static const inline std::wstring _vowels{
        L"aeiouáàâãäåæçèéêëìíîïðñòóôõöøšùúûüýÿ"};

    // Contains all vowel characters.
    static const inline std::map<wchar_t, wchar_t> _leet_map{
        {L'o', L'0'}, {L'O', L'0'}, {L'i', L'1'}, {L'I', L'1'}, {L's', L'2'},
        {L'S', L'2'}, {L'e', L'3'}, {L'E', L'3'}, {L'a', L'4'}, {L'A', L'4'},
        {L'g', L'6'}, {L'G', L'6'}, {L't', L'7'}, {L'T', L'7'}, {L'b', L'8'},
        {L'B', L'8'}, {L'q', L'9'}, {L'Q', L'9'}};

    // Vector for randomly accessing wordlists.
    std::vector<word_container> _wordlists;

    // Initialize random generator, no complicated processes.
    nng()
    {
        load(_default_resources_path);
    };

    // We don't manage any resource, all should gracefully deallocate by itself.
    ~nng() = default;

    // Add an x to either the front or back of the nickname - or both.
    static std::wstring xfy(const std::wstring& nickname)
    {
        // Xfied nickname.
        std::wstring x_nickname{nickname};

        // Distribution of possible xy, yx, xyx probability.
        switch (effolkronium::random_thread_local::get(0, 2))
        {
        case 0:
            x_nickname.push_back(L'X');
            break;
        case 1:
            x_nickname.insert(0, 1, L'X');
            break;
        case 2:
            x_nickname.push_back(L'X');
            x_nickname.insert(0, 1, L'X');
            break;
        default:
            break;
        }

        return x_nickname;
    }

    // Writes the nickname backwards such as emankcin.
    static std::wstring reverse(const std::wstring& nickname)
    {
        // Original nickname reversed.
        std::wstring reverse_nickname{nickname};

        std::reverse(std::begin(reverse_nickname), std::end(reverse_nickname));

        return reverse_nickname;
    }

    // Adds an y to the end of nickname or replace the last character if it's a
    // vowel.
    static std::wstring yfy(const std::wstring& nickname)
    {
        // Nickname with an y a the end.
        std::wstring nicknamy{nickname};

        if (std::find(std::cbegin(_vowels), std::cend(_vowels),
                      nickname.back()) != std::end(_vowels))
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
    static std::wstring numify(const std::wstring& nickname)
    {
        // Nickname with an y a the end.
        std::wstring nickname_with_number{nickname};

        // Utilized to randomize digit added to the end of the nickname.
        nickname_with_number.push_back(
            effolkronium::random_thread_local::get(1, 9));

        // Append n zeroes to the end of the nickname.
        for (std::size_t i{0}; i < effolkronium::random_thread_local::get(0, 3);
             i++)
        {
            nickname_with_number.push_back('0');
        }

        return nickname_with_number;
    }

    // Adds a trace to the end of the nickname.
    static std::wstring tracefy(const std::wstring& nickname)
    {
        return nickname + L"-";
    }

    // Adds an ing to the end of nickname or replace the last character if it's
    // a vowel.
    static std::wstring ingify(const std::wstring& nickname)
    {
        // Nickname with an ing a the end.
        std::wstring nicknaming{nickname};

        if (std::find(std::cbegin(_vowels), std::cend(_vowels),
                      nickname.back()) != std::end(_vowels))
        {
            nicknaming.back() = L'i';
            nicknaming.push_back(L'n');
            nicknaming.push_back(L'g');
        }
        else
        {
            nicknaming.push_back(L'i');
            nicknaming.push_back(L'n');
            nicknaming.push_back(L'g');
        }

        return nicknaming;
    }

    // Finds an aeio vowel and duplicates it, returns same nickname if no
    // available vowel.
    static std::wstring duovowel(const std::wstring& nickname)
    {
        static const std::wstring simple_vowels = L"aeio";

        // Nickname with duplicated vowel.
        std::wstring nicknamee{nickname};

        // Try duplicating each vowel in the nickname, stop after succeeding
        // once.
        for (const auto& vowel : simple_vowels)
        {
            // Iterator pointing to vowel finded in the nickname.
            const auto vowel_iterator =
                std::find(std::cbegin(nicknamee), std::cend(nicknamee), vowel);

            if (vowel_iterator != std::cend(nicknamee))
            {
                nicknamee.insert(vowel_iterator, *vowel_iterator);
                break;
            }
        }

        return nicknamee;
    }

    // Replaces a letter by a numerical character.
    static std::wstring oneleet(const std::wstring& nickname)
    {
        // Nickname with leetified letter.
        std::wstring leet_nickname{nickname};

        // Candidates letter to be leetified.
        std::list<wchar_t> candidates{};

        // Retrieve candidates for replacement.
        for (auto& character : leet_nickname)
        {
            if (_leet_map.contains(character))
            {
                candidates.push_back(character);
            }
        }

        return leet_nickname;
    }

    // Replaces as much letters as possible in the nickname by numerical
    // characters.
    static std::wstring allleet(const std::wstring& nickname)
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
    static nickname leetify(nickname nickname, bool force = false)
    {
        // We have 1/2 chance of leetifying, force parameter overrides this.
        if (force || effolkronium::random_thread_local::get<bool>())
        {
            // When leetifying, there's 1/2 chance of using a finalizer or a
            // random leetifier.
            if (effolkronium::random_thread_local::get<bool>())
            {
                // Possible methods utilized to leetify the nickname.
                static const generators possible_generators{
                    reverse,  // emanckin
                    duovowel, // nicknamee
                    oneleet,  // n1ckname
                    allleet   // n1ckn4m3
                };

                // New leetified nickname.
                nickname._internal_string =
                    (*effolkronium::random_thread_local::get(
                        possible_generators))(nickname);

                // If the new nickname didn't suffer any alteration, force
                // leetify again.
                return leetify(nickname, nickname._internal_string ==
                                             nickname._original_string);
            }

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
                (*effolkronium::random_thread_local::get(possible_generators))(
                    nickname);
        }

        return nickname;
    };

    // Returns the nickname with an underscore separating its original parts.
    static std::wstring snake_case(const std::wstring& name)
    {
        // Name containing underscore.
        std::wstring snakefied_name{name};

        // Introduce an underscore if it's the begginning of a part of the
        // nickname, except the first.
        for (std::size_t i = 1; i < snakefied_name.size(); i++)
        {
            if (iswupper(snakefied_name.at(i)) != 0)
            {
                snakefied_name.insert(i, L"_");
                i++;
            }
        }

        return snakefied_name;
    };

    // Returns the nickname in all uppercase.
    static std::wstring upper_case(const std::wstring& name)
    {
        // Name in all upper case characters.
        std::wstring upper_name{name};

        // Transform every character to uppercase if possible.
        std::for_each(
            std::begin(upper_name), std::end(upper_name),
            [](wchar_t& character) { character = std::towupper(character); });

        return upper_name;
    };

    // Returns the nickname in all lowercase.
    static std::wstring lower_case(const std::wstring& name)
    {
        // Name in all lower case characters.
        std::wstring lower_name{name};

        // Transform every character to lower if possible.
        std::for_each(
            std::begin(lower_name), std::end(lower_name),
            [](wchar_t& character) { character = std::towlower(character); });

        return lower_name;
    };

    // Returns the nickname in title case.
    static std::wstring title_case(const std::wstring& name)
    {
        // Well, titlecase is actually the default.
        return name;
    };

    // Returns the nickname in sentence case.
    static std::wstring sentence_case(const std::wstring& name)
    {
        // Sentence case formatted nickname.
        std::wstring sentence_name{lower_case(name)};

        // Transform the first character to upper case.
        sentence_name.at(0) = std::towupper(sentence_name.at(0));

        return sentence_name;
    };

    // Returns the nickname in camel case.
    static std::wstring camel_case(const std::wstring& name)
    {
        // Camel case formatted nickname.
        std::wstring camel_name{name};

        // Transform the first character to lower case.
        camel_name.at(0) = std::towlower(camel_name.at(0));

        return camel_name;
    };

    // Returns the nickname in reverse sentence case.
    static std::wstring reverse_sentence_case(const std::wstring& name)
    {
        // Reverse sentence case formatted nickname.
        std::wstring rsentence_name{lower_case(name)};

        // Transform the last character to upper case.
        rsentence_name.back() = std::towupper(rsentence_name.back());

        return rsentence_name;
    };

    // Returns the nickname in bathtub case.
    static std::wstring bathtub_case(const std::wstring& name)
    {
        // Bathtub case formatted nickname.
        std::wstring bathtub_name{lower_case(name)};

        // Transform the first character to upper case.
        bathtub_name.at(0) = std::towupper(bathtub_name.at(0));

        // Transform the last character to upper case.
        bathtub_name.back() = std::towupper(bathtub_name.back());

        return bathtub_name;
    };

    // Returns the nickname in winding case.
    static std::wstring winding_case(const std::wstring& name)
    {
        // Winding case formatted nickname.
        std::wstring winding_name{lower_case(name)};

        // Transform to uppercase half of the letters.
        for (std::size_t i = 0; i < winding_name.size(); i++)
        {
            if ((i % 2) == 0)
            {
                winding_name.at(i) = std::towupper(winding_name.at(i));
            }
        }

        return winding_name;
    };

    // Returns the nickname with case in a random fashion.
    static std::wstring random_case(const std::wstring& name)
    {
        // Name in all lower case characters.
        std::wstring random_name{lower_case(name)};

        // Transform every character to lower if possible.
        std::for_each(std::begin(random_name), std::end(random_name),
                      [&](wchar_t& character) {
                          if (effolkronium::random_thread_local::get<bool>())
                          {
                              character = std::towupper(character);
                          }
                      });

        return random_name;
    };

    // Returns the nickname all lower case with a single random character
    // uppercase.
    static std::wstring random_single_case(const std::wstring& name)
    {
        // Name in all lower case characters.
        std::wstring random_name{lower_case(name)};

        // Position of single random character to be uppercased.
        auto random_char{effolkronium::random_thread_local::get(random_name)};

        *random_char = std::towupper(*random_char);

        return random_name;
    };

    // Format nickname utilizing one of the possible cases.
    static nickname format(nickname nickname)
    {
        // 1% chance of snake case. nick_name
        if (effolkronium::random_thread_local::get<bool>(0.01))
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

        nickname._internal_string = (*effolkronium::random_thread_local::get(
            possible_generators))(nickname);

        return nickname;
    };

    // Split a full name into a vector containing each name/surname.
    static std::vector<std::wstring> split_name(const std::wstring& name)
    {
        // Container with names/surnames split by space character.
        std::vector<std::wstring> splitted_name;

        // String to ease the name splitting.
        std::wstringstream full_name_stream{name};

        // Single part of the full name.
        std::wstring single_name;

        while (std::getline(full_name_stream, single_name, L' '))
        {
            splitted_name.push_back(single_name);
        }

        return splitted_name;
    };

    // Returns the first name.
    static std::wstring first_name(const std::wstring& name)
    {
        return *(split_name(name).cbegin());
    };

    // Returns the last surname.
    static std::wstring last_name(const std::wstring& name)
    {
        return *(split_name(name).crbegin());
    };

    // Returns any name (until it hits a space character).
    static std::wstring any_name(const std::wstring& name)
    {
        // Container with names/surnames that compose the received name.
        std::vector<std::wstring> names_list{split_name(name)};

        return *effolkronium::random_thread_local::get(names_list);
    };

    // Returns only the name initials.
    static std::wstring initials(const std::wstring& name)
    {
        // Generated nickname containing each name first letter.
        std::wstring nickname;

        // Container with names/surnames that compose the received name.
        std::vector<std::wstring> names_list{split_name(name)};

        // Iterate through each name retrieving first letter.
        for (const auto& name : names_list)
        {
            nickname.push_back(*(name.cbegin()));
        }

        return nickname;
    };

    // Mix the last two names.
    static std::wstring mix_two(const std::wstring& name)
    {
        // Generated nickname containing a part the last two names.
        std::wstring nickname;

        // Container with names/surnames that compose the received name.
        auto names_list{split_name(name)};

        // Reduce name list to two names.
        while (names_list.size() > 2)
        {
            names_list.erase(names_list.begin());
        }

        // Iterate through each name retrieving random number of letters.
        for (const auto& name : names_list)
        {
            nickname.append(name.substr(
                0, effolkronium::random_thread_local::get<std::size_t>(
                       2, name.size())));
        }

        return nickname;
    };

    // Mix first name with last name initial.
    static std::wstring first_plus_initial(const std::wstring& name)
    {
        // Container with names/surnames that compose the received name.
        std::vector<std::wstring> names_list{split_name(name)};

        return *(names_list.cbegin()) + names_list.crbegin()->front();
    }

    // Mix last name with first name initial.
    static std::wstring initial_plus_last(const std::wstring& name)
    {
        // Container with names/surnames that compose the received name.
        auto names_list{split_name(name)};

        return names_list.cbegin()->front() + *(names_list.crbegin());
    }

    // Reduce a random part of the name.
    static std::wstring reduce_single_name(const std::wstring& name)
    {
        // Random part of name.
        std::wstring single_name{any_name(name)};

        if (single_name.size() > 3)
        {
            // Remove all vowel characters from the name unless it's already
            // small enough.
            single_name.erase(
                std::remove_if(single_name.begin() + 1, single_name.end() - 1,
                               [&single_name](const wchar_t& character) {
                                   return ((_vowels.find(character) !=
                                            std::wstring::npos) &&
                                           (single_name.size() > 3));
                               }),
                single_name.end() - 1);
        }

        return single_name;
    }

    // Contains logic to generate a random nickname optionally based on the
    // player full name.
    [[nodiscard]] nickname solver(const std::wstring& name) const
    {
        // Holds the original word used to generate the nickname.
        std::wstring original;

        // Holds the modified version.
        std::wstring nick;

        // 1/4 chance of nickname being name related.
        const std::double_t name_related_probability{0.25};

        // Proceed to generate nickname based on name.
        if (!name.empty() && effolkronium::random_thread_local::get<bool>(
                                 name_related_probability))
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
            nick = (*effolkronium::random_thread_local::get(
                possible_generators))(name);
        }
        // Proceed to generate nickname based on a word list.
        else if (!_wordlists.empty())
        {
            // Randomly select a worldist.
            word_container drawn_wordlist =
                *effolkronium::random_thread_local::get(_wordlists);

            // Randomly selects a word from the wordlist.
            nick = original =
                *effolkronium::random_thread_local::get(drawn_wordlist);
        }
        else
        {
            throw(std::invalid_argument(
                "Received no name and word lists are empty"));
        }

        return format(leetify({nick, original}));
    };

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
                words_read.push_back(file_line);
            }

            // Index our container.
            _wordlists.push_back(words_read);
        }
    }
};
} // namespace dasmig