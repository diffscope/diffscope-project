---
name: manage-qt-ts-translations
description: Audit, complete, translate, validate, and safely patch existing Qt Linguist TS translation files. Use for English source proofreading, en_US completion, translation into one or all existing locales, translation review and correction, terminology or sibling-file consistency checks, coverage inventories, and source-code localization-readiness checks involving Qt placeholders, plurals, rich text, numbers, dates, or times. Operate only on existing TS files and preserve every translation review-state attribute.
---

# Manage Qt TS Translations

Work on existing Qt Linguist `.ts` XML files without changing their review states. Use the bundled
helper for deterministic discovery, parsing, validation, batching, and minimal XML patching. Perform
language judgment yourself after reading the relevant repository context.

## Require Inputs

Resolve these inputs before doing work:

1. Choose exactly one selector:
   - one or more translation directories; or
   - a `crowdin.yml` whose `files[*].source` entries identify the source TS files.
2. Choose a task from the table below.
3. For message-processing tasks, require one scope:
   `unfinished-empty`, `unfinished-nonempty`, `unfinished`, `finished`, or `all`.
   Restrict `complete-en-us` and `translate` to the first three unfinished scopes.
4. For target-language tasks, require a locale, a locale list, or `all`.

Ask only for inputs that cannot be inferred safely. Treat locale values from `TS@language` as
authoritative; do not rely solely on file names.

| Task | Behavior |
| --- | --- |
| `audit-source` | Review English source grammar, spelling, capitalization, and punctuation. Report only. |
| `complete-en-us` | Fill empty unfinished en_US entries and separately review/correct non-empty unfinished candidates. |
| `translate` | Translate empty unfinished entries from scratch and separately review/correct non-empty unfinished candidates. |
| `audit-translations` | Review requested layers separately for en_US rules and target-language correctness. Report only. |
| `fix-translations` | Run the same layered review, then patch only issues that are confidently resolved. |
| `inventory` | Mechanically aggregate per-file, per-language, and total counts without source inspection or language judgment. |
| `audit-consistency` | Report sibling alignment, repeated-term conflicts, and inconsistent translations. |
| `audit-localization-readiness` | Inspect candidate source locations for missing locale-aware number/date/time formatting. Report only. |

Classify every active entry into exactly one review layer:

- `unfinished-empty`: explicit `type="unfinished"` and no non-whitespace translation content;
- `unfinished-nonempty`: explicit `type="unfinished"` with candidate content; for plurals, any
  non-empty form puts the whole message in this layer and empty sibling forms remain findings;
- `finished`: active without `type="unfinished"`, whether content is present or not. Treat an empty
  finished entry as an `empty-finished` error, never as unfinished.

`unfinished` selects the first two layers; `all` selects all three. Always exclude `vanished` and
`obsolete`. Never merge findings or counts across layers in an audit report.

## Enforce Invariants

- Never add, remove, reorder, or alter any `<translation>` attribute or review-state marker.
- Never run `lupdate`, `lrelease`, or create `.qm` files.
- Never create languages, translation files, messages, plural-form elements, or missing translations.
- Never edit `<source>`, comments, locations, source code, or TypeScript source files.
- Patch only the character data inside an existing `<translation>` or `<numerusform>`.
- Skip an ID-based message unless it has an engineering source in `<source>` or an associated
  `//%` source comment. Keep it byte-for-byte unchanged and report it.
- Refuse to patch malformed XML, stale manifest entries, unsupported nested translation XML,
  missing plural-form elements, placeholder mismatches, or files outside the selected manifest.
- Preserve UTF-8 BOM state, newline style, indentation, surrounding XML, and all opening tags.

Read [references/qt-ts-format.md](references/qt-ts-format.md) before parsing, matching, or patching.
Read [references/review-checklist.md](references/review-checklist.md) before any language review,
translation, correction, consistency audit, or localization-readiness audit.

## Run Pure Inventory

For `inventory`, use the dedicated read-only command and stop after reporting its result:

```text
python <skill>/scripts/qt_ts_tool.py inventory \
  --directory <translation-dir-or-file> \
  --directory <another-file> \
  --project-root <project-root> \
  --output <temp>/inventory.json
```

Replace the directory arguments with `--crowdin <project>/crowdin.yml` when requested. Add repeated
`--locale` values only to filter existing files; omit them to include every discovered locale.
Do not ask for a scope because inventory always calculates every scope.

Report the `files` rows first, then `by_language`, then `totals`. Each row includes:

- `messages_total`: every `<message>`, including `vanished` and `obsolete`;
- `active_total` and `historical_total`;
- `scopes.unfinished-empty`, `scopes.unfinished-nonempty`, their combined
  `scopes.unfinished`, `scopes.finished`, and `scopes.all`;
- separate vanished/obsolete, finished-empty, plural, and ID counts.

Keep `scopes.all` distinct from `messages_total`: `all` means all active messages and excludes
historical entries. This command must not inspect source files, infer meaning, run validation, or
modify anything.

## Run the Deterministic Workflow

Use a temporary directory outside the repository for manifests and change plans.

1. Scan the selected files:

   ```text
   python <skill>/scripts/qt_ts_tool.py scan \
     --directory <translation-dir> \
     --scope unfinished-empty \
     --locale zh_CN \
     --output <temp>/manifest.json
   ```

   Repeat `--directory` and `--locale` as needed. For Crowdin selection, replace the directory
   arguments with `--crowdin <project>/crowdin.yml`. Use `--source-locale` only when automatic
   source-locale inference is ambiguous.

