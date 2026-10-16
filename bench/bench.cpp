// Benchmarks for nickname-generator; see bench.hpp for the options.

#include "../dasmig/nicknamegen.hpp"
#include "bench.hpp"

int main(int argc, char** argv)
{
    dasmig::bench::runner run{"nickname-generator", argc, argv};

    run.load("load", [&] {
        dasmig::nng gen;
        return gen.load(run.tier());
    });

    dasmig::nng gen;
    if (!gen.load(run.tier())) return run.missing_tier();

    run.calls("get_nickname from words", [&](std::uint64_t seed) {
        return gen.get_nickname({}, seed);
    });
    run.calls("get_nickname from a name", [&](std::uint64_t seed) {
        return gen.get_nickname({.name = L"Alberto Bins Elis"}, seed);
    });
    run.calls("get_nickname to_json", [&](std::uint64_t seed) {
        return gen.get_nickname({.name = L"Alberto Bins Elis"}, seed).to_json();
    });

    return run.finish();
}
