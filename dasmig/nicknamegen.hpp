// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 Diego Dasso Migotto

#ifndef DASMIG_NICKNAMEGEN_HPP
#define DASMIG_NICKNAMEGEN_HPP

#include "core.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifdef DASMIG_NICKNAME_EMBED
#include "nicknamegen_data.hpp"
#endif

/// @file nicknamegen.hpp
/// @brief Nickname generator library — gamer-style nickname generation for C++23.
/// @author Diego Dasso Migotto (diegomigotto at hotmail dot com)
/// @see See doc/usage.md for the narrative tutorial.

DASMIG_CORE_REQUIRE(1, 1);

struct nng_test_access;

namespace dasmig
{

/// @brief Per-call choices for nickname generation.
///
/// Word lists are loaded on nng; deterministic calls take a separate seed.
struct nickname_request
{
    /// @brief Full name to optionally derive the nickname from.
    ///
    /// Empty (or only spaces) selects from the loaded word lists. With a
    /// name, one in four nicknames is derived from it; without word lists,
    /// the name is always used.
    std::wstring name{};

    /// @brief Themes the nickname may be drawn from, as plain strings.
    ///
    /// Typically the tags of a character's traits and job (`"combat"`,
    /// `"leadership"`, `"manual"`, ...), as trait-generator and
    /// occupation-generator give them; no dependency on either. A tag that
    /// has a `tag_<tag>.words` list is a theme, and with at least one theme
    /// one nickname in three is a word of a theme (a theme chosen uniformly,
    /// then a word), unless the name takes the nickname first. Tags without a
    /// list are ignored. The default is no themes.
    std::vector<std::string> tags{};
};

/// @brief Return type for nickname generation, holding both the transformed
///        nickname and the original source string.
///
/// Supports implicit conversion to std::wstring, streaming via operator<<
/// and JSON output via to_json().
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
    /// @see nng::get_nickname(const nickname_request&, std::uint64_t)
    [[nodiscard]] std::uint64_t seed() const
    {
        return _signature.seed;
    }

    /// @brief The seed and the data this nickname was generated from.
    /// @return The replay signature; `data` is nng::data_hash() at the time
    ///         of generation.
    [[nodiscard]] const core::seed_signature& signature() const
    {
        return _signature;
    }

    /// @brief Write this nickname as a JSON object
    ///        `{"nickname":…,"source":…,"signature":{…}}`.
    /// @param w Writer positioned where a value is allowed.
    void to_json(core::json_writer& w) const
    {
        w.begin_object()
            .field("nickname", core::to_utf8(_internal_string))
            .field("source", core::to_utf8(_original_string))
            .key("signature");
        _signature.to_json(w);
        w.end_object();
    }

    /// @brief This nickname as a JSON document.
    /// @return Compact JSON.
    [[nodiscard]] std::string to_json() const
    {
        core::json_writer w;
        to_json(w);
        return w.str();
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
    nickname(std::wstring nickname_str, std::wstring original_str)
        : _internal_string(std::move(nickname_str)),
          _original_string(std::move(original_str))
    {
    }

    std::wstring _internal_string; ///< Current nickname after transforms.
    std::wstring _original_string; ///< Original source string.
    core::seed_signature _signature; ///< Seed and data hash.

    friend class nng; ///< Allows nng to construct nicknames.
};

/// @brief Nickname generator that produces gamer-style nicknames.
///
/// Generates nicknames either from a person's name (first name, initials,
/// mixed parts, ...) or from word lists, then decorates them (leet speak,
/// suffixes, reversal, ...) and formats their case.
///
/// Output is identical on every compiler and platform (dasmig-core): the
/// case changes use a fixed mapping rather than the C locale, and word
/// lists are read as UTF-8 in name order.
///
/// Can be used as a singleton via instance() or constructed independently.
///
/// @par Thread safety
/// Each instance is independent. Concurrent calls to get_nickname(request)
/// on the **same** instance require external synchronisation. Seeded calls
/// are read-only and can run concurrently, provided no word lists are changed.
class nng
{
  public:
    /// @brief Default constructor — creates an empty generator with no word
    ///        lists.
    nng() = default;

    nng(const nng&) = delete;            ///< Not copyable.
    nng& operator=(const nng&) = delete; ///< Not copyable.
    nng(nng&&) noexcept = default;       ///< Move constructor.
    nng& operator=(nng&&) noexcept = default; ///< Move assignment.
    ~nng() = default;                    ///< Default destructor.

    /// @brief Access the global singleton instance.
    ///
    /// Loads the shipped word lists on first access, from compiled-in data
    /// or an auto-probed `resources/`.
    /// @return Reference to the global nng instance.
    static nng& instance()
    {
        static nng inst{auto_probe_tag{}};
        return inst;
    }

