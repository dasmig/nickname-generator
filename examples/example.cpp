#include <iostream>
#include "../dasmig/nicknamegen.hpp"

int main()
{
    auto& gen = dasmig::nng::instance();

    // Generate nicknames based on a player name.
    std::wcout << L"=== Name-based nicknames ===" << std::endl;
    for (int i = 0; i < 10; ++i)
    {
        auto nick = gen.get_nickname(L"Alberto Bins Elis");
        std::wcout << nick.plain() << L" -> " << nick << std::endl;
    }

    // Generate random nicknames from word lists.
    std::wcout << L"\n=== Random nicknames ===" << std::endl;
    for (int i = 0; i < 10; ++i)
    {
        auto nick = gen.get_nickname();
        std::wcout << nick.plain() << L" -> " << nick << std::endl;
    }

    // Load additional word lists from a custom path.
    // gen.load("path/to/custom/wordlists");

    // Generate using single names and multi-part names.
    std::wcout << L"\n=== Various name styles ===" << std::endl;
    std::wcout << gen.get_nickname(L"Mononym") << std::endl;
    std::wcout << gen.get_nickname(L"First Last") << std::endl;
    std::wcout << gen.get_nickname(L"First Middle Last Suffix") << std::endl;
    std::wcout << gen.get_nickname(L"José García") << std::endl;

    return 0;
}