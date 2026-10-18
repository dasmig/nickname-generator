# Data and Method

What the word lists are and how nicknames are built from them. For using the library, see the [usage guide](usage.md).

[TOC]

## Word Lists

The library ships one tier, `resources/lite/`; `dataset::full` loads the same lists. There are nine plain lists, drawn from for any nickname, and 43 theme lists, drawn from only when a request names their tag.

| List | Words | Content |
| --- | --- | --- |
| `adjectives` | 4,842 | English adjectives, some hyphenated or multi-word ("well-argued") |
| `animals` | 192 | English animal names |
| `japanese` | 233 | Japanese words and slang in Latin script (romaji): "Kawaiso", "Sempai", "Yokatta", ... |
| `colors` | 189 | Colour names ("Crimson", "Cobalt") |
| `food` | 245 | Dishes, sweets, spices and nuts ("Pretzel", "Paprika") |
| `nature` | 237 | Weather, landscape and plants ("Tempest", "Willow") |
| `space` | 226 | Astronomy and spaceflight ("Nebula", "Voyager") |
| `myth` | 260 | Creatures, gods and legends from many traditions ("Kraken", "Valkyrie") |
| `gems` | 167 | Gemstones and minerals ("Opal", "Obsidian") |
| `tag_<tag>` | 24 to 26 each | 43 theme lists, below |

The first three lists are curated; where they came from was not recorded. The 84 animals added in version 5, the six lists added in version 4 and all the theme lists were written for this library. Every word is ASCII. A list file has one word per line; the list's name is the file name.

### Themes

A list named `tag_<tag>` is a theme. It is not part of the plain pool; it is drawn from when a request lists its tag. The 43 shipped themes are the tags that `trait-generator` puts on traits and `occupation-generator` puts on jobs, so a caller can pass them on as they are: `academic`, `activity`, `art`, `care`, `caring`, `chaos`, `combat`, `crafting`, `creative`, `desk`, `discipline`, `dreaming`, `duty`, `exploration`, `faith`, `flight`, `haste`, `knowledge`, `labour`, `leadership`, `manual`, `medicine`, `morale`, `outdoor`, `physical`, `planning`, `rest`, `risk`, `routine`, `safety`, `social`, `solitude`, `solo`, `stealth`, `survival`, `team`, `technical`, `trade`, `training`, `violence`, `waiting`, `wealth`, `willpower`. Each has 24 to 26 words that suit the tag ("Brawler" and "Gladiator" for `combat`, "Hermit" and "Recluse" for `solitude`); they are a flavour, not a definition, and a word can be in a theme and a plain list.

Version 2 removed repeated words, which v1 counted twice: 38 in `animals` (Dog, Rabbit, Fish, ... appeared two or more times) and 4 in `japanese` (Ii, Ki, Omae).

## Generation Model

The per-call input is `nickname_request`; its `name` defaults to empty.
`get_nickname(request)` draws a call seed from the instance's random engine,
and `get_nickname(request, seed)` uses the supplied seed without advancing
that engine. Requests do not load or change word lists.