    /// @brief Generate a random nickname, optionally based on a name.
    ///
    /// With a name, one in four nicknames is derived from it (always, when
    /// no word lists are loaded); the others come from the word lists.
    ///
    /// @param request Per-call choices; use {} for a word-list nickname.
    /// @return The generated nickname.
    /// @throws std::invalid_argument If request.name has no name parts and
    ///         no word lists are loaded.
    [[nodiscard]] nickname get_nickname(const nickname_request& request)
    {
        return get_nickname(request, _engine());
    }

    /// @brief Generate a deterministic nickname.
    /// @param request Per-call choices; use {} for a word-list nickname.
    /// @param call_seed Seed; the same seed, request and word lists give the
    ///        same nickname.
    /// @return The generated nickname.
    /// @throws std::invalid_argument If request.name has no name parts and
    ///         no word lists are loaded.
    [[nodiscard]] nickname get_nickname(const nickname_request& request,
                                        std::uint64_t call_seed) const
    {
        auto r = core::rng{call_seed}.split("nickname");
        auto result = solver(request, r);
        result._signature = {.seed = call_seed, .data = data_hash()};
        return result;
    }

    // -- Batch generation -------------------------------------------------

    /// @brief Generate a batch of nickname results from a seed drawn like
    ///        get_nickname() draws its own.
    ///
    /// Item `i` is `get_nickname(request, core::item_seed(batch_seed, options.first + i))`
    /// for the drawn batch seed; see the seeded overload.
    ///
    /// @param request What every item is generated from.
    /// @param options Count, first index, threads and uniqueness.
    /// @return The results, in index order.
    [[nodiscard]] std::vector<nickname> get_nickname_batch(const nickname_request& request,
                                                  const core::batch_options& options)
    {
        return get_nickname_batch(request, options, _engine());
    }

    /// @brief Generate a batch of nickname results, deterministically.
    ///
    /// Item `i` is exactly `get_nickname(request, core::item_seed(batch_seed,
    /// options.first + i))`, so an item replays on its own, a range can be
    /// generated directly, two batches that follow each other equal one, and
    /// the output does not depend on `options.threads`.
    ///
    /// @param request What every item is generated from.
    /// @param options Count, first index, threads and uniqueness (see
    ///        core::unique_rule; the rule must be made for `nickname`).
    /// @param batch_seed Seed of the whole batch.
    /// @return The results, in index order.
    /// @throws std::invalid_argument If the request or options are invalid.
    /// @throws std::runtime_error If no data has been loaded, or a unique
    ///         batch runs out of attempts.
    [[nodiscard]] std::vector<nickname> get_nickname_batch(const nickname_request& request,
                                                  const core::batch_options& options,
                                                  std::uint64_t batch_seed) const
    {
        return core::run_batch(options, batch_seed, [&](std::uint64_t item_seed)
                               { return get_nickname(request, item_seed); });
    }

    /// @brief Stream a batch of nickname results to a sink without holding the
    ///        whole batch.
    ///
    /// Generates the same items as get_nickname_batch() and calls
    /// `sink(index, item)` in index order on the calling thread; `index`
    /// includes `options.first`.
    ///
    /// @tparam Sink Callable taking `(std::uint64_t, nickname&)`; for example
    ///         core::jsonl_writer or core::csv_writer.
    /// @param request What every item is generated from.
    /// @param options Count, first index, threads and uniqueness.
    /// @param batch_seed Seed of the whole batch.
    /// @param sink Receives the items.
    template <typename Sink>
    void for_each_nickname(const nickname_request& request, const core::batch_options& options,
                        std::uint64_t batch_seed, Sink&& sink) const
    {
        core::for_each_batch(options, batch_seed,
                             [&](std::uint64_t item_seed) { return get_nickname(request, item_seed); },
                             std::forward<Sink>(sink));
    }

    /// @brief Stream a batch of nickname results with a batch seed drawn like
    ///        get_nickname() draws its own.
    /// @tparam Sink Callable taking `(std::uint64_t, nickname&)`.
    /// @param request What every item is generated from.
    /// @param options Count, first index, threads and uniqueness.
    /// @param sink Receives the items.
    template <typename Sink>
    void for_each_nickname(const nickname_request& request, const core::batch_options& options,
                        Sink&& sink)
    {
        for_each_nickname(request, options, _engine(), std::forward<Sink>(sink));
    }


    /// @brief Check whether any word list is loaded.
    /// @return True when at least one non-empty word list is loaded.
    [[nodiscard]] bool has_wordlists() const
    {
        return !_wordlists.empty();
    }

