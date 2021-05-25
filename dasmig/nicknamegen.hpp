#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <filesystem>
#include <fstream>
#include <functional>
#include <cwctype>

// Written by Diego Dasso Migotto - diegomigotto at hotmail dot com
namespace dasmig
{
    // The nickname generator generates as realistic as possible gamers nicknames 
    // akin to professional players, allowing requests specifying the player name.
    class nng
    {
        public:
            // Thread safe access to nickname generator singleton.
            static nng& instance() { static nng instance; return instance; }

            // Generates a nickname based on requested name(optional).
            std::wstring get_nickname(std::wstring name = L"") const { return solver(name); };   

            // Try loading every possible wordlists file from the received resource path.
            void load(const std::filesystem::path& resource_path) 
            { 
                for (auto& f : std::filesystem::recursive_directory_iterator(resource_path))
                    if (f.is_regular_file() && (f.path().extension() == ".words"))
                        parse_file(f);
            };

        private:
            // Typedef to avoid type horror when defining a container of names.
            typedef std::vector<std::wstring> word_container;

            // Default folder to look for wordlists resources. 
            static const inline std::filesystem::path _default_resources_path = "resources";

            // Vector for randomly accessing wordlists.
            std::vector<word_container> _wordlists;

            // Initialize random generator, no complicated processes.
            nng() { load(_default_resources_path); };

            // We don't manage any resource, all should gracefully deallocate by itself.
            ~nng() {};

            // Slightly modify the nickname to add some flavor.
            static std::wstring leetify(const std::wstring& nickname) 
            {
                // Utilized to randomize leetify probability content.
                std::random_device random_device;
                
                // Distribution of possible leetifying probability (50%).
                std::uniform_int_distribution<std::size_t> leetify_distribution(0, 1);

                // Leetify
                // nickname2000
                // 100nicknames
                // nicknameX
                // n1ckname
                // n1ckn4m3
                // nicknamee
                // nicknaming
                // nicknamy
                // nikname
                // nickname-
                // colornickname
                // adjectivenickname
                // emankcin

                // We have 1/2 chance of keeping leetifying after a leetification.
                if (leetify_distribution(random_device))
                {
                    return leetify(nickname);
                }
                else
                {
                    return nickname;
                }
            };

            // Returns the nickname with an underscore separating its original parts.
            static std::wstring snake_case(const std::wstring& name) 
            { 
                // Name containing underscore.
                std::wstring snakefied_name = name;

                // Introduce an underscore if it's the begginning of a part of the nickname, except the first.
                for (std::size_t i = 1; i < snakefied_name.size(); i++)
                {
                    if (iswupper(snakefied_name.at(i)))
                    {
                        snakefied_name.insert(i, L"_");
                        i++;
                    }
                }

                return *(split_name(name).cbegin());
            };
                        
            // Returns the nickname in all uppercase.
            static std::wstring upper_case(const std::wstring& name) 
            { 
                // Name in all upper case characters.
                std::wstring upper_name = name;

                // Transform every character to uppercase if possible.
                std::for_each(std::begin(upper_name), std::end(upper_name), [](wchar_t& character)
                {
                    character = towupper(character);
                });

                return upper_name;
            };
                        
            // Returns the nickname in all lowercase.
            static std::wstring lower_case(const std::wstring& name) 
            { 
                // Name in all lower case characters.
                std::wstring lower_name = name;

                // Transform every character to lower if possible.
                std::for_each(std::begin(lower_name), std::end(lower_name), [](wchar_t& character)
                {
                    character = towlower(character);
                });

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
                std::wstring sentence_name = lower_case(name);

                // Transform the first character to upper case.
                sentence_name.at(0) = towupper(sentence_name.at(0));

                return sentence_name;
            };

            // Returns the nickname in camel case.
            static std::wstring camel_case(const std::wstring& name) 
            { 
                // Camel case formatted nickname.
                std::wstring camel_name = name;

                // Transform the first character to lower case.
                camel_name.at(0) = towlower(camel_name.at(0));

                return camel_name;
            };

            // Returns the nickname in reverse sentence case.
            static std::wstring reverse_sentence_case(const std::wstring& name) 
            { 
                // Reverse sentence case formatted nickname.
                std::wstring rsentence_name = lower_case(name);

                // Transform the last character to upper case.
                rsentence_name.at(rsentence_name.size() - 1) = towupper(rsentence_name.at(rsentence_name.size() - 1));

                return rsentence_name;
            };

