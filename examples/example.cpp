#include <iostream>
#include "../dasmig/nicknamegen.hpp"

int main()
{
    auto& gen = dasmig::nng::instance();

    // Generate nicknames based on a player name.
    std::wcout << L"=== Name-based nicknames ===\n";
    for (int i = 0; i < 10; ++i)
    {
        auto nick = gen.get_nickname(L"Alberto Bins Elis");
        std::wcout << nick.plain() << L" -> " << nick << L'\n';
    }

    // Generate random nicknames from word lists.
    std::wcout << L"\n=== Random nicknames ===\n";
    for (int i = 0; i < 10; ++i)
    {
        auto nick = gen.get_nickname();
        std::wcout << nick.plain() << L" -> " << nick << L'\n';
    }

    // Generate using single names and multi-part names.
    std::wcout << L"\n=== Various name styles ===\n";
    std::wcout << gen.get_nickname(L"Mononym") << L'\n';
    std::wcout << gen.get_nickname(L"First Last") << L'\n';
    std::wcout << gen.get_nickname(L"First Middle Last Suffix") << L'\n';

    // Deterministic generation: the same seed gives the same nickname on
    // every compiler and platform.
    std::wcout << L"\n=== Seeded (deterministic) ===\n";
    auto nick = gen.get_nickname(L"Alberto Bins Elis", 42);
    std::wcout << L"seed 42 -> " << nick << L'\n';

    // Replay: retrieve the seed from a nickname and replay it.
    auto replay = gen.get_nickname(L"Alberto Bins Elis", nick.seed());
    std::wcout << L"replay  -> " << replay << L'\n';

    // JSON, with the seed and the data it was generated from.
    std::cout << nick.to_json() << '\n';

    // Seed the engine for a deterministic sequence.
    gen.seed(100);
    std::wcout << L"\n=== Seeded sequence ===\n";
    for (int i = 0; i < 3; ++i)
    {
        std::wcout << gen.get_nickname(L"Sequence Test") << L'\n';
    }
    gen.unseed(); // restore non-deterministic state

    // Independent instance with its own word lists and engine, extended
    // with a list of its own.
    std::wcout << L"\n=== Independent instance with an extra list ===\n";
    dasmig::nng my_gen;
    (void)my_gen.load(dasmig::dataset::lite);
    my_gen.extend_from_string("planets", "word\nMercury\nVenus\nMars\nJupiter\n", "planets");
    for (int i = 0; i < 5; ++i)
    {
        auto planet_nick = my_gen.get_nickname();
        std::wcout << planet_nick.plain() << L" -> " << planet_nick << L'\n';
    }

    return 0;
}
