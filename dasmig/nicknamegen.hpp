// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 Diego Dasso Migotto

#ifndef DASMIG_NICKNAMEGEN_HPP
#define DASMIG_NICKNAMEGEN_HPP

#include "core.hpp"
#include <algorithm>
#include <cmath>
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

/// @brief The kind of nickname a request asks for.
enum class nickname_style : std::uint8_t
{
    gaming,   ///< A gaming handle: decorated and cased freely ("xX_Nioi_Xx", "dR4g0n").
    username, ///< A username: lowercase letters and digits, with `.` or `_` ("john.smith92").
};

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

    /// @brief The kind of nickname: a gaming handle (the default) or a username.
    ///
    /// A gaming handle may be decorated, leetified and cased freely. A
    /// username is lowercase ASCII letters and digits, at most 24 characters,
    /// with `.` or `_` between parts: from a name it is the name in a common
    /// shape ("john.smith", "jsmith", "smith_john", "johns92"), always
    /// derived from the name; without a name it is one or two words from the
    /// lists ("swiftfox", "quiet_otter"), with digits about half the time.
    /// Accents are folded to ASCII ("jose" for "José") and characters
    /// that have no ASCII form are dropped; a name with no letters left is
    /// treated as no name. A username sometimes gets a prefix ("real", "the",
    /// "its") or a suffix ("official", "hq", "irl"), about one in thirteen.
    nickname_style style{nickname_style::gaming};

    /// @brief Birth year, for the digits of a username.
    ///
    /// With a year from 1900 to 2100, the digits of a username are often the
    /// year, as two or four digits ("john.smith92", "jsmith1992"). Without it
    /// they are a made-up year, a small or lucky number, or a pattern.
    /// Ignored by the gaming style.
    std::optional<int> birth_year{};
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
    core::discrete _list_choice{1.0}; // over _wordlists, by the square root of their sizes
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
        // A list's chance grows with its size, but only by the square root, so
        // a long list does not drown a short one and a short one does not
        // take over from a long one.
        if (!wordlists.empty())
        {
            std::vector<double> weights;
            weights.reserve(wordlists.size());
            for (const auto& list : wordlists) weights.push_back(std::sqrt(static_cast<double>(list.size())));
            _list_choice = core::discrete{weights};
        }
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

    // -- Usernames ----------------------------------------------------------

    static constexpr std::size_t username_max_length_ = 24;

    // Lowercase ASCII letters and digits only: accents are folded (the same
    // Latin ranges as the case mapping), anything else is dropped.
    [[nodiscard]] static std::wstring fold_ascii_(const std::wstring& text)
    {
        // Latin Extended-A (U+0100 to U+017F), one letter per code point; the
        // ligatures (U+0132, U+0133, U+0152, U+0153) are handled apart.
        static constexpr std::string_view extended_a =
            "aaaaaacc" "ccccccdd" "ddeeeeee" "eeeegggg" "gggghhhh" "iiiiiiii" "iiiijjkk" "klllllll"
            "lllnnnnn" "nnnnoooo" "oooorrrr" "rrssssss" "sstttttt" "uuuuuuuu" "uuuuwwyy" "yzzzzzzs";
        std::wstring folded;
        for (const wchar_t original : text)
        {
            const wchar_t c = to_lower_(original);
            const auto u = static_cast<std::uint32_t>(c);
            if ((u >= 0x61U && u <= 0x7AU) || (u >= 0x30U && u <= 0x39U))
            {
                folded.push_back(c);
            }
            else if (u >= 0xE0U && u <= 0xFFU)
            {
                if (u <= 0xE5U) folded.push_back(L'a');
                else if (u == 0xE6U) folded.append(L"ae");
                else if (u == 0xE7U) folded.push_back(L'c');
                else if (u <= 0xEBU) folded.push_back(L'e');
                else if (u <= 0xEFU) folded.push_back(L'i');
                else if (u == 0xF0U) folded.push_back(L'd');
                else if (u == 0xF1U) folded.push_back(L'n');
                else if (u <= 0xF6U || u == 0xF8U) folded.push_back(L'o');
                else if (u >= 0xF9U && u <= 0xFCU) folded.push_back(L'u');
                else if (u == 0xFDU || u == 0xFFU) folded.push_back(L'y');
                else if (u == 0xFEU) folded.append(L"th");
            }
            else if (u == 0xDFU)
            {
                folded.append(L"ss");
            }
            else if (u >= 0x100U && u <= 0x17FU)
            {
                if (u == 0x132U || u == 0x133U) folded.append(L"ij");
                else if (u == 0x152U || u == 0x153U) folded.append(L"oe");
                else folded.push_back(static_cast<wchar_t>(extended_a[u - 0x100U]));
            }
        }
        return folded;
    }

    // The parts of a name that survive folding.
    [[nodiscard]] static std::vector<std::wstring> username_parts_(const std::wstring& name)
    {
        std::vector<std::wstring> parts;
        for (const auto& part : split_name(name))
        {
            if (auto folded = fold_ascii_(part); !folded.empty()) parts.push_back(std::move(folded));
        }
        return parts;
    }

    // A word of a theme or a list, as a username part: short when it can be.
    [[nodiscard]] std::wstring username_word_(const std::vector<const std::vector<std::wstring>*>& themes,
                                              bool allow_theme, core::rng& engine) const
    {
        static constexpr double theme_probability{1.0 / 3.0};
        static constexpr std::size_t preferred_length = 8;
        static constexpr int attempts = 8;
        std::wstring word;
        for (int attempt = 0; attempt < attempts; ++attempt)
        {
            const std::vector<std::wstring>* list = nullptr;
            if (allow_theme && !themes.empty() &&
                (_wordlists.empty() || core::bernoulli(engine, theme_probability)))
            {
                list = core::pick(engine, themes);
            }
            else if (!_wordlists.empty())
            {
                list = &_wordlists[_list_choice(engine)];
            }
            else
            {
                throw std::invalid_argument("Received no name and word lists are empty");
            }
            word = fold_ascii_(core::pick(engine, *list));
            if (!word.empty() && word.size() <= preferred_length) return word;
        }
        return word.empty() ? L"user" : word;
    }

    // Picks an index by weights that add up to 100.
    template <std::size_t N>
    [[nodiscard]] static std::size_t weighted_index_(core::rng& engine, const std::array<int, N>& weights)
    {
        auto r = core::uniform_int(engine, 0, 99);
        for (std::size_t i = 0; i < N; ++i)
        {
            if (r < weights[i]) return i;
            r -= weights[i];
        }
        return N - 1;
    }

    // Two digits of a year, or four.
    [[nodiscard]] static std::wstring year_digits_(int year, bool two)
    {
        if (!two) return std::to_wstring(year);
        const int yy = year % 100;
        return (yy < 10 ? L"0" : L"") + std::to_wstring(yy);
    }

    // Digits at the end of a username: a year, a small or lucky number, a
    // pattern, or none.
    [[nodiscard]] static std::wstring username_digits_(const std::optional<int>& birth_year,
                                                       core::rng& engine)
    {
        static constexpr std::array<std::wstring_view, 9> lucky{L"7",  L"13", L"21", L"23", L"42",
                                                                 L"77", L"88", L"99", L"100"};
        static constexpr std::array<std::wstring_view, 8> patterns{L"01", L"007", L"123", L"1234",
                                                                    L"11", L"12",  L"00",  L"10"};
        const bool known = birth_year && *birth_year >= 1900 && *birth_year <= 2100;
        // none, two-digit year, four-digit year, small, lucky, pattern, long
        static constexpr std::array<int, 7> unknown_weights{40, 20, 4, 14, 6, 6, 10};
        static constexpr std::array<int, 7> known_weights{30, 28, 14, 8, 5, 5, 10};
        std::wstring digits;
        switch (weighted_index_(engine, known ? known_weights : unknown_weights))
        {
        case 0:
            return {};
        case 1:
            digits = year_digits_(known ? *birth_year : 1970 + core::uniform_int(engine, 0, 39), true);
            break;
        case 2:
            digits = year_digits_(known ? *birth_year : core::uniform_int(engine, 1970, 2008), false);
            break;
        case 3:
            digits = std::to_wstring(core::uniform_int(engine, 1, 99));
            break;
        case 4:
            digits = std::wstring{core::pick(engine, lucky)};
            break;
        case 5:
            digits = std::wstring{core::pick(engine, patterns)};
            break;
        default:
            digits = std::to_wstring(core::uniform_int(engine, 100, 9999));
            break;
        }
        if (core::bernoulli(engine, 0.12)) digits.insert(0, 1, L'_');
        return digits;
    }

    // Words that sit between the parts of a name and are not a name of their own.
    [[nodiscard]] static bool is_particle_(const std::wstring& part)
    {
        static const std::array<std::wstring_view, 22> particles{
            L"da", L"das", L"de",  L"del", L"della", L"di", L"do",  L"dos", L"du",  L"la", L"le",
            L"van", L"von", L"der", L"den", L"ten",  L"ter", L"bin", L"ibn", L"al", L"el", L"y"};
        return std::ranges::find(particles, part) != particles.end();
    }

    // The name in one of the shapes usernames take.
    [[nodiscard]] static std::wstring username_from_name_(const std::vector<std::wstring>& parts,
                                                          core::rng& engine)
    {
        // The name's parts without the particles between them ("da", "von", ...).
        std::vector<std::wstring> names;
        for (std::size_t i = 0; i < parts.size(); ++i)
        {
            const bool inner = i > 0 && i + 1 < parts.size();
            if (!(inner && is_particle_(parts[i]))) names.push_back(parts[i]);
        }
        const std::wstring& given = names.front();
        if (names.size() == 1) return given;

        // With three or more names, the middle one is a middle name or the first
        // of two family names, and the family name may be either of the last two.
        std::wstring family = names.back();
        std::wstring middle;
        if (names.size() >= 3)
        {
            const bool middle_name = core::bernoulli(engine, 0.5);
            if (middle_name)
            {
                middle = names[1];
            }
            else if (core::bernoulli(engine, 0.4))
            {
                family = names[names.size() - 2];
            }
        }
        const std::wstring g = given.substr(0, 1);
        const std::wstring f = family.substr(0, 1);

        if (!middle.empty() && core::bernoulli(engine, 0.4))
        {
            const std::wstring m = middle.substr(0, 1);
            switch (core::uniform_int(engine, 0, 5))
            {
            case 0: return given + L"." + m + L"." + family; // john.m.smith
            case 1: return given + m + family;               // johnmsmith
            case 2: return g + m + family;                   // jmsmith
            case 3: return given + L"_" + m + L"_" + family; // john_m_smith
            case 4: return g + m + L"." + family;            // jm.smith
            default: return given + middle;                  // johnmichael
            }
        }

        // given.family, givenfamily, given_family, gfamily, g.family, given.f,
        // givenf, family.given, family_given, familyg
        static constexpr std::array<int, 10> weights{22, 17, 9, 14, 5, 5, 6, 8, 5, 9};
        switch (weighted_index_(engine, weights))
        {
        case 0: return given + L"." + family;
        case 1: return given + family;
        case 2: return given + L"_" + family;
        case 3: return g + family;
        case 4: return g + L"." + family;
        case 5: return given + L"." + f;
        case 6: return given + f;
        case 7: return family + L"." + given;
        case 8: return family + L"_" + given;
        default: return family + g;
        }
    }

    // A prefix or a suffix, about one username in thirteen.
    static void username_affix_(std::wstring& base, core::rng& engine)
    {
        static constexpr std::array<std::wstring_view, 6> prefixes{L"the", L"real", L"its", L"iam", L"hey", L"just"};
        static constexpr std::array<std::wstring_view, 6> suffixes{L"official", L"real", L"hq", L"irl", L"online", L"hi"};
        if (!core::bernoulli(engine, 1.0 / 13.0)) return;
        if (core::bernoulli(engine, 0.5))
        {
            const std::wstring prefix{core::pick(engine, prefixes)};
            base = prefix + (core::bernoulli(engine, 0.3) ? L"_" : L"") + base;
        }
        else
        {
            const std::wstring suffix{core::pick(engine, suffixes)};
            base += (core::bernoulli(engine, 0.6) ? L"_" : L"") + suffix;
        }
    }

    // A username: the name in a common shape, or one or two words.
    [[nodiscard]] nickname username_(const nickname_request& request, core::rng& engine) const
    {
        std::wstring original;
        std::wstring base;
        if (const auto parts = username_parts_(request.name); !parts.empty())
        {
            original = request.name;
            base = username_from_name_(parts, engine);
        }
        else
        {
            const auto themes = themes_of_(request.tags);
            std::wstring first = username_word_(themes, true, engine);
            original = first;
            base = first;
            if (core::bernoulli(engine, 0.5))
            {
                const std::wstring second = username_word_(themes, false, engine);
                original += L" " + second;
                switch (core::uniform_int(engine, 0, 4))
                {
                case 0: case 1: base += second; break;
                case 2: case 3: base += L"_" + second; break;
                default: base += L"." + second; break;
                }
            }
        }
        username_affix_(base, engine);

        const std::wstring digits = username_digits_(request.birth_year, engine);
        if (base.size() + digits.size() > username_max_length_)
        {
            base.resize(username_max_length_ - std::min(digits.size(), username_max_length_ - 1));
            while (base.size() > 1 && (base.back() == L'.' || base.back() == L'_')) base.pop_back();
        }
        return {base + digits, original};
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
        if (request.style == nickname_style::username) return username_(request, engine);
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
            const auto& drawn_wordlist = _wordlists[_list_choice(engine)];
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
