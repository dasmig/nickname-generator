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

    // Deterministic generation — same seed always produces the same nickname.
    std::wcout << L"\n=== Seeded (deterministic) ===" << std::endl;
    auto nick = gen.get_nickname(L"Alberto Bins Elis", 42);
    std::wcout << L"seed 42 -> " << nick << std::endl;

    // Replay: retrieve the seed from a nickname and replay it.
    auto replay = gen.get_nickname(L"Alberto Bins Elis", nick.seed());
    std::wcout << L"replay  -> " << replay << std::endl;

    // Seed the engine for a deterministic sequence.
    gen.seed(100);
    std::wcout << L"\n=== Seeded sequence ===" << std::endl;
    for (int i = 0; i < 3; ++i)
    {
        std::wcout << gen.get_nickname(L"Sequence Test") << std::endl;
    }
    gen.unseed(); // restore non-deterministic state

    // Independent instance with its own word lists and engine.
    std::wcout << L"\n=== Independent instance ===" << std::endl;
    dasmig::nng my_gen;
    my_gen.load("resources");
    for (int i = 0; i < 5; ++i)
    {
        std::wcout << my_gen.get_nickname(L"Own Instance") << std::endl;
    }

    return 0;
}