2. Review `summary_by_language`, `selected_by_review_layer`,
   `selected_by_language_and_review_layer`, `skipped_files`, `alignment_issues`, `source_issues`,
   and `localization_candidates`. Process `batches` in order; do not stop after a representative
   sample.
   A context stays in one batch when it fits. An oversized context is emitted as consecutive pages
   of at most 80 messages; a message and all of its plural forms always stay together.
3. For report-only tasks, combine deterministic findings with language review and inspect the
   referenced source locations when context is needed.
4. For modifying tasks, create a UTF-8 JSON change plan:

   ```json
   {
     "manifest_hash": "<manifest manifest_hash>",
     "changes": [
       {
         "entry_id": "<entry id>",
         "translation": "Translated text",
         "confidence": "high",
         "reason": "Short reason"
       },
       {
         "entry_id": "<plural entry id>",
         "forms": ["Singular form", "Plural form"],
         "confidence": "high",
         "reason": "English plural forms"
       }
     ]
   }
   ```

5. Validate the intended changes without writing:

   ```text
   python <skill>/scripts/qt_ts_tool.py apply \
     --manifest <temp>/manifest.json \
     --changes <temp>/changes.json \
     --dry-run
   ```

6. Fix every reported hard error. Then run the same command without `--dry-run`. The helper
   preflights all files, writes atomically, verifies XML and opening-tag snapshots, and rolls back
   already-written files if post-write verification fails.
7. Run post-change validation:

   ```text
   python <skill>/scripts/qt_ts_tool.py validate \
     --manifest <temp>/manifest.json
   ```

   A manifest intentionally becomes stale after a successful patch; `validate` permits changed
   translation text but still verifies file identity, scope, XML structure, and review-state
   opening tags. Rescan before preparing another change plan.

## Apply Task-Specific Judgment

### Audit English Sources

Review each distinct canonical-source message once. Focus on objective grammar, spelling,
capitalization, spacing, quote/bracket balance, and punctuation. Avoid stylistic rewrites and
product-copy or semantic criticism unless required to explain an objective language error. Report
the source location and a suggested correction; do not edit it.

### Complete en_US

- In `unfinished-empty`, fill non-plural messages with the exact engineering source and fill the
  existing two plural forms using `n == 1` and `otherwise`.
- In `unfinished-nonempty`, treat text as a candidate to review. Replace a divergent non-plural
  candidate with the exact engineering source and correct plural candidates as needed.
- Preserve every placeholder exactly, including the `L` in `%L1` or `%Ln`.
- Report filled-empty and revised-candidate counts separately.
- Do not process `finished` entries in this task.
- Skip ID entries without an engineering source.

### Translate Existing Locales

Use this priority for terminology and meaning:

1. accepted translations in the target locale, especially the same module and context;
2. existing candidate translations in the target locale;
3. sibling regions of the same language;
4. translator/developer comments and source locations;
5. other locales as semantic hints only.

Do not mechanically convert scripts or copy regional wording. Preserve established terminology
when context agrees. If ambiguity remains after inspecting code, supply the best-supported
candidate for completion tasks and mark it low confidence in the report. In fix tasks, leave an
unresolved entry unchanged and report it.

- In `unfinished-empty`, translate from the engineering source without treating the absence of text
  as a translation defect.
- In `unfinished-nonempty`, review the candidate first; retain it when correct, revise it when
  confidently wrong, and report retained/revised/low-confidence candidates separately.
- Do not modify `finished` entries through `translate`; use `audit-translations` or
  `fix-translations` with `scope=finished` or `scope=all`.

### Audit and Fix Translations

Check semantic fidelity, omissions/additions, grammar, terminology, punctuation intent,
placeholders, plural forms, rich-text structure, mnemonics, whitespace/newlines, shortcuts, and
file-filter patterns. Treat en_US specially: non-plural translations must equal source text and
plural forms must be correct English. Apply only translation-text fixes; report source/code issues.

Audit in three separate sections:

1. `unfinished-empty`: report missing coverage, missing engineering sources, and unpatchable
   structure; do not emit placeholder/HTML mismatch noise for absent text.
2. `unfinished-nonempty`: fully review candidate content, including partially empty plural forms.
3. `finished`: apply release-quality checks; treat empty finished entries as errors and give defects
   higher priority because the file declares them complete.

For `fix-translations`, allow changes in the selected layer only. Record the layer before and after
each content edit; the XML review-state marker must remain unchanged.

### Audit Localization Readiness

Treat helper findings as candidates, not automatic defects. Inspect code around every candidate:

- flag numeric/date/time/currency/size values formatted through ordinary `%1`, `.arg(number)`,
  `QString::number`, or non-localized `toString()` when the value is user-facing;
- do not flag identifiers, versions, paths, protocol values, hex values, or values already
  formatted through `%L`, `%Ln`, `QLocale`, `toLocaleString`, or a clearly locale-aware helper;
- report the TS message, source location, argument path, evidence, and recommended source-code
  correction without modifying code.

## Report Results

Always report:

- selector, task, scope, locales, file/message counts;
- separate counts for `unfinished-empty`, `unfinished-nonempty`, and `finished`;
- changed files and message counts split into filled-empty, revised-candidate, and revised-finished,
  or an explicit no-change result;
- skipped ID messages without engineering sources;
- unresolved or low-confidence translations;
- deterministic validation errors and warnings;
- source grammar and localization-readiness findings;
- confirmation that no source files, languages, review-state attributes, `lupdate`, or `lrelease`
  were involved.

For `inventory`, replace the normal task report with the pure per-file/language/total count tables
and its read-only safety confirmation.
