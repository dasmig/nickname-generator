# Usage Guide

This guide covers every feature of the nickname-generator library in detail. For a quick overview, see the [README](../README.md). For the full API reference, run `doxygen Doxyfile` from the repository root and open `doc/api/html/index.html`.

[TOC]

<!-- GitHub-rendered TOC (Doxygen uses [TOC] above instead) -->
<ul>
<li><a href="#quick-start">Quick Start</a></li>
<li><a href="#installation">Installation</a></li>
<li><a href="#loading-resources">Loading Resources</a></li>
<li><a href="#generating-nicknames">Generating Nicknames</a></li>
<li><a href="#nickname-transforms">Nickname Transforms</a></li>
<li><a href="#word-lists">Word Lists</a></li>
<li><a href="#thread-safety">Thread Safety</a></li>
<li><a href="#seeding-and-deterministic-generation">Seeding and Deterministic Generation</a></li>
<li><a href="#multi-instance-support">Multi-Instance Support</a></li>
<li><a href="#extending-data">Extending Data</a></li>
<li><a href="#replay-and-json">Replay and JSON</a></li>
<li><a href="#compiled-in-data">Compiled-in Data</a></li>
<li><a href="#request-api-migration">Request API Migration</a></li>
<li><a href="#migrating-from-v1">Migrating from v1</a></li>
<li><a href="#error-reference">Error Reference</a></li>
</ul>

## Quick Start

```cpp
#include <dasmig/nicknamegen.hpp>
#include <iostream>

int main()
{
    auto& gen = dasmig::nng::instance();

    // Random nickname from word lists.
    std::wcout << gen.get_nickname({}) << L"\n";

    // Nickname based on a player name.
    std::wcout << gen.get_nickname({.name = L"John Smith"}) << L"\n";
}
```

## Installation

1. Copy `dasmig/nicknamegen.hpp`, `dasmig/core.hpp` and the `dasmig/core/` directory into your include path, keeping them side by side.
2. Copy the `resources/` folder (`lite/` with the `.words` files) so it is accessible at runtime, or compile the lists in (see Compiled-in Data).
3. Compile with C++23 enabled: `-std=c++23`.

## Loading Resources

The library ships one tier, `resources/lite/`; `dataset::full` loads the same lists.

On first access the singleton (`nng::instance()`) loads it. It uses the compiled-in copy when built with `DASMIG_NICKNAME_EMBED`, and otherwise looks for `lite/animals.words` in these directories, relative to the working directory:

1. `resources`
2. `../resources`
3. `nickname-generator/resources`
4. `../nickname-generator/resources`

`load(dataset::lite)` searches the same way and returns `false` when it finds nothing. If your word lists are elsewhere, load the directory explicitly:

```cpp
dasmig::nng gen;
(void)gen.load(dasmig::dataset::lite);   // shipped lists
gen.load("/opt/data/wordlists");         // or a directory of your own
```

`load(path)` reads every `.words` and `.tsv` file below the directory, in path order, and **replaces** all lists and layers loaded before. A path that is not a directory is ignored; a malformed file throws `dasmig::core::tsv_error` and leaves the lists unchanged. To add to what is loaded, use `extend()` (see Extending Data).

## Generating Nicknames

Both generation overloads require a `dasmig::nickname_request`:
`get_nickname(request)` uses the instance's normal random engine, and
`get_nickname(request, seed)` is deterministic and `const`.

| Request field | Type | Default | Meaning |
|---------------|------|---------|---------|
| `name` | `std::wstring` | Empty | Optional full name; empty or only spaces selects from word lists |

The request owns its name and can be reused. Word lists, resource paths and
data layers are instance setup, not request fields. Decoration and case
probabilities retain the existing model described below.

### Random (word list based)

```cpp
auto nick = dasmig::nng::instance().get_nickname({});
std::wstring text = nick;  // implicit conversion
```

### Name-based

When a name is provided there is a 25 % chance the nickname will be derived
from it (using initials, mixing name parts, etc.). Otherwise a random word
from the loaded lists is used. Without word lists, the name is always used.

```cpp
const dasmig::nickname_request request{.name = L"Alberto Bins Elis"};
auto nick = dasmig::nng::instance().get_nickname(request);
```

### Accessing the original