    /// @brief Load word lists from a directory, replacing all lists and
    ///        layers loaded before.
    ///
    /// Reads every `.words` file (one word per line) and `.tsv` file (a
    /// `word` column) below the directory, in path order. The list name is
    /// the file name without extension; files with the same name extend the
    /// same list. A path that is not a directory is ignored.
    ///
    /// @param resource_path Directory such as `resources/lite`.
    /// @throws core::tsv_error If a file is malformed; the lists are then
    ///         unchanged.
    void load(const std::filesystem::path& resource_path)
    {
        std::error_code ec;
        if (!std::filesystem::is_directory(resource_path, ec)) return;
        apply_(read_directory_(resource_path), core::layer_mode::replace, true);
    }

    /// @brief Load the shipped word lists, replacing all lists and layers
    ///        loaded before.
    ///
    /// The library ships one tier: `dataset::full` loads the same lists as
    /// `dataset::lite`. Uses the compiled-in copy when built with
    /// `DASMIG_NICKNAME_EMBED`, otherwise searches `resources/`,
    /// `../resources`, `nickname-generator/resources` and
    /// `../nickname-generator/resources`.
    ///
    /// @param tier Ignored (one tier).
    /// @return False when no word list was found.
    /// @throws core::tsv_error If a file is malformed.
    [[nodiscard]] bool load([[maybe_unused]] dataset tier)
    {
        std::vector<std::pair<std::string, core::tsv_document>> docs;
        const auto root = core::find_resources(library_name_, std::string{shipped_tier_} + "/" +
                                                                  std::string{marker_list_} + ".words");
        for (const auto list : shipped_lists_)
        {
            const auto path = std::string{shipped_tier_} + "/" + std::string{list} + ".words";
            if (auto doc = core::load_resource(path, embedded_files_(), root, {"word"}))
            {
                docs.emplace_back(std::string{list}, std::move(*doc));
            }
        }
        if (docs.empty()) return false;
        apply_(std::move(docs), core::layer_mode::replace, true);
        return true;
    }

    /// @brief Add word list layers from a directory or a file.
    ///
    /// A directory is read like load() and all its files are applied
    /// together, or none if any is invalid. A `.words` file has one word
    /// per line; a `.tsv` file has a `word` column and may have `disabled`
    /// to remove words. The list name is the file name without extension;
    /// a new name adds a list.
    ///
    /// @param path Directory or file.
    /// @param mode How the layers combine with the words loaded before.
    /// @throws core::tsv_error If a file cannot be read or is invalid; the
    ///         lists are then unchanged.
    /// @throws std::invalid_argument If @p path is a file that is neither
    ///         `.words` nor `.tsv`.
    void extend(const std::filesystem::path& path,
                core::layer_mode mode = core::layer_mode::override)
    {
        std::error_code ec;
        if (std::filesystem::is_directory(path, ec))
        {
            apply_(read_directory_(path), mode, false);
            return;
        }
        auto doc = read_file_(path);
        if (!doc)
        {
            throw std::invalid_argument("extend: expected a .words or .tsv file, got " +
                                        path.filename().string());
        }
        std::vector<std::pair<std::string, core::tsv_document>> docs;
        docs.emplace_back(core::to_utf8(path.stem().wstring()), std::move(*doc));
        apply_(std::move(docs), mode, false);
    }

    /// @brief Add a layer to a word list from TSV text with a `word`
    ///        column (and optionally `disabled`).
    /// @param list Name of the list (a new name adds a list).
    /// @param text TSV text.
    /// @param source Name used in errors.
    /// @param mode How the layer combines with the words loaded before.
    /// @return What the layer did.
    /// @throws core::tsv_error If the text is invalid; the lists are
    ///         unchanged.
    const core::layer_info& extend_from_string(std::string_view list, std::string_view text,
                                               std::string source,
                                               core::layer_mode mode = core::layer_mode::override)
    {
        std::vector<std::pair<std::string, core::tsv_document>> docs;
        docs.emplace_back(std::string{list}, core::tsv_document::parse(text, std::move(source)));
        apply_(std::move(docs), mode, false);
        return _lists.at(std::string{list}).layers().back();
    }

    /// @brief Names of the loaded word lists, in the order they are used.
    /// @return The list names (UTF-8).
    [[nodiscard]] std::vector<std::string> list_names() const
    {
        std::vector<std::string> names;
        for (const auto& [name, table] : _lists)
        {
            if (!table.empty()) names.push_back(name);
        }
        return names;
    }

    /// @brief The layers in effect for one word list.
    /// @param list The list name.
    /// @return The layer records.
    /// @throws std::out_of_range If there is no such list.
    [[nodiscard]] const std::vector<core::layer_info>& layers(const std::string& list) const
    {
        return _lists.at(list).layers();
    }