            // Returns the nickname in bathtub case.
            static std::wstring bathtub_case(const std::wstring& name) 
            { 
                // Bathtub case formatted nickname.
                std::wstring bathtub_name = lower_case(name);
                
                // Transform the first character to upper case.
                bathtub_name.at(0) = towupper(bathtub_name.at(0));

                // Transform the last character to upper case.
                bathtub_name.at(bathtub_name.size() - 1) = towupper(bathtub_name.at(bathtub_name.size() - 1));

                return bathtub_name;
            };

            // Returns the nickname in winding case.
            static std::wstring winding_case(const std::wstring& name) 
            { 
                // Winding case formatted nickname.
                std::wstring winding_name = lower_case(name);

                // Transform to uppercase half of the letters.
                for (std::size_t i = 0; i < winding_name.size(); i++)
                {
                    if (!(i % 2))
                    {
                        winding_name.at(i) = towupper(winding_name.at(i));
                    }
                }

                return winding_name;
            };

            // Format nickname utilizing one of the possible cases.
            static std::wstring format(const std::wstring& nickname) 
            {
                // Utilized to randomize leetify probability content.
                std::random_device random_device;
                
                // Chance of turning into snake case.
                std::uniform_int_distribution<std::size_t> snake_algorithm_distribution(0, 99);

                // Nickname being randomly formatted.
                std::wstring formatted_nickname = nickname;

                // 1% chance of snake case. nick_name
                if (!snake_algorithm_distribution(random_device))
                {
                    formatted_nickname = snake_case(formatted_nickname);
                }

                // Possible methods utilized to format the nickname.
                // Repeat functions to enforce a distribution.
                std::vector<std::function<std::wstring(const std::wstring&)>> possible_generators =
                {
                    upper_case, upper_case, upper_case, upper_case,                                                 // NICKNAME
                    lower_case, lower_case, lower_case, lower_case, lower_case, lower_case, lower_case, lower_case, // nickname
                    title_case, title_case,                                                                         // NickName
                    sentence_case, sentence_case, sentence_case, sentence_case, sentence_case,                      // Nickname
                    camel_case, camel_case,                                                                         // nickName
                    reverse_sentence_case, reverse_sentence_case,                                                   // nicknamE
                    bathtub_case, bathtub_case, bathtub_case,                                                       // NicknamE
                    winding_case,                                                                                   // nIcKnAmE
                    // random_case,                                                                                    // niCKnaMe
                    // random_single_case                                                                              // nicknaMe
                };
                
                // Possible choices of formatting algorithm.
                std::uniform_int_distribution<std::size_t> format_algorithm_distribution(0, possible_generators.size() - 1);

                return possible_generators.at(format_algorithm_distribution(random_device))(formatted_nickname);
            };

            // Split a full name into a vector containing each name/surname.
            static std::vector<std::wstring> split_name(const std::wstring& name) 
            {  
                // Container with names/surnames split by space character.
                std::vector<std::wstring> splitted_name;

                // String to ease the name splitting.
                std::wstringstream full_name_stream(name);

                // Single part of the full name.
                std::wstring single_name = L"";

                while (std::getline(full_name_stream, single_name, L' '))
                {
                    splitted_name.push_back(single_name);
                }
                
                return splitted_name;
            };

            // Returns the first name.
            static std::wstring first_name(const std::wstring& name) { return *(split_name(name).cbegin()); };

            // Returns the last surname.
            static std::wstring last_name(const std::wstring& name) { return *(split_name(name).crbegin()); };
            
            // Returns any name (until it hits a space character).
            static std::wstring any_name(const std::wstring& name) 
            { 
                // Container with names/surnames that compose the received name.
                auto names_list = split_name(name);

                // Utilized to randomize nickname content.
                std::random_device random_device;
                
                // Distribution of possible names.
                std::uniform_int_distribution<std::size_t> default_distribution(0, names_list.size() - 1);

                return names_list.at(default_distribution(random_device)); 
            };

