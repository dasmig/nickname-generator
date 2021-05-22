#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <filesystem>
#include <fstream>
#include <functional>

// Written by Diego Dasso Migotto - diegomigotto at hotmail dot com
namespace dasmig
{
    // The nickname generator generates as realistic as possible gamers nicknames 
    // akin to professional players, allowing requests specifying the player name.
    class nng
    {
        public:
            // Use to index the wordlists by content.
            enum class content
            {
                general,
                gamer,
                any
            };

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
            // Typedef to avoid type horror when defining a pointer to a container of names.
            typedef std::shared_ptr<std::vector<std::wstring>> word_container;

            // Default folder to look for wordlists resources. 
            static const inline std::filesystem::path _default_resources_path = "resources";

            // Maps for accessing wordlists through type.
            std::map<content, word_container> _content_indexed_words;

            // Initialize random generator, no complicated processes.
            nng() { load(_default_resources_path); };

            // We don't manage any resource, all should gracefully deallocate by itself.
            ~nng() {};

            // Slightly modify the nickname to add some flavor.
            static std::wstring leetify(const std::wstring& nickname) { return nickname; };

            // Split a full name into a vector containing each name/surname.
            static std::vector<std::wstring> split_name(const std::wstring& name) 
            {  
                // Container with names/surnames split by space character.
                std::vector<std::wstring> splitted_name;

                // String to ease the name splitting.
                std::wstringstream full_name_stream(name);

                // Single part of the full name.
                std::wstring single_name = L"";

                while (std::getline(full_name_stream, single_name, L' ' ))
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

                // Proceed to generate nickname based on name.
                if (default_distribution(random_device) == 0)
                {
                    // Possible methods utilized to generate a nickname.
                    // Purposefully adds redundancy to first and last name with any name to add double weight to them.
                    std::vector<std::function<std::wstring(const std::wstring&)>> possible_generators =
                    {
                        first_name,
                        last_name,
                        any_name,
                        initials,
                        mix_two,
                        initial_plus_last,
                        first_plus_initial,
                        reduce_single_name
                    };
                
                    // Possible choices of name based nickname algorithm.
                    std::uniform_int_distribution<std::size_t> name_algorithm_distribution(0, possible_generators.size() - 1);

                    // Return a nickname from one of the possibilities, possibly leetified in some way.
                    return leetify((possible_generators.at(name_algorithm_distribution(random_device)))(name));
                }
                // Proceed to generate nickname based on a word list. 
                else
                {
                    // Distribution of possible wordlist content values.  
                    std::uniform_int_distribution<std::size_t> content_range(0, static_cast<std::size_t>(content::any) - 1);

                    // Randomly select a content.
                    content random_content = static_cast<content>(content_range(random_device));

                    return L"";
                }
            };

            // Translates possible wordlists types to content enum.
            static content to_content(const std::wstring& type_string) 
            {
                static const std::map<std::wstring, content> content_map = {
                    { L"general", content::general },
                    { L"gamer", content::gamer }
                };

                return (content_map.find(type_string) != content_map.end()) ? content_map.at(type_string) : content::any; 
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

                    // Content type read from file header.
                    content content_read = content::any;

                    // List of parsed words.
                    word_container words_read = std::make_shared<std::vector<std::wstring>>();

                    // Retrieves content from file being read.
                    if (std::getline(tentative_file, file_line, delimiter))
                    {
                        content_read = to_content(file_line);
                    }

                    // We can't continue without a valid content type.
                    if (content_read != content::any)
                    {                            
                        // Retrieves list of words.
                        while (std::getline(tentative_file, file_line, delimiter))
                        {
                            words_read->push_back(file_line);
                        }

                        // Index our container.
                        _content_indexed_words.emplace(content_read, words_read);
                    }
                }
            }

    };

}