    /// @brief Hash of the loaded word lists, recorded in each result's
    ///        signature().
    /// @return The hash; 0 when nothing is loaded.
    [[nodiscard]] std::uint64_t data_hash() const
    {
        std::uint64_t h = 0;
        for (const auto& [name, table] : _lists)
        {
            h = core::hash_combine(core::hash_combine(h, core::fnv1a64(name)), table.data_hash());
        }
        return h;
    }

  private:
    static constexpr std::string_view library_name_ = "nickname-generator";
    static constexpr std::string_view shipped_tier_ = "lite";
    static constexpr std::string_view marker_list_ = "animals";
    // Lists named tag_<tag> are themes, drawn only for a request's tags.
    static constexpr std::string_view tag_prefix_ = "tag_";
    static constexpr std::array<std::string_view, 52> shipped_lists_{"adjectives", "animals", "colors", "food", "gems", "japanese", "myth", "nature", "space", "tag_academic", "tag_activity", "tag_art", "tag_care", "tag_caring", "tag_chaos", "tag_combat", "tag_crafting", "tag_creative", "tag_desk", "tag_discipline", "tag_dreaming", "tag_duty", "tag_exploration", "tag_faith", "tag_flight", "tag_haste", "tag_knowledge", "tag_labour", "tag_leadership", "tag_manual", "tag_medicine", "tag_morale", "tag_outdoor", "tag_physical", "tag_planning", "tag_rest", "tag_risk", "tag_routine", "tag_safety", "tag_social", "tag_solitude", "tag_solo", "tag_stealth", "tag_survival", "tag_team", "tag_technical", "tag_trade", "tag_training", "tag_violence", "tag_waiting", "tag_wealth", "tag_willpower"};

    // Function pointer type for nickname transform methods.
    using generator_fn = std::wstring (*)(const std::wstring&, core::rng&);

    // Container of methods used to modify the nickname.
    using generators = std::vector<generator_fn>;

    // Characters treated as vowels (escaped so the source encoding does not
    // matter): aeiou, then á à â ã ä å æ ç è é ê ë ì í î ï ð ñ ò ó ô õ ö ø
    // š ù ú û ü ý ÿ.
    // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
    static const inline std::wstring _vowels{
        L"aeiou\u00E1\u00E0\u00E2\u00E3\u00E4\u00E5\u00E6\u00E7\u00E8\u00E9\u00EA\u00EB"
        L"\u00EC\u00ED\u00EE\u00EF\u00F0\u00F1\u00F2\u00F3\u00F4\u00F5\u00F6\u00F8"
        L"\u0161\u00F9\u00FA\u00FB\u00FC\u00FD\u00FF"};

    // Maps letters to their leet-speak digit equivalents.
    // NOLINTNEXTLINE(bugprone-throwing-static-initialization)
    static const inline std::map<wchar_t, wchar_t> _leet_map{
        {L'o', L'0'}, {L'O', L'0'}, {L'i', L'1'}, {L'I', L'1'}, {L's', L'2'},
        {L'S', L'2'}, {L'e', L'3'}, {L'E', L'3'}, {L'a', L'4'}, {L'A', L'4'},
        {L'g', L'6'}, {L'G', L'6'}, {L't', L'7'}, {L'T', L'7'}, {L'b', L'8'},
        {L'B', L'8'}, {L'q', L'9'}, {L'Q', L'9'}};

    static core::table_schema schema_()
    {
        return {.key = {"word"}, .required = {}, .optional = {}};
    }

    // Word lists by name (sorted, so the order does not depend on the
    // filesystem), and the words in use, rebuilt after every change.
    std::map<std::string, core::layered_table, std::less<>> _lists;
    std::vector<std::vector<std::wstring>> _wordlists;
    std::map<std::string, std::vector<std::wstring>, std::less<>> _themes; // by tag

    core::rng _engine{core::rng::from_entropy()};

    struct auto_probe_tag {};

    explicit nng(auto_probe_tag /*tag*/)
    {
        (void)load(dataset::lite);
    }

    static std::span<const core::embedded_file> embedded_files_()
    {
#ifdef DASMIG_NICKNAME_EMBED
        return embedded::nicknamegen::files;
#else
        return {};
#endif
    }

    // -- Data ---------------------------------------------------------------

    static std::optional<core::tsv_document> read_file_(const std::filesystem::path& file)
    {
        const auto ext = file.extension();
        if (ext == ".words") return core::tsv_document::load(file, {"word"});
        if (ext == ".tsv") return core::tsv_document::load(file);
        return std::nullopt;
    }