            // Returns only the name initials.
            static std::wstring initials(const std::wstring& name) 
            { 
                // Generated nickname containing each name first letter.
                std::wstring nickname(L"");

                // Container with names/surnames that compose the received name.
                auto names_list = split_name(name);

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
                std::wstring nickname(L"");

                // Container with names/surnames that compose the received name.
                auto names_list = split_name(name);

                // Reduce name list to two names.
                while (names_list.size() > 2)
                {
                    names_list.erase(names_list.begin());
                }

                // Iterate through each name retrieving random number of letters.
                for (const auto& name : names_list)
                {
                    // Utilized to randomize nickname content.
                    std::random_device random_device;
                    
                    // Distribution of possible names.
                    std::uniform_int_distribution<std::size_t> normal_distribution(2, name.size());

                    nickname.append(name.substr(0, normal_distribution(random_device)));
                }

                return nickname;
            };

            // Mix first name with last name initial.
            static std::wstring first_plus_initial(const std::wstring& name) 
            {
                // Container with names/surnames that compose the received name.
                auto names_list = split_name(name);

                return *(names_list.cbegin()) + names_list.crbegin()->front();
            }

            // Mix last name with first name initial.
            static std::wstring initial_plus_last(const std::wstring& name) 
            {
                // Container with names/surnames that compose the received name.
                auto names_list = split_name(name);

                return *(names_list.crbegin()) + names_list.cbegin()->front();
            }

            // Reduce a random part of the name.
            static std::wstring reduce_single_name(const std::wstring& name) 
            {
                // Random part of name.
                std::wstring single_name = any_name(name);
                
                // Contains all vowel characters.
                std::wstring vowels = L"aeiouáàâãäåæçèéêëìíîïðñòóôõöøšùúûüýÿ";

                if (single_name.size() > 3)
                {
                    // Remove all vowel characters from the name unless it's already small enough.
                    std::remove_if(single_name.begin() + 1, single_name.end() - 1,
                        [&vowels, &single_name](const wchar_t& character)
                        { 
                            return (vowels.find(character) && (single_name.size() > 3)); 
                        });
                }
                
                return single_name;
            }

            // Contains logic to generate a random nickname optionally based on the player full name. 
            std::wstring solver(const std::wstring& name) const 
            {
                // Utilized to randomize nickname content.
                std::random_device random_device;
                
                // 1/4 chance of nickname being name related.
                std::uniform_int_distribution<std::size_t> default_distribution(0, 3);

                // Holds the nickname being generated.
                std::wstring nickname = L"";

                // Proceed to generate nickname based on name.
                if (default_distribution(random_device) == 0)
                {
                    // Possible methods utilized to generate a nickname.
                    // Purposefully adds redundancy to first and last name with any name to add double weight to them.
                    std::vector<std::function<std::wstring(const std::wstring&)>> possible_generators =
                    {
                        first_name,             // John
                        last_name,              // Doe
                        any_name,               // Smith
                        initials,               // JSD
                        mix_two,                // DoSmi
                        initial_plus_last,      // JSmith
                        first_plus_initial,     // JohnS
                        reduce_single_name      // Jhn
                    };
                
                    // Possible choices of name based nickname algorithm.
                    std::uniform_int_distribution<std::size_t> name_algorithm_distribution(0, possible_generators.size() - 1);

                    // Return a nickname from one of the name based possibilities.
                    nickname = possible_generators.at(name_algorithm_distribution(random_device))(name);
                }
                // Proceed to generate nickname based on a word list. 
                else
                {
                    // Distribution of possible wordlist.  
                    std::uniform_int_distribution<std::size_t> wordlists_range(0, _wordlists.size() - 1);

                    // Randomly select a worldist.
                    auto drawn_wordlist = _wordlists.at(wordlists_range(random_device));

                    // Distribution of possible words.  
                    std::uniform_int_distribution<std::size_t> drawn_wordlist_range(0, drawn_wordlist.size() - 1);

                    // Randomly selects a word from the worldist.
                    nickname = drawn_wordlist.at(drawn_wordlist_range(random_device));
                }
                
                return format(leetify(nickname));
            };

            // Try parsing the wordlist file and index it into our container.
            void parse_file(const std::filesystem::path& file) 
            {
                // Expected wordlist file format is content type string, list of words.
                std::wifstream tentative_file(file);

                // If managed to open the file proceed.
                if (tentative_file.is_open())
                {
                    // Expected delimiter character.
                    const wchar_t delimiter('\n');

                    // Line being read from the file.
                    std::wstring file_line;

                    // List of parsed words.
                    word_container words_read = std::vector<std::wstring>();

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
}