```cpp
auto nick = dasmig::nng::instance().get_nickname({.name = L"Jane Doe"});
std::wcout << nick.plain();   // "Jane Doe" or the source word
std::wcout << nick;           // the transformed nickname
```

## Nickname Transforms

Each generated nickname passes through two stages:

The exact probabilities are in [Data and Method](data.md#generation-model).

### Leetify stage

With probability one half the nickname is decorated: half of the time with one of reverse, duovowel, oneleet or allleet (retried while the text does not change), otherwise with one of reverse, xfy, yfy, numify, tracefy or ingify.

| Transform | Example |
|-----------|---------|
| reverse | `emankcin` |
| duovowel | `nicknamee` |
| oneleet | `n1ckname` |
| allleet | `n1cknam3` |
| xfy | `XnicknameX` |
| yfy | `nicknamy` |
| numify | `nickname2000` |
| tracefy | `nickname-` |
| ingify | `nicknaming` |

### Format stage

A case format is randomly selected with weighted distribution:

| Format | Weight | Example |
|--------|--------|---------|
| lower_case | 8 | `nickname` |
| sentence_case | 5 | `Nickname` |
| upper_case | 4 | `NICKNAME` |
| bathtub_case | 3 | `NicknamE` |
| title_case (kept as is) | 2 | `NickName` |
| camel_case (first letter lower) | 2 | `nickName` |
| reverse_sentence_case | 2 | `nicknamE` |
| winding_case | 1 | `nIcKnAmE` |
| random_case | 1 | `niCKnaMe` |
| random_single_case | 1 | `nicknaMe` |

There is also a 1 % chance of `snake_case` being applied first (`Nick_Name`).

## Word Lists

The library ships three word lists in `resources/lite/` (see [Data and Method](data.md)):

| File | Contents |
|------|----------|
| `adjectives.words` | English adjectives |
| `animals.words` | Animal names |
| `japanese.words` | Japanese-themed words |

A `.words` file has one word per line; the list's name is the file name.
Lists of your own are added with `extend()` (see Extending Data).
A list is chosen uniformly, then a word from it, so a short list's words come
up more often than a long list's.

## Thread Safety

Each `nng` instance is independent. The static `instance()` singleton uses a
local static for safe initialization.

| Operation | Thread-safe? |
|-----------|-------------|
| `instance()` | Yes (static local) |
| `get_nickname(request)` on **different** instances | Yes |
| `get_nickname(request)` on the **same** instance | **No** — requires external synchronization |
| `get_nickname(request, seed)` on the **same** instance | Yes, if word lists are not changed concurrently |
| `load()`, `extend()` | **No** — must not be called concurrently with `get_nickname()` on the same instance |

Load and extend once during initialization before spawning threads. For
concurrent generation, give each thread its own `nng` instance; the seeded
overload `get_nickname(request, seed)` is `const` and does not touch the engine.

## Seeding and Deterministic Generation

### Per-call seeding

Pass an explicit seed to `get_nickname()` to produce the same nickname every time:

```cpp
auto nick = dasmig::nng::instance().get_nickname({.name = L"John Smith"}, 42);
// The same request, seed and word lists give the same nickname on every
// compiler, standard library and CPU.
```

### Seed replay

Every nickname records its seed. Retrieve it with `nickname::seed()` and pass
it back with the same request and word lists to reproduce the exact same result:

```cpp
const dasmig::nickname_request request{.name = L"Jane Doe"};
auto nick = dasmig::nng::instance().get_nickname(request);
auto saved_seed = nick.seed();

// Later, replay:
auto replay = dasmig::nng::instance().get_nickname(request, saved_seed);
// replay == nick
```

### Deterministic sequences

Supply a seed for each call to produce a reproducible sequence. There are no
generator-level seeding controls:

```cpp
auto& gen = dasmig::nng::instance();

auto a = gen.get_nickname({.name = L"Alice"}, 100);
auto b = gen.get_nickname({.name = L"Bob"}, 101);

auto a2 = gen.get_nickname({.name = L"Alice"}, 100); // identical to a
auto b2 = gen.get_nickname({.name = L"Bob"}, 101);   // identical to b
```

Explicitly seeded calls do not advance the instance's random engine. Calls
without a seed continue normal random generation.

### Checking resources

Use `has_wordlists()` before generating to verify resources loaded, and
`list_names()` to see which lists are in use:

```cpp
if (!dasmig::nng::instance().has_wordlists())
{
    dasmig::nng::instance().load("/path/to/resources");
}
```

## Multi-Instance Support

In addition to the singleton `instance()`, you can construct independent `nng`
objects. Each instance owns its own word lists and random engine, so they
operate without shared state — ideal for embedding inside other generators or
running multiple configurations side by side.

```cpp
// Create an independent generator.
dasmig::nng gen;

// Load a custom word list directory.
gen.load("path/to/my-words");

// Generate nicknames — independent from instance().
std::wcout << gen.get_nickname({.name = L"Player One"}) << L"\n";
```

### Per-call Seeding

An explicit seed affects only that call, not another instance or later calls:

```cpp
dasmig::nng a;
dasmig::nng b;
a.load("resources");
b.load("resources");

std::wcout << a.get_nickname({.name = L"Test"}, 42) << L"\n";
std::wcout << b.get_nickname({.name = L"Test"}, 99) << L"\n";
```

### Move Semantics

Instances can be moved (but not copied), which lets you store them in
containers or transfer ownership:

```cpp
dasmig::nng src;
src.load("resources");

dasmig::nng dst = std::move(src);
// dst now owns the word lists and engine; src is in a moved-from state.
```

## Extending Data

The shipped lists are a base layer. Your own files are loaded on top of them to add lists, add words or remove words, without editing the shipped files. A list's words are its keys.

```cpp
auto& gen = dasmig::nng::instance();

// A .words file adds its words to the list named after the file:
// mods/animals.words adds animals, mods/planets.words adds a new list.
gen.extend("mods/animals.words");

// A directory: every .words and .tsv file below it, applied together.
gen.extend("mods/");

// From TSV text with a `word` column, for example supplied by a
// WebAssembly host or a game scenario.
gen.extend_from_string("planets", "word\nMercury\nVenus\nMars\n", "scenario: planets");

// Remove words with a `disabled` column; a list left with no words is not used.
gen.extend_from_string("animals", "word\tdisabled\nCockroach\t1\n", "cleanup");

// Add mode refuses words that are already in the list.
gen.extend("mods/japanese.words", dasmig::core::layer_mode::add);
```

- A layer is checked before anything changes. A bad layer throws `dasmig::core::tsv_error` with the file, line and column, and the lists stay as they were; a directory is applied all or nothing.
- `layers("animals")` lists what is loaded for one list: source, mode, words added, replaced and removed, and warnings such as unknown columns.
- A file that is neither `.words` nor `.tsv` is refused with `std::invalid_argument`.
- `load(...)` replaces the base and drops every layer.

## Replay and JSON

Every result records the seed and the data it came from:

```cpp
const dasmig::nickname_request request{.name = L"Alberto Bins Elis"};
auto n = gen.get_nickname(request);
n.seed();                    // the call seed
n.signature().data;          // gen.data_hash() when it was generated

// Same seed, same request, same lists: the same nickname everywhere.
auto again = gen.get_nickname(request, n.seed());

if (n.signature().data != gen.data_hash())
{
    // Different lists or layers are loaded now; the replay would differ.
}
```

`to_json()` gives the result as JSON (UTF-8), the form the WebAssembly layer and the game consume:

```json
{"nickname":"SILE","source":"Alberto Bins Elis","signature":{"seed":"000000000000002a","data":"87dd5622316ce1fe"}}
```

Seeds are strings because JavaScript numbers cannot hold every 64-bit value. `to_json(writer)` writes into an existing `dasmig::core::json_writer`, for composing larger documents.

## Compiled-in Data

Define `DASMIG_NICKNAME_EMBED` before including the header to compile the word lists into the program (about 50 KB). `load(dataset::lite)` and `instance()` then use them and need no files, which is what a WebAssembly build wants. The compiled-in lists have the same `data_hash()` as the files, so seeds replay the same either way.

```cpp
#define DASMIG_NICKNAME_EMBED
#include <dasmig/nicknamegen.hpp>
```

`dasmig/nicknamegen_data.hpp` is generated from `resources/lite` by `make embed`; CI checks it is up to date.

## Request API Migration

Generation is now request-first, with only `get_nickname(request)` and
`get_nickname(request, seed)`. Replace no-argument calls with
`get_nickname({})`, and string arguments with
`get_nickname({.name = L"Player Name"})` (or a reusable `nickname_request`).
For deterministic calls, the seed remains the second argument.

Generator-level `seed()` and `unseed()` are removed. To generate deterministic
sequences, pass an explicit seed on each call; to resume normal random
generation, omit the seed. `nickname::seed()` remains available for replay.
The request, loaded word lists and explicit seed reproduce the previous
per-call seeded results unchanged. `load()`, `extend()` and resource queries
remain instance APIs.

## Migrating from v1

Version 2 moves the library onto [dasmig-core](https://github.com/dasmig/dasmig-core).

- **Every generated nickname changes.** The engine (xoshiro256\*\*) and the random choices are now the core's own, so a seed gives the same nickname on every compiler, standard library and CPU. Seeds saved with v1 do not reproduce their v1 results. The generation model is unchanged (checked against v1 over 20,000 draws per scenario).
- **Case changes do not depend on the locale.** v1 used `std::towupper` and `std::towlower`, so whether accented letters changed case depended on the C locale and the platform. v2 maps Basic Latin, Latin-1 Supplement and Latin Extended-A itself; other letters keep their case.
- **Word lists.** The files moved to `resources/lite/`, and repeated words were removed (38 in `animals`, 4 in `japanese`), so those words are no longer two or three times as likely. Lists are used in name order, not in the order the filesystem returns them.
- **`load(path)` replaces** the lists instead of adding to them; use `extend()` to add. A malformed file now throws `dasmig::core::tsv_error`. `load(dataset)` is new.
- **Files:** replace `random.hpp` with `core.hpp` and the `core/` directory (see Installation).
- **New:** `extend()`, `extend_from_string()`, `layers()`, `list_names()`, `data_hash()`, `nickname::signature()`, `nickname::to_json()`, `DASMIG_NICKNAME_EMBED`, `.tsv` word lists.

## Batch Generation

Every seeded getter has batch forms: `get_nickname_batch` return a `std::vector`, and `for_each_nickname` streams the items to a sink in index order without holding the batch.

```cpp
auto& gen = dasmig::nng::instance();
const auto request = dasmig::nickname_request{.name = L"Alberto Bins Elis"};

// 1,000 results from one batch seed, on four threads.
auto items = gen.get_nickname_batch(request, {.count = 1000, .threads = 4}, 42);

// Item 1 replays on its own: the batch seed and the index give its seed.
auto again = gen.get_nickname(request, dasmig::core::item_seed(42, 1));

// A million results to a file as JSON Lines, never all in memory.
std::ofstream out{"items.jsonl"};
dasmig::core::jsonl_writer lines{out};
gen.for_each_nickname(request, {.count = 1'000'000, .threads = 0}, 42, lines);
```

- **Item seeds.** Item `i` is exactly `get_nickname(request, core::item_seed(batch_seed, first + i))`. A range can be generated directly (`first = 1000` starts at item 1000), two batches that follow each other equal one, and the output is the same for any `threads` (0 means the hardware count). Without a `batch_seed` one is drawn like the single getter draws its own.
- **Options.** `core::batch_options` has `count`, `first`, `threads` and `unique`. The request describes one item; the options describe the batch.
- **Uniqueness.** `.unique = core::unique_rule::by<nickname>(key)` regenerates an item whose key already appeared in the batch, with a deterministic retry seed, and throws `std::runtime_error` naming the item when the attempts run out.
- **Sinks.** `core::jsonl_writer` writes one `to_json()` per line, `core::csv_writer` flattens each result's JSON into dotted columns. Any callable `(std::uint64_t index, nickname&)` works.
- Batch calls use the same const getters from several threads, which the thread-safety rules above allow; the CI runs the batch tests under ThreadSanitizer.

## Error Reference

| Exception | Thrown by | Condition |
|-----------|-----------|----------|
| `std::invalid_argument` | `nng::get_nickname()` | Request name is empty or only spaces **and** no word lists loaded |
| `std::invalid_argument` | `nng::extend(path)` | A file that is neither `.words` nor `.tsv` |
| `std::out_of_range` | `nng::layers(list)` | No list with that name |
| `dasmig::core::tsv_error` | `load()`, `extend()`, `extend_from_string()` | A malformed file or layer (message gives file, line and column); the lists are unchanged |