    static std::vector<std::pair<std::string, core::tsv_document>> read_directory_(
        const std::filesystem::path& dir)
    {
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
        {
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".words" || entry.path().extension() == ".tsv"))
            {
                files.push_back(entry.path());
            }
        }
        std::ranges::sort(files); // the iteration order is unspecified
        std::vector<std::pair<std::string, core::tsv_document>> docs;
        docs.reserve(files.size());
        for (const auto& file : files)
        {
            docs.emplace_back(core::to_utf8(file.stem().wstring()), *read_file_(file));
        }
        return docs;
    }

    // Apply layers to copies of the lists, then commit: all or nothing.
    // With @p reset (load), lists not in @p docs are dropped.
    void apply_(std::vector<std::pair<std::string, core::tsv_document>> docs, core::layer_mode mode,
                bool reset)
    {
        auto next = reset ? decltype(_lists){} : _lists;
        // On reset each list's first file replaces; later files with the
        // same name extend it.
        std::map<std::string, bool, std::less<>> started;
        for (auto& [name, doc] : docs)
        {
            auto it = next.try_emplace(name, schema_()).first;
            auto list_mode = mode;
            if (reset)
            {
                list_mode = started[name] ? core::layer_mode::override : core::layer_mode::replace;
            }
            started[name] = true;
            (void)it->second.extend(std::move(doc), list_mode, [](const core::tsv_row& row) {
                (void)core::from_utf8(row["word"]);
            });
        }
        std::vector<std::vector<std::wstring>> wordlists;
        std::map<std::string, std::vector<std::wstring>, std::less<>> themes;
        for (const auto& [name, table] : next)
        {
            std::vector<std::wstring> words;
            words.reserve(table.size());
            for (const auto& row : table.rows()) words.push_back(core::from_utf8(row["word"]));
            if (words.empty()) continue;
            if (name.starts_with(tag_prefix_))
            {
                themes.emplace(name.substr(tag_prefix_.size()), std::move(words));
            }
            else
            {
                wordlists.push_back(std::move(words));
            }
        }
        _lists = std::move(next);
        _wordlists = std::move(wordlists);
        _themes = std::move(themes);
    }

    // -- Case mapping -----------------------------------------------------
    //
    // A fixed mapping for Basic Latin, Latin-1 Supplement and Latin
    // Extended-A, so the result does not depend on the C locale. Other
    // characters are left as they are.

    static wchar_t to_upper_(wchar_t c)
    {
        const auto u = static_cast<std::uint32_t>(c);
        if (u >= 0x61U && u <= 0x7AU) return static_cast<wchar_t>(u - 0x20U);
        if (u >= 0xE0U && u <= 0xFEU && u != 0xF7U) return static_cast<wchar_t>(u - 0x20U);
        if (u == 0xFFU) return static_cast<wchar_t>(0x178U);
        if ((u >= 0x100U && u <= 0x12FU) || (u >= 0x132U && u <= 0x137U) ||
            (u >= 0x14AU && u <= 0x177U))
        {
            return static_cast<wchar_t>(u & ~1U); // pairs: upper even, lower odd
        }
        if ((u >= 0x139U && u <= 0x148U) || (u >= 0x179U && u <= 0x17EU))
        {
            return (u % 2U == 0U) ? static_cast<wchar_t>(u - 1U) : c; // upper odd, lower even
        }
        return c;
    }

    static wchar_t to_lower_(wchar_t c)
    {
        const auto u = static_cast<std::uint32_t>(c);
        if (u >= 0x41U && u <= 0x5AU) return static_cast<wchar_t>(u + 0x20U);
        if (u >= 0xC0U && u <= 0xDEU && u != 0xD7U) return static_cast<wchar_t>(u + 0x20U);
        if (u == 0x178U) return static_cast<wchar_t>(0xFFU);
        if ((u >= 0x100U && u <= 0x12FU) || (u >= 0x132U && u <= 0x137U) ||
            (u >= 0x14AU && u <= 0x177U))
        {
            return static_cast<wchar_t>(u | 1U);
        }
        if ((u >= 0x139U && u <= 0x148U) || (u >= 0x179U && u <= 0x17EU))
        {
            return (u % 2U == 1U) ? static_cast<wchar_t>(u + 1U) : c;
        }
        return c;
    }

    static bool is_upper_(wchar_t c) { return to_lower_(c) != c; }

    // -- Decorations --------------------------------------------------------

    // Add an x to either the front or back of the nickname - or both.
    static std::wstring xfy(const std::wstring& nickname, core::rng& engine)
    {
        std::wstring x_nickname{nickname};
        switch (core::uniform_int(engine, 0, 2))
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
    static std::wstring reverse(const std::wstring& nickname,
                                [[maybe_unused]] core::rng& engine)
    {
        std::wstring reverse_nickname{nickname};
        std::ranges::reverse(reverse_nickname);
        return reverse_nickname;
    }

    // Adds an y to the end of nickname or replace the last character if it's a
    // vowel.
    static std::wstring yfy(const std::wstring& nickname,
                            [[maybe_unused]] core::rng& engine)
    {
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
    static std::wstring numify(const std::wstring& nickname, core::rng& engine)
    {
        std::wstring nickname_with_number{nickname};
        static constexpr int max_digit{9};
        nickname_with_number.push_back(static_cast<wchar_t>(L'0' + core::uniform_int(engine, 1, max_digit)));
        for (auto i = core::uniform_int(engine, 0, 3); i > 0; i--)
        {
            nickname_with_number.push_back(L'0');
        }
        return nickname_with_number;
    }

    // Adds a trace to the end of the nickname.
    static std::wstring tracefy(const std::wstring& nickname,
                                [[maybe_unused]] core::rng& engine)
    {
        return nickname + L"-";
    }

    // Adds an ing to the end of nickname or replace the last character if it's
    // a vowel.
    static std::wstring ingify(const std::wstring& nickname,
                               [[maybe_unused]] core::rng& engine)
    {
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
    static std::wstring duovowel(const std::wstring& nickname,
                                 [[maybe_unused]] core::rng& engine)
    {
        static constexpr std::wstring_view simple_vowels = L"aeio";
        std::wstring nicknamee{nickname};
        for (const auto& vowel : simple_vowels)
        {
            if (auto it = std::ranges::find(nicknamee, vowel); it != nicknamee.end())
            {
                nicknamee.insert(it, *it);
                break;
            }
        }
        return nicknamee;
    }

    // Positions of characters that have a leet-speak digit.
    static std::vector<std::size_t> leet_positions_(const std::wstring& text)
    {
        std::vector<std::size_t> positions;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            if (_leet_map.contains(text[i])) positions.push_back(i);
        }
        return positions;
    }

    // Replaces a letter by a numerical character.
    static std::wstring oneleet(const std::wstring& nickname, core::rng& engine)
    {
        std::wstring leet_nickname{nickname};
        const auto candidates = leet_positions_(leet_nickname);
        if (!candidates.empty())
        {
            const auto pos = core::pick(engine, candidates);
            leet_nickname[pos] = _leet_map.at(leet_nickname[pos]);
        }
        return leet_nickname;
    }

    // Replaces up to two random letters in the nickname by their numerical
    // leet-speak equivalents.
    static std::wstring allleet(const std::wstring& nickname, core::rng& engine)
    {
        std::wstring leet_nickname{nickname};
        static constexpr std::size_t max_replacements{2};
        auto candidates = leet_positions_(leet_nickname);
        core::shuffle(engine, candidates);
        for (std::size_t n = 0; n < std::min(max_replacements, candidates.size()); ++n)
        {
            const auto pos = candidates[n];
            leet_nickname[pos] = _leet_map.at(leet_nickname[pos]);
        }
        return leet_nickname;
    }

    // Slightly modify the nickname to add some flavor.
    // NOLINTNEXTLINE(misc-no-recursion)
    static nickname leetify(nickname nickname, core::rng& engine, bool force = false,
                            int depth = 0)
    {
        // Maximum recursion depth to prevent stack overflow on unmappable
        // inputs.
        static constexpr int max_depth{7};
        // We have 1/2 chance of leetifying, force parameter overrides this.
        if (force || core::bernoulli(engine, 0.5))
        {
            // When leetifying, there's 1/2 chance of using a finalizer or a
            // random leetifier.
            if (core::bernoulli(engine, 0.5))
            {
                static const generators possible_generators{
                    reverse,  // emanckin
                    duovowel, // nicknamee
                    oneleet,  // n1ckname
                    allleet   // n1ckn4m3
                };
                auto before = nickname._internal_string;
                nickname._internal_string =
                    core::pick(engine, possible_generators)(nickname, engine);
                // If the new nickname didn't change and we haven't exceeded
                // the depth limit, force leetify again.
                if (depth < max_depth)
                {
                    return leetify(nickname, engine, nickname._internal_string == before,
                                   depth + 1);
                }
            }
            else
            {
                static const generators possible_generators{
                    xfy,     // nicknameX
                    reverse, // emanckin
                    yfy,     // nicknamy
                    numify,  // nickname2000
                    tracefy, // nickname-
                    ingify,  // nicknaming
                };
                nickname._internal_string =
                    core::pick(engine, possible_generators)(nickname, engine);
            }
        }
        return nickname;
    }

    // -- Case formats -------------------------------------------------------

    // Returns the nickname with an underscore separating its original parts.
    static std::wstring snake_case(const std::wstring& name)
    {
        std::wstring result;
        result.reserve(name.size() + 4);
        for (std::size_t i = 0; i < name.size(); ++i)
        {
            if (i > 0 && is_upper_(name[i])) result.push_back(L'_');
            result.push_back(name[i]);
        }
        return result;
    }

    // Returns the nickname in all uppercase.
    static std::wstring upper_case(const std::wstring& name,
                                   [[maybe_unused]] core::rng& engine)
    {
        std::wstring upper_name{name};
        std::ranges::for_each(upper_name, [](wchar_t& character) { character = to_upper_(character); });
        return upper_name;
    }

    // Returns the nickname in all lowercase.
    static std::wstring lower_case(const std::wstring& name,
                                   [[maybe_unused]] core::rng& engine)
    {
        std::wstring lower_name{name};
        std::ranges::for_each(lower_name, [](wchar_t& character) { character = to_lower_(character); });
        return lower_name;
    }

    // Returns the nickname in title case.
    static std::wstring title_case(const std::wstring& name,
                                   [[maybe_unused]] core::rng& engine)
    {
        // Well, titlecase is actually the default.
        return name;
    }

    // Returns the nickname in sentence case.
    static std::wstring sentence_case(const std::wstring& name, core::rng& engine)
    {
        if (name.empty()) return name;
        std::wstring sentence_name{lower_case(name, engine)};
        sentence_name.at(0) = to_upper_(sentence_name.at(0));
        return sentence_name;
    }

    // Returns the nickname in camel case.
    static std::wstring camel_case(const std::wstring& name,
                                   [[maybe_unused]] core::rng& engine)
    {
        if (name.empty()) return name;
        std::wstring camel_name{name};
        camel_name.at(0) = to_lower_(camel_name.at(0));
        return camel_name;
    }

    // Returns the nickname in reverse sentence case.
    static std::wstring reverse_sentence_case(const std::wstring& name, core::rng& engine)
    {
        if (name.empty()) return name;
        std::wstring rsentence_name{lower_case(name, engine)};
        rsentence_name.back() = to_upper_(rsentence_name.back());
        return rsentence_name;
    }

    // Returns the nickname in bathtub case.
    static std::wstring bathtub_case(const std::wstring& name, core::rng& engine)
    {
        if (name.empty()) return name;
        std::wstring bathtub_name{lower_case(name, engine)};
        bathtub_name.at(0) = to_upper_(bathtub_name.at(0));
        bathtub_name.back() = to_upper_(bathtub_name.back());
        return bathtub_name;
    }

    // Returns the nickname in winding case.
    static std::wstring winding_case(const std::wstring& name, core::rng& engine)
    {
        std::wstring winding_name{lower_case(name, engine)};
        for (std::size_t i = 0; i < winding_name.size(); i += 2)
        {
            winding_name[i] = to_upper_(winding_name[i]);
        }
        return winding_name;
    }

    // Returns the nickname with case in a random fashion.
    static std::wstring random_case(const std::wstring& name, core::rng& engine)
    {
        std::wstring random_name{lower_case(name, engine)};
        std::ranges::for_each(random_name, [&engine](wchar_t& character) {
            if (core::bernoulli(engine, 0.5)) character = to_upper_(character);
        });
        return random_name;
    }

    // Returns the nickname all lower case with a single random character
    // uppercase.
    static std::wstring random_single_case(const std::wstring& name, core::rng& engine)
    {
        std::wstring random_name{lower_case(name, engine)};
        if (random_name.empty()) return random_name;
        auto& random_char = core::pick(engine, random_name);
        random_char = to_upper_(random_char);
        return random_name;
    }

    // Format nickname utilizing one of the possible cases.
    static nickname format(nickname nickname, core::rng& engine)
    {
        // 1% chance of snake case. nick_name
        static constexpr double snake_case_probability{0.01};
        if (core::bernoulli(engine, snake_case_probability))
        {
            nickname._internal_string = snake_case(nickname);
        }
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
        nickname._internal_string = core::pick(engine, possible_generators)(nickname, engine);
        return nickname;
    }

    // -- Name-based nicknames -------------------------------------------------

    // Split a full name into a vector containing each name/surname.
    // Empty parts (from consecutive spaces) are filtered out.
    [[nodiscard]] static std::vector<std::wstring> split_name(const std::wstring& name)
    {
        std::vector<std::wstring> parts;
        std::wstring current;
        for (const wchar_t ch : name)
        {
            if (ch == L' ')
            {
                if (!current.empty()) parts.push_back(std::move(current));
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }
        if (!current.empty()) parts.push_back(std::move(current));
        return parts;
    }

    // Returns the first name.
    static std::wstring first_name(const std::wstring& name,
                                   [[maybe_unused]] core::rng& engine)
    {
        return split_name(name).front();
    }

    // Returns the last surname.
    static std::wstring last_name(const std::wstring& name,
                                  [[maybe_unused]] core::rng& engine)
    {
        return split_name(name).back();
    }

    // Returns any name (until it hits a space character).
    static std::wstring any_name(const std::wstring& name, core::rng& engine)
    {
        const auto names_list = split_name(name);
        return core::pick(engine, names_list);
    }

    // Returns only the name initials.
    static std::wstring initials(const std::wstring& name,
                                 [[maybe_unused]] core::rng& engine)
    {
        std::wstring nickname;
        for (const auto& part : split_name(name))
        {
            nickname.push_back(part.front());
        }
        return nickname;
    }

    // Mix the last two names.
    static std::wstring mix_two(const std::wstring& name, core::rng& engine)
    {
        std::wstring nickname;
        auto names_list{split_name(name)};
        if (names_list.size() > 2)
        {
            names_list.erase(names_list.begin(), names_list.end() - 2);
        }
        for (const auto& part : names_list)
        {
            if (part.size() < 2)
            {
                nickname.append(part);
            }
            else
            {
                nickname.append(part.substr(0, core::uniform_int<std::size_t>(engine, 2, part.size())));
            }
        }
        return nickname;
    }

    // Mix first name with last name initial.
    static std::wstring first_plus_initial(const std::wstring& name,
                                           [[maybe_unused]] core::rng& engine)
    {
        const auto names_list{split_name(name)};
        if (names_list.size() < 2) return names_list.front();
        return names_list.front() + names_list.back().front();
    }

    // Mix last name with first name initial.
    static std::wstring initial_plus_last(const std::wstring& name,
                                          [[maybe_unused]] core::rng& engine)
    {
        const auto names_list{split_name(name)};
        if (names_list.size() < 2) return names_list.front();
        return names_list.front().front() + names_list.back();
    }

    // Reduce a random part of the name.
    static std::wstring reduce_single_name(const std::wstring& name, core::rng& engine)
    {
        std::wstring single_name{any_name(name, engine)};
        if (single_name.size() > 3)
        {
            // Remove all vowel characters from the name unless it's already
            // small enough.
            single_name.erase(std::remove_if(single_name.begin() + 1, single_name.end() - 1,
                                             [&single_name](const wchar_t& character) {
                                                 return (_vowels.contains(character) &&
                                                         (single_name.size() > 3));
                                             }),
                              single_name.end() - 1);
        }
        return single_name;
    }

    // The theme lists for a request's tags, each once, in the request's order;
    // tags without a list are left out.
    [[nodiscard]] std::vector<const std::vector<std::wstring>*> themes_of_(
        const std::vector<std::string>& tags) const
    {
        std::vector<const std::vector<std::wstring>*> themes;
        for (const auto& tag : tags)
        {
            const auto it = _themes.find(tag);
            if (it == _themes.end()) continue;
            if (std::ranges::find(themes, &it->second) == themes.end()) themes.push_back(&it->second);
        }
        return themes;
    }

    // Contains logic to generate a random nickname optionally based on the
    // player full name.
    [[nodiscard]] nickname solver(const nickname_request& request, core::rng& engine) const
    {
        const std::wstring& name = request.name;
        std::wstring original;
        std::wstring nick;
        // 1/4 chance of nickname being name related.
        static constexpr double name_related_probability{0.25};
        // 1/3 chance of a themed word when the request has a theme (always, if no
        // plain list is loaded).
        static constexpr double theme_probability{1.0 / 3.0};
        // Always use the name if there are no word lists to fall back to.
        const bool has_name = !split_name(name).empty();
        const bool use_name =
            has_name && (_wordlists.empty() || core::bernoulli(engine, name_related_probability));
        if (use_name)
        {
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
            nick = core::pick(engine, possible_generators)(name, engine);
        }
        else if (const auto themes = themes_of_(request.tags);
                 !themes.empty() && (_wordlists.empty() || core::bernoulli(engine, theme_probability)))
        {
            const auto* drawn_theme = core::pick(engine, themes);
            nick = original = core::pick(engine, *drawn_theme);
        }
        else if (!_wordlists.empty())
        {
            const auto& drawn_wordlist = core::pick(engine, _wordlists);
            nick = original = core::pick(engine, drawn_wordlist);
        }
        else
        {
            throw std::invalid_argument("Received no name and word lists are empty");
        }
        return format(leetify({nick, original}, engine), engine);
    }

    friend struct ::nng_test_access;
};

} // namespace dasmig

#endif // DASMIG_NICKNAMEGEN_HPP
