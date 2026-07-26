# Qt Linguist TS Format Rules

Use these rules when discovering, parsing, matching, or modifying Qt TS files.

## Contents

- [Discovery](#discovery)
- [Message identity and state](#message-identity-and-state)
- [ID-based translations](#id-based-translations)
- [Plural forms and placeholders](#plural-forms-and-placeholders)
- [Locations and source files](#locations-and-source-files)
- [Safe mutation](#safe-mutation)
- [Official references](#official-references)

## Discovery

- Treat a file as a Qt translation file only when it is parseable XML and its root local name is
  `TS`. A `.ts` suffix alone is insufficient because TypeScript uses the same suffix.
- Use `TS@language` as the target locale. Normalize hyphens to underscores only for comparisons.
- Group language siblings by their containing translation directory. This skill assumes one module
  family per translation directory and rejects duplicate target locales in a group.
- With a Crowdin selector, expand existing `files[*].source` globs from the project root. Select the
  valid Qt TS source matches and all existing valid Qt TS siblings in their directories. Do not
  interpret Crowdin output placeholders as permission to create files.
- Infer the source locale for each directory in this order: explicit `--source-locale`, Crowdin
  source match, a sibling matching `TS@sourcelanguage`, then `en_US`. Reject ambiguity.

## Message Identity and State

- For an ID message, use `message@id` as its semantic key.
- For a source-text message, use `(context name, source, comment, occurrence index)`. The `comment`
  is disambiguation and is part of identity; `extracomment` is guidance but not identity.
- Preserve document order and all locations.
- An active message has no translation type of `vanished` or `obsolete`.
- `unfinished-empty` means explicit `type="unfinished"` with no non-whitespace translation content.
- `unfinished-nonempty` means explicit `type="unfinished"` with candidate content. For plurals, any
  non-empty form makes the message non-empty; audit empty sibling forms separately.
- `finished` means active without `type="unfinished"`. An empty finished translation stays in this
  layer and is an error; never reclassify it as unfinished or add a state marker.
- `scope=unfinished` selects both unfinished layers. The single-layer scopes select their named
  layer, and `scope=all` selects all active messages.
- Never mutate historical messages.
- In inventory output, `messages_total` counts all XML `<message>` elements, while `scopes.all`
  counts only active messages. Require
  `messages_total == scopes.all + historical.vanished + historical.obsolete`.

## ID-Based Translations

Qt text-ID translation calls use `qtTrId`, `qsTrId`, or related `QT_TRID_*` macros.

- Use non-empty `<source>` as the engineering source when present.
- Otherwise inspect the associated source location for a preceding `//% "Engineering source"`
  comment. Consecutive `//%` strings form one engineering source.
- Treat `//:` and `<extracomment>` as context only. They cannot substitute for missing source text.
- When no engineering source exists, do not create, clear, or replace a translation. Report the ID,
  locale, TS path, and code location.

## Plural Forms and Placeholders

- A plural message has `message@numerus="yes"` and existing `<numerusform>` children.
- Preserve the existing number and order of forms. Refuse to invent missing form elements.
- English has two forms: `n == 1` and `otherwise`. Japanese and Chinese commonly have one; other
  locales follow their Qt plural rules and the existing file structure.
- Preserve the exact placeholder multiset in every translation or plural form:
  `%1`, `%2`, `%L1`, `%L2`, `%n`, and `%Ln` are distinct.
- Placeholder order may change to fit target grammar, but count and spelling may not.
- `%L1` localizes a numeric argument supplied to `QString::arg`; `%Ln` localizes the plural count.

## Locations and Source Files

- Resolve relative `location@filename` values from the TS file directory.
- Support Qt's compressed locations: omitted filenames reuse the current filename; line values
  beginning with `+` or `-` are relative to the last line for that filename.
- A location ending in `.ts` may point to TypeScript source code. Once it is a source location,
  treat it as code unless it independently parses as a selected Qt TS XML file.
- Read comments and nearby code only to resolve meaning or inspect localization formatting. Never
  edit source through this skill.

## Safe Mutation

- Decode XML for reasoning, but patch original UTF-8 text spans rather than serializing the XML
  tree. General XML serialization can rewrite declarations, entities, whitespace, and indentation.
- Patch only character data inside an existing `<translation>` or `<numerusform>`.
- XML-escape `&`, `<`, `>`, double quotes, and apostrophes in new character data.
- Preserve BOM state and dominant newline style.
- Before writing, verify the original file hash, entry translation hash, scope, active state,
  placeholder/markup invariants, and plural-form count.
- Snapshot every exact `<translation ...>` opening tag. After constructing and after writing the
  new file, require the full ordered snapshot to be byte-for-byte identical.
- Parse every constructed file as XML before any write. Preflight the whole change set, use
  same-directory atomic replacement, and roll back files already written if verification fails.

## Official References

- [TS file format](https://doc.qt.io/qt-6/linguist-ts-file-format.html)
- [Text ID based translations](https://doc.qt.io/qt-6/linguist-id-based-i18n.html)
- [Plural rules](https://doc.qt.io/qt-6/i18n-plural-rules.html)
- [Validating translations](https://doc.qt.io/qt-6/linguist-validating-translations.html)
- [Internationalization source translation](https://doc.qt.io/qt-6/i18n-source-translation.html)
- [QString localized arguments](https://doc.qt.io/qt-6/qstring.html)