1. **Source.** With a name, one nickname in four is built from it (always, if no list is loaded): first name, last name, any part (counted twice), initials, a mix of the last two parts, first name plus last initial, first initial plus last name, or one part with its inner vowels removed, each equally likely. Otherwise, if the request has tags and at least one of them is a theme, one nickname in three is a word of a theme (a theme chosen uniformly among the request's themes, each counted once, then a word from it uniformly). Otherwise a plain list is chosen, with a chance proportional to the square root of its word count, then a word from it uniformly. (With the shipped lists `adjectives` is drawn about 37% of the time, each other list 7% to 9%.) The square root keeps a long list from drowning a short one, and keeps a short list a user adds from taking over.
2. **Decoration**, with probability one half: either one of reverse, double the first a/e/i/o, replace one letter with a leet digit, or replace up to two (retried while the result does not change, at most 7 times), or one of add X, reverse, end in y, add digits, add a dash, end in ing.
3. **Case**: snake case first with probability 1%, then one of upper (4/29), lower (8/29), as is (2/29), sentence (5/29), camel (2/29), last letter capital (2/29), first and last capital (3/29), alternating (1/29), random per letter (1/29) or one random capital (1/29).

### Usernames

A request with `style = nickname_style::username` skips decoration and case changes (steps 2 and 3) and builds a username instead, from the same `"nickname"` stream:

1. **Fold.** The name's parts are lowercased and folded to ASCII: Latin-1 and Latin Extended-A accents lose them (`é` is `e`), `ß` is `ss`, `æ` is `ae`, `œ` is `oe`, `þ` is `th`, `ø` is `o`; apostrophes, hyphens and any character with no ASCII form (CJK, Cyrillic, ...) are dropped. A part with nothing left is dropped, and a name with no part left counts as no name.
2. **Names.** Particles between the first and the last part (`da`, `de`, `del`, `van`, `von`, `bin`, `al`, `el`, `y`, ... 22 in all) are left out; a particle that is the first or last part is kept (`Van Helsing`, `Anh Do`). With two names, the second is the family name. With three or more, half the time the second is a middle name, and otherwise the family name is the last, or 40% of the time the one before it (a double surname: "Maria da Silva Santos" is `maria.santos` or `maria.silva`).
3. **Base.** With a name the username is always derived from it. The shapes and their chances: `john.smith` 22%, `johnsmith` 17%, `jsmith` 14%, `john_smith` 9%, `smith.john` 8%, `john.s` 5%, `johns` 6%, `j.smith` 5%, `smith_john` 5%, `smithj` 9%. With a middle name, 40% of the time it is one of `john.m.smith`, `johnmsmith`, `jmsmith`, `john_m_smith`, `jm.smith` or `johnmichael` instead. A one-name request uses the name as it is. Without a name the base is a word, and half the time two words joined by nothing (40%), `_` (40%) or `.` (20%). A word is drawn as in step 1 (a theme word first, for a request with tags; a plain list otherwise, weighted by size) but only the first word is ever a theme word, and a word longer than eight letters is redrawn up to eight times. Words are folded the same way, so "well-argued" is "wellargued".
4. **Prefix or suffix.** One in thirteen gets one: a prefix (`the`, `real`, `its`, `iam`, `hey`, `just`; `the_john.smith`, `realjohn.smith`) or a suffix (`official`, `real`, `hq`, `irl`, `online`, `hi`; `john.smith_official`), each equally likely, with an underscore between 30% (prefix) or 60% (suffix) of the time.
5. **Digits.** Without a birth year: none 40%, a two-digit year (1970 to 2009) 20%, a four-digit year (1970 to 2008) 4%, 1 to 99 14%, a lucky number (`7`, `13`, `21`, `23`, `42`, `77`, `88`, `99`, `100`) 6%, a pattern (`01`, `007`, `123`, `1234`, `11`, `12`, `00`, `10`) 6%, three or four random digits 10%. With `birth_year` (1900 to 2100): none 30%, its two-digit form 28%, its four-digit form 14%, 1 to 99 8%, lucky 5%, pattern 5%, long 10%. Twelve times in a hundred the digits follow an underscore.
6. **Length.** At most 24 characters: a longer base is cut, and a `.` or `_` the cut leaves at the end is removed.

The result is lowercase ASCII letters and digits with `.` and `_` between parts, never ending in a separator. `plain()` is the name, or the words joined by a space.

Everything draws from the `"nickname"` stream split from the call seed, so the result is the same on every platform. Case changes use a fixed mapping for Basic Latin, Latin-1 Supplement and Latin Extended-A, not the C locale; other letters keep their case.

## Known Limitations

- The lists are English and romanised Japanese only.
- A theme has 24 to 26 words, so a character with one tag still meets the same words now and then. Add words to a theme with `extend()`.
- The tags choose a flavour, not a meaning: nothing checks that a word fits a particular trait.
- Name-based nicknames split on spaces only; hyphenated and particle names ("da Silva") are treated as separate parts.
- Usernames are English-shaped: patterns put the first name first and the family name last, a name from a language that writes it the other way round gets the wrong order, and names in other scripts become words.
- Nothing checks that a username is free anywhere or that it passes a site's own rules (some forbid `.` or a leading digit, or allow fewer than 24 characters).
- Leet speak maps a fixed set of Latin letters (o, i, s, e, a, g, t, b, q).
