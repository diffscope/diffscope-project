# Qt Translation Review Checklist

Apply the relevant sections during translation, review, correction, consistency analysis, and
localization-readiness analysis.

## Contents

- [Context and terminology](#context-and-terminology)
- [English source audit](#english-source-audit)
- [en_US translation rules](#en_us-translation-rules)
- [Target-language review](#target-language-review)
- [Structural checks](#structural-checks)
- [Localization-readiness checks](#localization-readiness-checks)
- [Additional reusable tasks](#additional-reusable-tasks)
- [Report format](#report-format)

## Context and Terminology

1. Read `<comment>`, `<extracomment>`, message context, and all source locations.
2. For ambiguous words, inspect the widget/action/model behavior at the location and nearby call
   sites.
3. Build terminology evidence from accepted target-locale translations first.
4. Prefer the dominant translation only when the grammatical and product context match. Do not
   merge homonyms merely because their English source text is equal.
5. Prefer the same module/context over a global majority. Record unresolved terminology conflicts.
6. Use another region of the same language as evidence, not as text to copy mechanically.
7. Preserve product names, file extensions, command names, identifiers, and literal syntax unless
   existing translations establish a localized form.

## English Source Audit

Focus on objective correctness:

- articles, agreement, tense, number, prepositions, word forms, and obvious spelling;
- capitalization consistency for sentence text versus titles/labels;
- spaces around punctuation and placeholders;
- matched quotes, brackets, and parentheses;
- terminal punctuation, ellipsis form, and repeated punctuation;
- accidental duplicated/missing words.

Do not optimize tone, brevity, product meaning, or marketing style. When semantics seem questionable
but grammar is valid, omit the finding or label it explicitly as outside the requested audit.
Always report rather than edit the source.

## en_US Translation Rules

- Empty unfinished non-plural translation: fill with exact engineering source text.
- Empty unfinished plural translation: fill natural singular form followed by natural plural form.
- Non-empty unfinished translation: review as a candidate; replace a divergent non-plural candidate
  with the exact source and correct malformed plural candidates.
- Finished translation: audit separately; do not include it in completion work.
- ID without engineering source: unchanged and reported.
- In `scope=all`, report any non-plural en_US translation that differs from source and any malformed
  English plural; fix only during `fix-translations`.

## Target-Language Review

Check:

- full meaning with no unjustified omission or addition;
- correct sense of ambiguous terms after code inspection;
- natural target-language grammar and word order;
- established project terminology and regional vocabulary;
- appropriate target-language punctuation, quotes, spacing, capitalization, and ellipsis;
- correct singular/plural form for every existing `<numerusform>`;
- untranslated fragments, accidental source-language copies, or suspicious machine literalness.

For completion, write the best-supported candidate when normal source context remains imperfect and
mark it low confidence. For correction, leave unresolved entries unchanged.

Separate the work queue and results:

- **unfinished-empty** — translate from scratch and measure missing coverage;
- **unfinished-nonempty** — review candidate meaning and structure before retaining or revising it;
- **finished** — perform release-quality regression review and prioritize confirmed defects.

Do not run content-validity checks against absent unfinished text. Do report partially empty plural
candidates. Treat a wholly empty finished translation as an error.

## Structural Checks

- Require identical placeholder multisets per translation form; allow reordering only.
- Preserve `%L` and `%Ln` exactly.
- Preserve leading/trailing whitespace and intentional newline structure.
- Preserve escaped literal syntax and file filters such as `*.dspx`.
- Preserve rich-text tag order/nesting and attribute names. Keep `href`, `src`, `style`, `class`,
  and `id` values unchanged unless only translatable visible text is involved.
- Translate visible text inside rich text, never tag or attribute names.
- Match mnemonic intent: distinguish a single mnemonic `&` from escaped `&&` and XML `&amp;`.
- Preserve Qt-recognized shortcut modifiers such as `Ctrl`, `Alt`, and `Shift`.
- Treat `?`/`？`, `!`/`！`, `:`/`：`, `.`/`。`, and ellipsis variants as functional equivalents
  when correct for the target locale.
- Verify URLs, file names, wildcard patterns, code fragments, and literal operators exactly.

Qt Linguist's built-in validation covers accelerators, terminal punctuation, placeholders, and
surrounding whitespace. Run `lcheck` as an additional read-only check when available; the bundled
helper remains the portable baseline.

## Localization-Readiness Checks

Start from messages whose wording indicates a user-visible count, number, duration, date, time,
currency, percentage, size, measurement, line/index, or position and which use ordinary `%1`-style
placeholders.

Inspect how each value is produced:

- suspect direct numeric `.arg(value)`, `QString::number`, QML `.arg(number)`, default
  `QDate/QTime/QDateTime::toString()`, and JavaScript/TypeScript `.toString()`;
- accept `%L1`, `%Ln`, `QLocale::toString`, locale-aware QML/JS methods, or a verified localized
  formatting helper;
- exclude versions, IDs, hashes, ports, hex/error codes, paths, protocol text, machine-readable
  formats, music syntax, and other deliberately invariant values;
- distinguish formatting from plural grammar: a localized number still needs a numerus message
  when surrounding words vary by count.

Report the placeholder, runtime argument, code evidence, why localization is or is not required,
and a recommended source-code change. Never change code or source text through this skill.

## Additional Reusable Tasks

- Pure mechanical inventory by file, language, and total for all message scopes, historical states,
  finished-empty anomalies, plurals, and IDs; do not perform language review or source inspection.
- Sibling drift report for missing or extra active message keys without running `lupdate`.
- Terminology extraction and conflict report using accepted translations as the glossary source.
- Translation-memory suggestions for exact or context-compatible repeated source text.
- ID-source completeness audit for missing `<source>`/`//%` engineering text.
- Empty-finished and suspicious source-equals-translation reports with an allowlist mindset.
- Locale header/file-name consistency audit.
- CI-friendly deterministic validation report; do not generate `.qm`.

## Report Format

Return sections only when they contain information:

1. **Scope** — task, selector, scope, source locale, target locales, files/messages, and separate
   counts for all three review layers.
2. **Changes** — group filled empty unfinished, revised unfinished candidates, and revised finished
   translations; include file, context/ID, old/new summary, confidence, and reason.
3. **Skipped IDs** — ID, locale, TS path, source location, missing evidence.
4. **Translation findings** — separate sections for each review layer with severity, locale,
   message identity, issue, and recommendation.
5. **Source findings** — grammar/punctuation or localization-readiness evidence; report-only.
6. **Unresolved/low confidence** — ambiguity and human-review note.
7. **Safety verification** — unchanged review-state tags, no new files/languages, no source edits,
   no `lupdate`, and no `lrelease`.
