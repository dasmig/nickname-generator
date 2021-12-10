#include <iostream>
#include "../dasmig/nicknamegen.hpp"

int main()
{
    for (std::size_t i = 0; i < 1000; i++)
        std::wcout << dasmig::nng::instance().get_nickname(L"Alberto Bins Elis") << std::endl;

    return 0;
}