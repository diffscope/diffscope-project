#!/usr/bin/env python3
"""Discover, inspect, validate, and minimally patch existing Qt Linguist TS files."""

from __future__ import annotations

import argparse
import ast
import codecs
from collections import Counter, defaultdict
from dataclasses import dataclass
from hashlib import sha256
from html.parser import HTMLParser
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tempfile
from typing import Any, Iterable
import xml.etree.ElementTree as ET


MANIFEST_VERSION = 2
INVENTORY_VERSION = 1
HISTORICAL_TYPES = {"vanished", "obsolete"}
REVIEW_LAYERS = ("unfinished-empty", "unfinished-nonempty", "finished")
SCOPES = ("unfinished-empty", "unfinished-nonempty", "unfinished", "finished", "all")
PLACEHOLDER_RE = re.compile(r"%(?:L?n|L?[1-9][0-9]*)")
ORDINARY_NUMBERED_PLACEHOLDER_RE = re.compile(r"%(?!L)([1-9][0-9]*)")
MESSAGE_OPEN_RE = re.compile(r"<message\b(?:\"[^\"]*\"|'[^']*'|[^'\">])*>", re.IGNORECASE)
MESSAGE_CLOSE_RE = re.compile(r"</message\s*>", re.IGNORECASE)
TRANSLATION_OPEN_RE = re.compile(
    r"<translation\b(?:\"[^\"]*\"|'[^']*'|[^'\">])*>", re.IGNORECASE
)
TRANSLATION_CLOSE_RE = re.compile(r"</translation\s*>", re.IGNORECASE)
NUMERUS_OPEN_RE = re.compile(r"<numerusform\b(?:\"[^\"]*\"|'[^']*'|[^'\">])*>", re.IGNORECASE)
NUMERUS_CLOSE_RE = re.compile(r"</numerusform\s*>", re.IGNORECASE)
RICH_TAG_RE = re.compile(
    r"</?(?:a|b|br|p|h[1-6]|ul|ol|li|span|div|i|em|strong|table|tr|td|th)\b",
    re.IGNORECASE,
)
FILTER_RE = re.compile(r"\*\.[A-Za-z0-9*?._-]+")
SHORTCUT_RE = re.compile(r"\b(?:Ctrl|Alt|Shift|Meta)(?:\+[A-Za-z0-9]+)+")
NUMERIC_SEMANTIC_RE = re.compile(
    r"\b(?:"
    r"count|number|total|selected|line|row|column|index|position|offset|length|size|"
    r"byte|kilobyte|megabyte|gigabyte|percent|percentage|ratio|value|amount|price|cost|"
    r"second|millisecond|minute|hour|day|week|month|year|date|time|duration|elapsed|"
    r"track|clip|note|tick|measure|beat|warning|error|notification|item|file"
    r")s?\b",
    re.IGNORECASE,
)
LIKELY_INVARIANT_RE = re.compile(
    r"\b(?:version|identifier|id|hash|commit|port|hex|error code|file path|url|uri)\b",
    re.IGNORECASE,
)
LOCALE_EVIDENCE_RE = re.compile(
    r"QLocale|toLocaleString|toLocaleDateString|toLocaleTimeString|%L(?:n|[1-9])"
)
FORMATTING_EVIDENCE_RE = re.compile(r"\.arg\s*\(|QString::number|\.toString\s*\(")
CRITICAL_HTML_ATTRIBUTES = {"href", "src", "style", "class", "id"}
VOID_HTML_TAGS = {"br", "hr", "img", "meta", "link", "input"}


class ToolError(RuntimeError):
    """An expected safety or input error."""


@dataclass
class TextFile:
    raw: bytes
    text: str
    bom: bool
    newline: str


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def normalize_locale(value: str | None) -> str:
    return (value or "").replace("-", "_")


def review_layer(
    *, active: bool, unfinished: bool, has_translation_content: bool
) -> str:
    if not active:
        return "historical"
    if not unfinished:
        return "finished"
    return "unfinished-nonempty" if has_translation_content else "unfinished-empty"


def entry_matches_scope(entry: dict[str, Any], scope: str) -> bool:
    if not entry["active"]:
        return False
    layer = entry["review_layer"]
    if scope == "all":
        return True
    if scope == "unfinished":
        return layer in {"unfinished-empty", "unfinished-nonempty"}
    return layer == scope


def sha256_bytes(data: bytes) -> str:
    return sha256(data).hexdigest()


def sha256_text(value: str) -> str:
    return sha256(value.encode("utf-8")).hexdigest()


def canonical_json(value: Any) -> str:
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))


def sha256_json(value: Any) -> str:
    return sha256_text(canonical_json(value))


def relative_display(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return str(path.resolve())


def resolve_input_path(value: str, root: Path) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = root / path
    return path.resolve()


def read_utf8(path: Path) -> TextFile:
    raw = path.read_bytes()
    bom = raw.startswith(codecs.BOM_UTF8)
    payload = raw[len(codecs.BOM_UTF8) :] if bom else raw
    try:
        text = payload.decode("utf-8")
    except UnicodeDecodeError as error:
        raise ToolError(f"{path}: only UTF-8 Qt TS files are safe to patch ({error})") from error
    crlf = text.count("\r\n")
    lf_only = len(re.findall(r"(?<!\r)\n", text))
    newline = "\r\n" if crlf > lf_only else "\n"
    return TextFile(raw=raw, text=text, bom=bom, newline=newline)


def encode_text_file(text_file: TextFile, text: str) -> bytes:
    payload = text.encode("utf-8")
    return codecs.BOM_UTF8 + payload if text_file.bom else payload


def element_text(element: ET.Element | None) -> str:
    if element is None:
        return ""
    return "".join(element.itertext())


def direct_child(element: ET.Element, name: str) -> ET.Element | None:
    for child in element:
        if local_name(child.tag) == name:
            return child
    return None


def direct_children(element: ET.Element, name: str) -> list[ET.Element]:
    return [child for child in element if local_name(child.tag) == name]


def _span(open_match: re.Match[str], close_match: re.Match[str]) -> dict[str, Any]:
    return {
        "open_start": open_match.start(),
        "open_end": open_match.end(),
        "inner_start": open_match.end(),
        "inner_end": close_match.start(),
        "close_end": close_match.end(),
        "open_tag": open_match.group(0),
    }


def lexical_message_spans(text: str) -> list[dict[str, Any]]:
    spans: list[dict[str, Any]] = []
    search_position = 0
    while True:
        message_open = MESSAGE_OPEN_RE.search(text, search_position)
        if message_open is None:
            break
        message_close = MESSAGE_CLOSE_RE.search(text, message_open.end())
        if message_close is None:
            raise ToolError("Unclosed <message> element in lexical TS scan")
        block_start = message_open.start()
        block_end = message_close.end()
        translation_open = TRANSLATION_OPEN_RE.search(text, message_open.end(), message_close.start())
        translation: dict[str, Any] | None = None
        forms: list[dict[str, Any]] = []
        if translation_open is not None:
            self_closing = translation_open.group(0).rstrip().endswith("/>")
            if self_closing:
                translation = {
                    "open_start": translation_open.start(),
                    "open_end": translation_open.end(),
                    "inner_start": None,
                    "inner_end": None,
                    "close_end": translation_open.end(),
                    "open_tag": translation_open.group(0),
                    "self_closing": True,
                }
            else:
                translation_close = TRANSLATION_CLOSE_RE.search(
                    text, translation_open.end(), message_close.start()
                )
                if translation_close is None:
                    raise ToolError("Unclosed <translation> element in lexical TS scan")
                translation = _span(translation_open, translation_close)
                translation["self_closing"] = False
                form_position = translation["inner_start"]
                while True:
                    form_open = NUMERUS_OPEN_RE.search(
                        text, form_position, translation["inner_end"]
                    )
                    if form_open is None:
                        break
                    form_close = NUMERUS_CLOSE_RE.search(
                        text, form_open.end(), translation["inner_end"]
                    )
                    if form_close is None:
                        raise ToolError("Unclosed <numerusform> element in lexical TS scan")
                    forms.append(_span(form_open, form_close))
                    form_position = form_close.end()
        spans.append(
            {
                "message_start": block_start,
                "message_end": block_end,
                "translation": translation,
                "forms": forms,
            }
        )
        search_position = block_end
    return spans


def translation_opening_snapshot(text: str | bytes) -> tuple[str, int]:
    if isinstance(text, bytes):
        payload = text[len(codecs.BOM_UTF8) :] if text.startswith(codecs.BOM_UTF8) else text
        text = payload.decode("utf-8")
    spans = lexical_message_spans(text)
    opening_tags = [
        span["translation"]["open_tag"]
        for span in spans
        if span["translation"] is not None
    ]
    return sha256_json(opening_tags), len(opening_tags)


def xml_escape_text(value: str) -> str:
    return (
        value.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
        .replace("'", "&apos;")
    )


def normalize_newlines(value: str, newline: str) -> str:
    normalized = value.replace("\r\n", "\n").replace("\r", "\n")
    return normalized.replace("\n", newline)


def _yaml_scalar(value: str) -> str:
    value = value.strip()
    if not value:
        return ""
    if value[0] == value[-1] and value[0] in {'"', "'"}:
        if value[0] == '"':
            try:
                return str(ast.literal_eval(value))
            except (ValueError, SyntaxError):
                return value[1:-1]
        return value[1:-1].replace("''", "'")
    return re.sub(r"\s+#.*$", "", value).strip()


def parse_crowdin_sources(path: Path) -> list[str]:
    sources: list[str] = []
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        match = re.match(r"^\s*(?:-\s*)?source\s*:\s*(.*?)\s*$", line)
        if match:
            scalar = _yaml_scalar(match.group(1))
            if scalar:
                sources.append(scalar)
    if not sources:
        raise ToolError(
            f"{path}: no simple files[*].source entries found; use --directory for unsupported YAML"
        )
    return sources


def probe_qt_ts(path: Path) -> tuple[bool, str]:
    try:
        raw = path.read_bytes()
    except OSError as error:
        return False, f"unreadable: {error}"
    if not raw.lstrip(codecs.BOM_UTF8).lstrip().startswith(b"<"):
        return False, "not XML (likely TypeScript or another .ts format)"
    try:
        root = ET.fromstring(raw)
    except ET.ParseError as error:
        return False, f"invalid XML: {error}"
    if local_name(root.tag) != "TS":
        return False, f"XML root is <{local_name(root.tag)}>, not <TS>"
    return True, ""


def discover_files(
    *,
    directories: list[str],
    crowdin: str | None,
    project_root: Path,
) -> tuple[list[Path], list[dict[str, str]], set[Path], dict[str, Any]]:
    if bool(directories) == bool(crowdin):
        raise ToolError("Choose either one or more --directory values or exactly one --crowdin")

    candidates: set[Path] = set()
    crowdin_sources: set[Path] = set()
    selector: dict[str, Any]
    if crowdin:
        crowdin_path = resolve_input_path(crowdin, project_root)
        if not crowdin_path.is_file():
            raise ToolError(f"Crowdin config does not exist: {crowdin_path}")
        source_patterns = parse_crowdin_sources(crowdin_path)
        for source_pattern in source_patterns:
            normalized_pattern = source_pattern.replace("\\", "/").lstrip("/")
            try:
                matches = list(project_root.glob(normalized_pattern))
            except (ValueError, OSError) as error:
                raise ToolError(
                    f"Unsupported Crowdin source glob {source_pattern!r}: {error}"
                ) from error
            for match in matches:
                if match.is_file():
                    resolved = match.resolve()
                    crowdin_sources.add(resolved)
                    candidates.update(item.resolve() for item in match.parent.glob("*.ts"))
        if not crowdin_sources:
            raise ToolError(f"{crowdin_path}: source globs matched no existing files")
        selector = {
            "kind": "crowdin",
            "path": relative_display(crowdin_path, project_root),
            "source_patterns": source_patterns,
        }
    else:
        resolved_directories: list[str] = []
        for value in directories:
            path = resolve_input_path(value, project_root)
            if not path.exists():
                raise ToolError(f"Translation selector does not exist: {path}")
            resolved_directories.append(relative_display(path, project_root))
            if path.is_file():
                candidates.add(path)
            else:
                candidates.update(item.resolve() for item in path.rglob("*.ts"))
        selector = {"kind": "directories", "paths": resolved_directories}

    valid: list[Path] = []
    skipped: list[dict[str, str]] = []
    for candidate in sorted(candidates, key=lambda item: str(item).lower()):
        is_qt_ts, reason = probe_qt_ts(candidate)
        if is_qt_ts:
            valid.append(candidate)
        else:
            skipped.append({"path": relative_display(candidate, project_root), "reason": reason})
    if not valid:
        raise ToolError("The selector found no valid existing Qt TS XML files")
    return valid, skipped, crowdin_sources, selector


def _normalized_locations(
    message: ET.Element,
    *,
    ts_path: Path,
    project_root: Path,
    current_filename: str,
    line_by_filename: dict[str, int],
) -> tuple[list[dict[str, Any]], str]:
    locations: list[dict[str, Any]] = []
    for location in direct_children(message, "location"):
        filename = location.get("filename") or current_filename
        if filename:
            current_filename = filename
        raw_line = location.get("line") or ""
        line: int | None = None
        if raw_line:
            try:
                if raw_line.startswith(("+", "-")):
                    line = line_by_filename.get(filename, 0) + int(raw_line)
                else:
                    line = int(raw_line)
                line_by_filename[filename] = line
            except ValueError:
                line = None
        resolved_path = ""
        if filename:
            candidate = Path(filename)
            if not candidate.is_absolute():
                candidate = ts_path.parent / candidate
            resolved_path = relative_display(candidate.resolve(), project_root)
        locations.append(
            {
                "filename": filename,
                "line": line,
                "raw_line": raw_line,
                "resolved_path": resolved_path,
            }
        )
    return locations, current_filename


def _decode_c_string(token: str) -> str:
    try:
        value = ast.literal_eval(token)
        return value if isinstance(value, str) else ""
    except (ValueError, SyntaxError):
        return token[1:-1]


def engineering_source_from_code(
    message_id: str, locations: list[dict[str, Any]], project_root: Path
) -> tuple[str, str]:
    if not message_id:
        return "", ""
    for location in locations:
        resolved = location.get("resolved_path", "")
        if not resolved:
            continue
        source_path = Path(resolved)
        if not source_path.is_absolute():
            source_path = project_root / source_path
        if not source_path.is_file():
            continue
        try:
            lines = source_path.read_text(encoding="utf-8-sig", errors="replace").splitlines()
        except OSError:
            continue
        expected = location.get("line")
        centers: list[int] = []
        if isinstance(expected, int) and expected > 0:
            centers.append(min(expected - 1, len(lines) - 1))
        centers.extend(index for index, line in enumerate(lines) if message_id in line)
        for center in dict.fromkeys(centers):
            if center < 0:
                continue
            start = max(0, center - 12)
            comment_tokens: list[str] = []
            for line in lines[start : center + 1]:
                match = re.search(r'//%\s*("(?:\\.|[^"\\])*")', line)
                if match:
                    comment_tokens.append(match.group(1))
            if comment_tokens:
                return "".join(_decode_c_string(token) for token in comment_tokens), str(
                    source_path
                )
    return "", ""


def parse_ts_file(
    path: Path,
    project_root: Path,
    *,
    inspect_engineering_sources: bool = True,
) -> dict[str, Any]:
    text_file = read_utf8(path)
    try:
        root = ET.fromstring(text_file.raw)
    except ET.ParseError as error:
        raise ToolError(f"{path}: invalid XML: {error}") from error
    if local_name(root.tag) != "TS":
        raise ToolError(f"{path}: XML root is not <TS>")

    spans = lexical_message_spans(text_file.text)
    xml_messages: list[tuple[str, ET.Element]] = []
    for context in root:
        if local_name(context.tag) != "context":
            continue
        context_name = element_text(direct_child(context, "name"))
        for message in context:
            if local_name(message.tag) == "message":
                xml_messages.append((context_name, message))
    if len(spans) != len(xml_messages):
        raise ToolError(
            f"{path}: XML/lexical message count mismatch ({len(xml_messages)} vs {len(spans)})"
        )

    file_display = relative_display(path, project_root)
    occurrence_counts: Counter[tuple[str, str, str, str]] = Counter()
    entries: list[dict[str, Any]] = []
    current_filename = ""
    line_by_filename: dict[str, int] = {}

    for message_index, ((context_name, message), span) in enumerate(zip(xml_messages, spans)):
        source = element_text(direct_child(message, "source"))
        comment = element_text(direct_child(message, "comment"))
        extra_comment = element_text(direct_child(message, "extracomment"))
        translator_comment = element_text(direct_child(message, "translatorcomment"))
        message_id = message.get("id") or ""
        numerus = message.get("numerus") == "yes"
        translation_element = direct_child(message, "translation")
        translation_type = (
            translation_element.get("type", "") if translation_element is not None else ""
        )
        forms = (
            [element_text(item) for item in direct_children(translation_element, "numerusform")]
            if translation_element is not None
            else []
        )
        translation = (
            ""
            if forms
            else element_text(translation_element)
        )
        nested_names = (
            [local_name(item.tag) for item in translation_element]
            if translation_element is not None
            else []
        )
        unsupported_nested = (
            any(name != "numerusform" for name in nested_names)
            or (not numerus and bool(nested_names))
        )
        locations, current_filename = _normalized_locations(
            message,
            ts_path=path,
            project_root=project_root,
            current_filename=current_filename,
            line_by_filename=line_by_filename,
        )
        engineering_source = source
        engineering_source_origin = "ts-source" if source else ""
        if message_id and not engineering_source and inspect_engineering_sources:
            engineering_source, origin_path = engineering_source_from_code(
                message_id, locations, project_root
            )
            if engineering_source:
                engineering_source_origin = f"code-comment:{relative_display(Path(origin_path), project_root)}"

        identity_kind = "id" if message_id else "source"
        identity_value = message_id if message_id else source
        occurrence_base = (identity_kind, context_name, identity_value, comment)
        occurrence = occurrence_counts[occurrence_base]
        occurrence_counts[occurrence_base] += 1
        message_key_data = {
            "kind": identity_kind,
            "context": "" if message_id else context_name,
            "value": identity_value,
            "comment": "" if message_id else comment,
            "occurrence": occurrence,
        }
        message_key = canonical_json(message_key_data)
        entry_id = sha256_text(f"{file_display}\0{message_index}\0{message_key}")[:24]

        translation_span = span["translation"]
        raw_translation_inner = ""
        translation_open_tag = ""
        translation_self_closing = False
        if translation_span is not None:
            translation_open_tag = translation_span["open_tag"]
            translation_self_closing = bool(translation_span["self_closing"])
            if not translation_self_closing:
                raw_translation_inner = text_file.text[
                    translation_span["inner_start"] : translation_span["inner_end"]
                ]

        active = translation_type not in HISTORICAL_TYPES
        unfinished = active and translation_type == "unfinished"
        has_translation_content = bool(translation.strip()) or any(
            form.strip() for form in forms
        )
        entry_review_layer = review_layer(
            active=active,
            unfinished=unfinished,
            has_translation_content=has_translation_content,
        )
        entries.append(
            {
                "entry_id": entry_id,
                "file": file_display,
                "message_index": message_index,
                "message_key": message_key,
                "context": context_name,
                "id": message_id,
                "source": source,
                "engineering_source": engineering_source,
                "engineering_source_origin": engineering_source_origin,
                "comment": comment,
                "extracomment": extra_comment,
                "translatorcomment": translator_comment,
                "locations": locations,
                "numerus": numerus,
                "translation_type": translation_type,
                "translation": translation,
                "forms": forms,
                "active": active,
                "unfinished": unfinished,
                "has_translation_content": has_translation_content,
                "review_layer": entry_review_layer,
                "empty_finished": entry_review_layer == "finished"
                and not has_translation_content,
                "translation_exists": translation_element is not None,
                "translation_self_closing": translation_self_closing,
                "translation_open_tag": translation_open_tag,
                "translation_hash": sha256_text(raw_translation_inner),
                "unsupported_nested_translation_xml": unsupported_nested,
                "source_placeholders": sorted(PLACEHOLDER_RE.findall(engineering_source)),
                "translation_placeholders": (
                    [sorted(PLACEHOLDER_RE.findall(form)) for form in forms]
                    if forms
                    else sorted(PLACEHOLDER_RE.findall(translation))
                ),
                "write_allowed": bool(
                    translation_element is not None
                    and not translation_self_closing
                    and active
                    and (not message_id or engineering_source)
                    and not unsupported_nested
                ),
            }
        )

    opening_hash, opening_count = translation_opening_snapshot(text_file.text)
    return {
        "path": path.resolve(),
        "display_path": file_display,
        "directory": relative_display(path.parent, project_root),
        "language": normalize_locale(root.get("language")),
        "source_language": normalize_locale(root.get("sourcelanguage")),
        "sha256": sha256_bytes(text_file.raw),
        "bom": text_file.bom,
        "newline": "CRLF" if text_file.newline == "\r\n" else "LF",
        "translation_opening_tags_hash": opening_hash,
        "translation_opening_tag_count": opening_count,
        "message_count": len(entries),
        "entries": entries,
    }


def infer_modules(
    parsed_files: list[dict[str, Any]],
    *,
    explicit_source_locale: str | None,
    crowdin_source_paths: set[Path],
) -> dict[str, str]:
    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for parsed in parsed_files:
        grouped[parsed["directory"]].append(parsed)

    module_sources: dict[str, str] = {}
    explicit = normalize_locale(explicit_source_locale)
    for module, files in grouped.items():
        by_language: dict[str, list[dict[str, Any]]] = defaultdict(list)
        for parsed in files:
            by_language[parsed["language"]].append(parsed)
        duplicates = [language for language, items in by_language.items() if len(items) > 1]
        if duplicates:
            raise ToolError(
                f"{module}: duplicate TS@language values are unsupported: {', '.join(duplicates)}"
            )
        languages = set(by_language)
        candidates: set[str] = set()
        if explicit:
            if explicit not in languages:
                raise ToolError(f"{module}: explicit source locale {explicit} is not present")
            candidates.add(explicit)
        elif crowdin_source_paths:
            for parsed in files:
                if parsed["path"] in crowdin_source_paths:
                    candidates.add(parsed["language"])
        if not candidates:
            for parsed in files:
                source_language = parsed["source_language"]
                if source_language and source_language in languages:
                    candidates.add(source_language)
        if not candidates and "en_US" in languages:
            candidates.add("en_US")
        if len(candidates) != 1:
            rendered = ", ".join(sorted(candidates)) or "none"
            raise ToolError(
                f"{module}: source locale is ambiguous ({rendered}); pass --source-locale"
            )
        module_sources[module] = next(iter(candidates))
    return module_sources


def _source_issue_candidates(source: str) -> list[dict[str, str]]:
    visible = re.sub(r"<[^>]+>", "", source)
    issues: list[dict[str, str]] = []
    patterns = [
        (
            re.compile(r"\ban\s+(newer|new|user|unique|unified|universal|one)\b", re.IGNORECASE),
            "article",
            "Likely use 'a' before this consonant-sound word.",
        ),
        (
            re.compile(r"\b(\w+)\s+\1\b", re.IGNORECASE),
            "duplicate-word",
            "Repeated adjacent word.",
        ),
        (
            re.compile(r"[ \t]{2,}"),
            "spacing",
            "Repeated horizontal whitespace.",
        ),
        (
            re.compile(r"\s+[,.!?;:]"),
            "punctuation-spacing",
            "Unexpected space before punctuation.",
        ),
    ]
    for pattern, code, message in patterns:
        match = pattern.search(visible)
        if match:
            issues.append(
                {
                    "code": code,
                    "message": message,
                    "matched_text": match.group(0),
                }
            )
    for opening, closing, label in [("(", ")", "parentheses"), ("[", "]", "brackets")]:
        if visible.count(opening) != visible.count(closing):
            issues.append(
                {
                    "code": "unbalanced-delimiter",
                    "message": f"Unbalanced {label}.",
                    "matched_text": f"{opening}{closing}",
                }
            )
    return issues


def _read_source_snippet(
    location: dict[str, Any], project_root: Path, radius: int = 6
) -> tuple[str, str]:
    resolved = location.get("resolved_path", "")
    if not resolved:
        return "", ""
    path = Path(resolved)
    if not path.is_absolute():
        path = project_root / path
    if not path.is_file():
        return "", ""
    try:
        lines = path.read_text(encoding="utf-8-sig", errors="replace").splitlines()
    except OSError:
        return "", ""
    line = location.get("line")
    if not isinstance(line, int) or line < 1:
        return "", relative_display(path, project_root)
    start = max(0, line - 1 - radius)
    end = min(len(lines), line + radius)
    rendered = "\n".join(f"{index + 1}: {lines[index]}" for index in range(start, end))
    return rendered, relative_display(path, project_root)


def _localization_candidate(entry: dict[str, Any], project_root: Path) -> dict[str, Any] | None:
    source = entry["engineering_source"]
    placeholders = ORDINARY_NUMBERED_PLACEHOLDER_RE.findall(source)
    if not placeholders or not NUMERIC_SEMANTIC_RE.search(source):
        return None
    snippet = ""
    code_path = ""
    for location in entry["locations"]:
        snippet, code_path = _read_source_snippet(location, project_root)
        if snippet:
            break
    if snippet and LOCALE_EVIDENCE_RE.search(snippet):
        return None
    evidence = FORMATTING_EVIDENCE_RE.findall(snippet)
    return {
        "entry_id": entry["entry_id"],
        "file": entry["file"],
        "context": entry["context"],
        "id": entry["id"],
        "source": source,
        "placeholders": [f"%{item}" for item in placeholders],
        "location": entry["locations"][0] if entry["locations"] else {},
        "code_path": code_path,
        "code_snippet": snippet,
        "formatting_evidence": evidence,
        "likely_invariant": bool(LIKELY_INVARIANT_RE.search(source)),
        "reason": (
            "Numeric/date/time semantics use an ordinary placeholder and nearby code shows "
            "non-locale-qualified formatting."
            if evidence
            else "Numeric/date/time semantics use an ordinary placeholder; inspect runtime formatting."
        ),
    }


def _build_batches(entries: list[dict[str, Any]], batch_size: int) -> list[list[str]]:
    grouped: list[list[dict[str, Any]]] = []
    current_key: tuple[str, str] | None = None
    current_group: list[dict[str, Any]] = []
    for entry in entries:
        key = (entry["file"], entry["context"])
        if current_key is not None and key != current_key:
            grouped.append(current_group)
            current_group = []
        current_key = key
        current_group.append(entry)
    if current_group:
        grouped.append(current_group)

    batches: list[list[str]] = []
    current: list[str] = []
    for group in grouped:
        group_ids = [entry["entry_id"] for entry in group]
        if len(group_ids) > batch_size:
            if current:
                batches.append(current)
                current = []
            for offset in range(0, len(group_ids), batch_size):
                batches.append(group_ids[offset : offset + batch_size])
            continue
        if current and len(current) + len(group_ids) > batch_size:
            batches.append(current)
            current = []
        current.extend(group_ids)
        if len(current) == batch_size:
            batches.append(current)
            current = []
    if current:
        batches.append(current)
    return batches


def _summaries(parsed_files: list[dict[str, Any]]) -> dict[str, dict[str, int]]:
    summary: dict[str, dict[str, int]] = {}
    for parsed in parsed_files:
        language = parsed["language"]
        row = summary.setdefault(
            language,
            {
                "files": 0,
                "messages": 0,
                "active": 0,
                "unfinished": 0,
                "unfinished_empty": 0,
                "unfinished_nonempty": 0,
                "nonempty_unfinished": 0,
                "finished": 0,
                "finished_empty": 0,
                "numerus": 0,
                "id_based": 0,
                "id_without_engineering_source": 0,
            },
        )
        row["files"] += 1
        row["messages"] += len(parsed["entries"])
        for entry in parsed["entries"]:
            if not entry["active"]:
                continue
            row["active"] += 1
            row["unfinished"] += int(entry["unfinished"])
            row["unfinished_empty"] += int(
                entry["review_layer"] == "unfinished-empty"
            )
            row["unfinished_nonempty"] += int(
                entry["review_layer"] == "unfinished-nonempty"
            )
            row["nonempty_unfinished"] += int(
                entry["review_layer"] == "unfinished-nonempty"
            )
            row["finished"] += int(entry["review_layer"] == "finished")
            row["finished_empty"] += int(entry["empty_finished"])
            row["numerus"] += int(entry["numerus"])
            row["id_based"] += int(bool(entry["id"]))
            row["id_without_engineering_source"] += int(
                bool(entry["id"]) and not entry["engineering_source"]
            )
    return dict(sorted(summary.items()))


def _inventory_counts(entries: list[dict[str, Any]]) -> dict[str, Any]:
    active_entries = [entry for entry in entries if entry["active"]]
    unfinished_empty = sum(
        entry["review_layer"] == "unfinished-empty" for entry in active_entries
    )
    unfinished_nonempty = sum(
        entry["review_layer"] == "unfinished-nonempty" for entry in active_entries
    )
    finished = sum(entry["review_layer"] == "finished" for entry in active_entries)
    vanished = sum(entry["translation_type"] == "vanished" for entry in entries)
    obsolete = sum(entry["translation_type"] == "obsolete" for entry in entries)
    return {
        "messages_total": len(entries),
        "active_total": len(active_entries),
        "historical_total": vanished + obsolete,
        "scopes": {
            "unfinished-empty": unfinished_empty,
            "unfinished-nonempty": unfinished_nonempty,
            "unfinished": unfinished_empty + unfinished_nonempty,
            "finished": finished,
            "all": len(active_entries),
        },
        "historical": {
            "vanished": vanished,
            "obsolete": obsolete,
        },
        "finished_empty": sum(entry["empty_finished"] for entry in active_entries),
        "numerus_total": sum(entry["numerus"] for entry in entries),
        "numerus_active": sum(entry["numerus"] for entry in active_entries),
        "id_based_total": sum(bool(entry["id"]) for entry in entries),
        "id_based_active": sum(bool(entry["id"]) for entry in active_entries),
    }


def build_inventory(
    *,
    directories: list[str],
    crowdin: str | None,
    project_root: Path,
    locales: list[str],
) -> dict[str, Any]:
    paths, skipped_files, _crowdin_sources, selector = discover_files(
        directories=directories,
        crowdin=crowdin,
        project_root=project_root,
    )
    parsed_files = [
        parse_ts_file(
            path,
            project_root,
            inspect_engineering_sources=False,
        )
        for path in paths
    ]
    present_locales = {parsed["language"] for parsed in parsed_files}
    requested = {
        normalize_locale(item)
        for raw in locales
        for item in raw.split(",")
        if item.strip()
    }
    if not requested or "all" in {item.lower() for item in requested}:
        target_locales = present_locales
    else:
        missing = requested - present_locales
        if missing:
            raise ToolError(f"Requested locales are not present: {', '.join(sorted(missing))}")
        target_locales = requested

    selected_files = [
        parsed for parsed in parsed_files if parsed["language"] in target_locales
    ]
    file_rows = [
        {
            "path": parsed["display_path"],
            "directory": parsed["directory"],
            "language": parsed["language"],
            **_inventory_counts(parsed["entries"]),
        }
        for parsed in selected_files
    ]
    all_entries = [
        entry for parsed in selected_files for entry in parsed["entries"]
    ]
    by_language = {
        language: _inventory_counts(
            [
                entry
                for parsed in selected_files
                if parsed["language"] == language
                for entry in parsed["entries"]
            ]
        )
        for language in sorted(target_locales)
    }
    return {
        "inventory_version": INVENTORY_VERSION,
        "project_root": str(project_root.resolve()),
        "selector": selector,
        "requested_locales": sorted(target_locales),
        "file_count": len(selected_files),
        "directory_count": len({parsed["directory"] for parsed in selected_files}),
        "files": file_rows,
        "by_language": by_language,
        "totals": _inventory_counts(all_entries),
        "skipped_files": skipped_files,
        "definitions": {
            "messages_total": "All <message> elements, including vanished and obsolete.",
            "scopes.all": "All active messages; excludes vanished and obsolete.",
            "unfinished-empty": (
                "Active explicit-unfinished messages with no non-whitespace translation content."
            ),
            "unfinished-nonempty": (
                "Active explicit-unfinished messages with candidate translation content."
            ),
            "finished": "Active messages without type=unfinished.",
        },
        "safety": {
            "read_only": True,
            "source_code_inspected": False,
            "language_judgment_performed": False,
            "lupdate_invoked": False,
            "lrelease_invoked": False,
        },
    }


def _alignment_issues(parsed_files: list[dict[str, Any]]) -> list[dict[str, Any]]:
    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for parsed in parsed_files:
        grouped[parsed["directory"]].append(parsed)
    issues: list[dict[str, Any]] = []
    for module, files in grouped.items():
        source_files = [item for item in files if item["is_source_file"]]
        if len(source_files) != 1:
            continue
        source_keys = {
            entry["message_key"] for entry in source_files[0]["entries"] if entry["active"]
        }
        for target in files:
            if target is source_files[0]:
                continue
            target_keys = {entry["message_key"] for entry in target["entries"] if entry["active"]}
            missing = sorted(source_keys - target_keys)
            extra = sorted(target_keys - source_keys)
            if missing or extra:
                issues.append(
                    {
                        "module": module,
                        "language": target["language"],
                        "missing_count": len(missing),
                        "extra_count": len(extra),
                        "missing_examples": missing[:10],
                        "extra_examples": extra[:10],
                    }
                )
    return issues


def _consistency_issues(entries: list[dict[str, Any]]) -> list[dict[str, Any]]:
    groups: dict[tuple[str, str, str, str], list[dict[str, Any]]] = defaultdict(list)
    for entry in entries:
        if (
            entry["active"]
            and entry["review_layer"] in {"unfinished-nonempty", "finished"}
            and not entry["numerus"]
            and entry["engineering_source"]
            and entry["translation"].strip()
        ):
            groups[
                (
                    entry["language"],
                    entry["engineering_source"],
                    entry["comment"],
                    entry["review_layer"],
                )
            ].append(entry)
    issues: list[dict[str, Any]] = []
    for (language, source, comment, layer), grouped_entries in groups.items():
        variants = sorted({entry["translation"] for entry in grouped_entries})
        if len(variants) > 1:
            issues.append(
                {
                    "language": language,
                    "review_layer": layer,
                    "source": source,
                    "comment": comment,
                    "translations": variants,
                    "entry_ids": [entry["entry_id"] for entry in grouped_entries],
                    "note": (
                        "Accepted-translation inconsistency; confirm contexts are semantically equivalent."
                        if layer == "finished"
                        else "Unfinished-candidate inconsistency; review before completion."
                    ),
                }
            )
    return issues


def compute_manifest_hash(manifest: dict[str, Any]) -> str:
    copy = dict(manifest)
    copy.pop("manifest_hash", None)
    return sha256_json(copy)


def build_manifest(
    *,
    directories: list[str],
    crowdin: str | None,
    project_root: Path,
    scope: str,
    locales: list[str],
    source_locale: str | None,
    batch_size: int,
) -> dict[str, Any]:
    if scope not in SCOPES:
        raise ToolError(f"Unsupported scope: {scope}")
    paths, skipped_files, crowdin_sources, selector = discover_files(
        directories=directories,
        crowdin=crowdin,
        project_root=project_root,
    )
    parsed_files = [parse_ts_file(path, project_root) for path in paths]
    module_sources = infer_modules(
        parsed_files,
        explicit_source_locale=source_locale,
        crowdin_source_paths=crowdin_sources,
    )

    present_locales = {parsed["language"] for parsed in parsed_files}
    requested = {
        normalize_locale(item)
        for raw in locales
        for item in raw.split(",")
        if item.strip()
    }
    if not requested or "all" in {item.lower() for item in requested}:
        target_locales = present_locales
    else:
        missing = requested - present_locales
        if missing:
            raise ToolError(f"Requested locales are not present: {', '.join(sorted(missing))}")
        target_locales = requested

    all_entries: list[dict[str, Any]] = []
    files_manifest: list[dict[str, Any]] = []
    for parsed in parsed_files:
        module_source = module_sources[parsed["directory"]]
        parsed["module_source_language"] = module_source
        parsed["is_source_file"] = parsed["language"] == module_source
        for entry in parsed["entries"]:
            entry["language"] = parsed["language"]
            entry["module"] = parsed["directory"]
            entry["module_source_language"] = module_source
            entry["is_source_message"] = parsed["is_source_file"]
            entry["in_target_locale"] = parsed["language"] in target_locales
            entry["in_scope"] = bool(
                entry["active"]
                and entry["in_target_locale"]
                and entry_matches_scope(entry, scope)
            )
            all_entries.append(entry)
        files_manifest.append(
            {
                "path": parsed["display_path"],
                "directory": parsed["directory"],
                "language": parsed["language"],
                "source_language": parsed["source_language"],
                "module_source_language": module_source,
                "is_source_file": parsed["is_source_file"],
                "sha256": parsed["sha256"],
                "bom": parsed["bom"],
                "newline": parsed["newline"],
                "translation_opening_tags_hash": parsed["translation_opening_tags_hash"],
                "translation_opening_tag_count": parsed["translation_opening_tag_count"],
                "message_count": parsed["message_count"],
            }
        )

    source_issues: list[dict[str, Any]] = []
    localization_candidates: list[dict[str, Any]] = []
    for entry in all_entries:
        if not entry["is_source_message"] or not entry["active"]:
            continue
        if not entry_matches_scope(entry, scope):
            continue
        if entry["engineering_source"]:
            for issue in _source_issue_candidates(entry["engineering_source"]):
                source_issues.append(
                    {
                        **issue,
                        "entry_id": entry["entry_id"],
                        "file": entry["file"],
                        "context": entry["context"],
                        "id": entry["id"],
                        "review_layer": entry["review_layer"],
                        "source": entry["engineering_source"],
                        "locations": entry["locations"],
                    }
                )
            localization = _localization_candidate(entry, project_root)
            if localization:
                localization_candidates.append(localization)

    scoped_entries = [entry for entry in all_entries if entry["in_scope"]]
    skipped_ids = [
        {
            "entry_id": entry["entry_id"],
            "file": entry["file"],
            "language": entry["language"],
            "review_layer": entry["review_layer"],
            "id": entry["id"],
            "locations": entry["locations"],
        }
        for entry in scoped_entries
        if entry["id"] and not entry["engineering_source"]
    ]
    selected_by_review_layer = {
        layer: sum(entry["review_layer"] == layer for entry in scoped_entries)
        for layer in REVIEW_LAYERS
    }
    selected_by_language_and_review_layer = {
        language: {
            layer: sum(
                entry["language"] == language and entry["review_layer"] == layer
                for entry in scoped_entries
            )
            for layer in REVIEW_LAYERS
        }
        for language in sorted(target_locales)
    }
    manifest: dict[str, Any] = {
        "manifest_version": MANIFEST_VERSION,
        "project_root": str(project_root.resolve()),
        "selector": selector,
        "scope": scope,
        "requested_locales": sorted(target_locales),
        "module_source_locales": dict(sorted(module_sources.items())),
        "batch_size": batch_size,
        "files": files_manifest,
        "entries": all_entries,
        "batches": _build_batches(scoped_entries, batch_size),
        "summary_by_language": _summaries(parsed_files),
        "selected_file_count": len(parsed_files),
        "selected_directory_count": len({parsed["directory"] for parsed in parsed_files}),
        "selected_entry_count": len(scoped_entries),
        "selected_by_review_layer": selected_by_review_layer,
        "selected_by_language_and_review_layer": (
            selected_by_language_and_review_layer
        ),
        "skipped_files": skipped_files,
        "skipped_ids": skipped_ids,
        "alignment_issues": _alignment_issues(parsed_files),
        "consistency_issues": _consistency_issues(all_entries),
        "source_issues": source_issues,
        "localization_candidates": localization_candidates,
    }
    manifest["manifest_hash"] = compute_manifest_hash(manifest)
    return manifest


class FragmentInspector(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.events: list[tuple[str, str, tuple[str, ...], tuple[tuple[str, str], ...]]] = []
        self.stack: list[str] = []
        self.errors: list[str] = []

    def _attributes(
        self, attrs: list[tuple[str, str | None]]
    ) -> tuple[tuple[str, ...], tuple[tuple[str, str], ...]]:
        names = tuple(sorted(name.lower() for name, _ in attrs))
        critical = tuple(
            sorted(
                (name.lower(), value or "")
                for name, value in attrs
                if name.lower() in CRITICAL_HTML_ATTRIBUTES
            )
        )
        return names, critical

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        tag = tag.lower()
        names, critical = self._attributes(attrs)
        kind = "void" if tag in VOID_HTML_TAGS else "start"
        self.events.append((kind, tag, names, critical))
        if tag not in VOID_HTML_TAGS:
            self.stack.append(tag)

    def handle_startendtag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        tag = tag.lower()
        names, critical = self._attributes(attrs)
        self.events.append(("void", tag, names, critical))

    def handle_endtag(self, tag: str) -> None:
        tag = tag.lower()
        self.events.append(("end", tag, (), ()))
        if not self.stack or self.stack[-1] != tag:
            self.errors.append(f"unexpected closing tag </{tag}>")
            return
        self.stack.pop()

    def close(self) -> None:
        super().close()
        if self.stack:
            self.errors.append(f"unclosed tags: {', '.join(self.stack)}")


def inspect_fragment(value: str) -> FragmentInspector:
    inspector = FragmentInspector()
    try:
        inspector.feed(value)
        inspector.close()
    except Exception as error:  # HTMLParser can surface malformed declarations.
        inspector.errors.append(str(error))
    return inspector


def mnemonic_count(value: str) -> int:
    visible = re.sub(r"<[^>]+>", "", value) if RICH_TAG_RE.search(value) else value
    count = 0
    index = 0
    while index < len(visible):
        if visible[index] != "&":
            index += 1
            continue
        if index + 1 < len(visible) and visible[index + 1] == "&":
            index += 2
            continue
        if index + 1 < len(visible) and not visible[index + 1].isspace():
            count += 1
        index += 1
    return count


def terminal_group(value: str) -> str:
    visible = re.sub(r"<[^>]+>", "", value).rstrip()
    groups = [
        ("ellipsis", ("...", "……", "…")),
        ("question", ("?", "？")),
        ("exclamation", ("!", "！")),
        ("colon", (":", "：")),
        ("period", (".", "。")),
        ("semicolon", (";", "；")),
    ]
    for name, suffixes in groups:
        if visible.endswith(suffixes):
            return name
    return ""


def edge_whitespace(value: str) -> tuple[str, str]:
    leading = re.match(r"^\s*", value)
    trailing = re.search(r"\s*$", value)
    return (leading.group(0) if leading else "", trailing.group(0) if trailing else "")


def validate_translation_text(source: str, target: str) -> dict[str, list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    if Counter(PLACEHOLDER_RE.findall(source)) != Counter(PLACEHOLDER_RE.findall(target)):
        errors.append("placeholder multiset differs from engineering source")

    source_filters = Counter(FILTER_RE.findall(source))
    target_filters = Counter(FILTER_RE.findall(target))
    if source_filters != target_filters:
        errors.append("file-filter/wildcard tokens differ from engineering source")

    source_is_rich = bool(RICH_TAG_RE.search(source))
    target_is_rich = bool(RICH_TAG_RE.search(target))
    if source_is_rich or target_is_rich:
        source_fragment = inspect_fragment(source)
        target_fragment = inspect_fragment(target)
        if source_fragment.errors:
            warnings.append(f"engineering source rich text is malformed: {source_fragment.errors}")
        if target_fragment.errors:
            errors.append(f"translation rich text is malformed: {target_fragment.errors}")
        if source_fragment.events != target_fragment.events:
            errors.append("rich-text tag order, nesting, attributes, or critical values differ")

    if edge_whitespace(source) != edge_whitespace(target):
        warnings.append("leading/trailing whitespace differs")
    if source.count("\n") != target.count("\n"):
        warnings.append("newline count differs")
    if mnemonic_count(source) != mnemonic_count(target):
        warnings.append("mnemonic ampersand count differs")
    source_terminal = terminal_group(source)
    target_terminal = terminal_group(target)
    if source_terminal and source_terminal != target_terminal:
        warnings.append(
            f"functional terminal punctuation differs ({source_terminal} vs {target_terminal or 'none'})"
        )
    if Counter(SHORTCUT_RE.findall(source)) != Counter(SHORTCUT_RE.findall(target)):
        warnings.append("shortcut tokens differ")
    return {"errors": errors, "warnings": warnings}


def validate_entry(entry: dict[str, Any]) -> list[dict[str, str]]:
    findings: list[dict[str, str]] = []
    source = entry["engineering_source"]
    if entry["id"] and not source:
        findings.append(
            {
                "severity": "info",
                "code": "id-without-engineering-source",
                "message": "ID-based message is intentionally unchanged.",
            }
        )
        return findings
    if not entry["translation_exists"]:
        findings.append(
            {
                "severity": "error",
                "code": "missing-translation-element",
                "message": "Message has no existing <translation> element.",
            }
        )
        return findings
    if entry["translation_self_closing"]:
        findings.append(
            {
                "severity": "error",
                "code": "self-closing-translation",
                "message": "Self-closing <translation/> cannot be patched without changing its tag.",
            }
        )
        return findings
    if entry["unsupported_nested_translation_xml"]:
        findings.append(
            {
                "severity": "error",
                "code": "unsupported-nested-xml",
                "message": "Translation contains unsupported nested XML.",
            }
        )
        return findings

    targets = entry["forms"] if entry["numerus"] else [entry["translation"]]
    if entry["numerus"] and not targets:
        findings.append(
            {
                "severity": "error",
                "code": "missing-plural-forms",
                "message": "Numerus message has no existing <numerusform> elements.",
            }
        )
        return findings
    if entry["review_layer"] == "unfinished-empty":
        findings.append(
            {
                "severity": "warning",
                "code": "unfinished-empty",
                "message": "Explicit-unfinished translation has no candidate content.",
            }
        )
        return findings
    if entry["empty_finished"]:
        findings.append(
            {
                "severity": "error",
                "code": "empty-finished",
                "message": "Translation is marked finished but has no content.",
            }
        )
        return findings
    for index, target in enumerate(targets):
        suffix = f" (form {index + 1})" if entry["numerus"] else ""
        if not target.strip():
            findings.append(
                {
                    "severity": (
                        "error" if entry["review_layer"] == "finished" else "warning"
                    ),
                    "code": (
                        "empty-finished-form"
                        if entry["review_layer"] == "finished"
                        else "unfinished-partial-empty"
                    ),
                    "message": (
                        "Finished translation form is empty"
                        if entry["review_layer"] == "finished"
                        else "Unfinished candidate has an empty form"
                    )
                    + suffix,
                }
            )
            continue
        result = validate_translation_text(source, target)
        for message in result["errors"]:
            findings.append(
                {
                    "severity": "error",
                    "code": "translation-structure",
                    "message": message + suffix,
                }
            )
        for message in result["warnings"]:
            findings.append(
                {
                    "severity": "warning",
                    "code": "translation-warning",
                    "message": message + suffix,
                }
            )
    return findings


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as error:
        raise ToolError(f"Unable to load JSON {path}: {error}") from error
    if not isinstance(value, dict):
        raise ToolError(f"{path}: expected a JSON object")
    return value


def load_manifest(path: Path) -> dict[str, Any]:
    manifest = load_json(path)
    if manifest.get("manifest_version") != MANIFEST_VERSION:
        raise ToolError(f"{path}: unsupported manifest version")
    actual_hash = compute_manifest_hash(manifest)
    if manifest.get("manifest_hash") != actual_hash:
        raise ToolError(f"{path}: manifest_hash does not match manifest content")
    return manifest


def validate_manifest(manifest: dict[str, Any], run_lcheck: bool = True) -> dict[str, Any]:
    project_root = Path(manifest["project_root"])
    file_manifest_by_path = {item["path"]: item for item in manifest["files"]}
    current_entries: dict[str, dict[str, Any]] = {}
    findings: list[dict[str, Any]] = []
    file_results: list[dict[str, Any]] = []

    for display_path, file_manifest in file_manifest_by_path.items():
        path = Path(display_path)
        if not path.is_absolute():
            path = project_root / path
        if not path.is_file():
            findings.append(
                {
                    "severity": "error",
                    "code": "missing-file",
                    "file": display_path,
                    "message": "Selected TS file no longer exists.",
                }
            )
            continue
        try:
            parsed = parse_ts_file(path, project_root)
        except ToolError as error:
            findings.append(
                {
                    "severity": "error",
                    "code": "parse-error",
                    "file": display_path,
                    "message": str(error),
                }
            )
            continue
        if (
            parsed["translation_opening_tags_hash"]
            != file_manifest["translation_opening_tags_hash"]
            or parsed["translation_opening_tag_count"]
            != file_manifest["translation_opening_tag_count"]
        ):
            findings.append(
                {
                    "severity": "error",
                    "code": "review-state-opening-tags-changed",
                    "file": display_path,
                    "message": "Ordered <translation ...> opening-tag snapshot changed.",
                }
            )
        file_results.append(
            {
                "file": display_path,
                "current_sha256": parsed["sha256"],
                "matches_original_sha256": parsed["sha256"] == file_manifest["sha256"],
            }
        )
        for entry in parsed["entries"]:
            current_entries[entry["entry_id"]] = entry

    for original in manifest["entries"]:
        if not original["in_scope"]:
            continue
        current = current_entries.get(original["entry_id"])
        if current is None:
            findings.append(
                {
                    "severity": "error",
                    "code": "message-identity-changed",
                    "file": original["file"],
                    "entry_id": original["entry_id"],
                    "message": "Manifest message cannot be matched in current file.",
                }
            )
            continue
        for finding in validate_entry(current):
            findings.append(
                {
                    **finding,
                    "file": original["file"],
                    "entry_id": original["entry_id"],
                    "language": original["language"],
                    "context": original["context"],
                    "id": original["id"],
                    "review_layer": original["review_layer"],
                    "current_review_layer": current["review_layer"],
                    "source": original["engineering_source"],
                }
            )

    lcheck_results: list[dict[str, Any]] = []
    lcheck_path = shutil.which("lcheck") if run_lcheck else None
    if lcheck_path:
        for file_manifest in manifest["files"]:
            path = Path(file_manifest["path"])
            if not path.is_absolute():
                path = project_root / path
            command = [lcheck_path]
            if manifest["scope"] in {"finished", "all"}:
                command.append("-check-finished")
            command.append(str(path))
            completed = subprocess.run(command, text=True, capture_output=True, check=False)
            lcheck_results.append(
                {
                    "file": file_manifest["path"],
                    "returncode": completed.returncode,
                    "stdout": completed.stdout,
                    "stderr": completed.stderr,
                }
            )

    counts = Counter(finding["severity"] for finding in findings)
    findings_by_review_layer: dict[str, dict[str, int]] = {}
    for finding in findings:
        layer = finding.get("review_layer", "file")
        row = findings_by_review_layer.setdefault(
            layer, {"error": 0, "warning": 0, "info": 0}
        )
        row[finding["severity"]] += 1
    return {
        "valid": counts["error"] == 0,
        "error_count": counts["error"],
        "warning_count": counts["warning"],
        "info_count": counts["info"],
        "findings": findings,
        "findings_by_review_layer": findings_by_review_layer,
        "files": file_results,
        "lcheck": {
            "available": bool(lcheck_path),
            "path": lcheck_path or "",
            "results": lcheck_results,
        },
        "safety": {
            "translation_opening_tags_unchanged": not any(
                finding["code"] == "review-state-opening-tags-changed"
                for finding in findings
            ),
            "source_files_modified_by_tool": False,
            "lupdate_invoked": False,
            "lrelease_invoked": False,
        },
    }


def _validate_change(
    entry: dict[str, Any], change: dict[str, Any]
) -> tuple[list[str], list[str], list[str]]:
    if not entry["in_scope"]:
        raise ToolError(f"{entry['entry_id']}: entry is outside manifest scope/locale")
    if not entry["write_allowed"]:
        raise ToolError(f"{entry['entry_id']}: entry is not safe to patch")
    if entry["id"] and not entry["engineering_source"]:
        raise ToolError(f"{entry['entry_id']}: ID message has no engineering source")

    if entry["numerus"]:
        if "translation" in change or "forms" not in change:
            raise ToolError(f"{entry['entry_id']}: plural change requires only a forms array")
        forms = change["forms"]
        if not isinstance(forms, list) or not all(isinstance(item, str) for item in forms):
            raise ToolError(f"{entry['entry_id']}: forms must be an array of strings")
        if len(forms) != len(entry["forms"]):
            raise ToolError(
                f"{entry['entry_id']}: expected {len(entry['forms'])} existing plural forms, "
                f"got {len(forms)}"
            )
        targets = forms
    else:
        if "forms" in change or not isinstance(change.get("translation"), str):
            raise ToolError(f"{entry['entry_id']}: non-plural change requires only translation")
        targets = [change["translation"]]

    errors: list[str] = []
    warnings: list[str] = []
    for index, target in enumerate(targets):
        result = validate_translation_text(entry["engineering_source"], target)
        suffix = f" (form {index + 1})" if entry["numerus"] else ""
        errors.extend(message + suffix for message in result["errors"])
        warnings.extend(message + suffix for message in result["warnings"])
    if errors:
        raise ToolError(f"{entry['entry_id']}: " + "; ".join(errors))
    return targets, errors, warnings


def _construct_updated_file(
    text_file: TextFile,
    parsed: dict[str, Any],
    changes: list[tuple[dict[str, Any], dict[str, Any], list[str]]],
) -> bytes:
    spans = lexical_message_spans(text_file.text)
    replacements: list[tuple[int, int, str]] = []
    for entry, _change, targets in changes:
        try:
            message_span = spans[entry["message_index"]]
        except IndexError as error:
            raise ToolError(f"{entry['entry_id']}: message index is out of range") from error
        translation_span = message_span["translation"]
        if translation_span is None or translation_span["self_closing"]:
            raise ToolError(f"{entry['entry_id']}: existing translation span is not patchable")
        if entry["numerus"]:
            form_spans = message_span["forms"]
            if len(form_spans) != len(targets):
                raise ToolError(f"{entry['entry_id']}: lexical plural-form count changed")
            for form_span, target in zip(form_spans, targets):
                rendered = xml_escape_text(normalize_newlines(target, text_file.newline))
                replacements.append((form_span["inner_start"], form_span["inner_end"], rendered))
        else:
            if message_span["forms"]:
                raise ToolError(f"{entry['entry_id']}: unexpected plural child elements")
            rendered = xml_escape_text(normalize_newlines(targets[0], text_file.newline))
            replacements.append(
                (translation_span["inner_start"], translation_span["inner_end"], rendered)
            )

    updated = text_file.text
    for start, end, replacement in sorted(replacements, reverse=True):
        updated = updated[:start] + replacement + updated[end:]
    return encode_text_file(text_file, updated)


def _atomic_replace(path: Path, data: bytes, mode: int) -> None:
    temporary_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb", delete=False, dir=path.parent, prefix=f".{path.name}.", suffix=".tmp"
        ) as handle:
            handle.write(data)
            handle.flush()
            os.fsync(handle.fileno())
            temporary_path = Path(handle.name)
        os.chmod(temporary_path, stat.S_IMODE(mode))
        os.replace(temporary_path, path)
    finally:
        if temporary_path is not None and temporary_path.exists():
            temporary_path.unlink()


def apply_changes(
    manifest: dict[str, Any], changes_document: dict[str, Any], dry_run: bool
) -> dict[str, Any]:
    if changes_document.get("manifest_hash") != manifest["manifest_hash"]:
        raise ToolError("Change plan manifest_hash does not match the manifest")
    changes_value = changes_document.get("changes")
    if not isinstance(changes_value, list):
        raise ToolError("Change plan requires a changes array")

    manifest_entries = {entry["entry_id"]: entry for entry in manifest["entries"]}
    seen: set[str] = set()
    changes_by_file: dict[
        str, list[tuple[dict[str, Any], dict[str, Any], list[str]]]
    ] = defaultdict(list)
    warnings: list[dict[str, Any]] = []
    for change in changes_value:
        if not isinstance(change, dict) or not isinstance(change.get("entry_id"), str):
            raise ToolError("Every change requires a string entry_id")
        entry_id = change["entry_id"]
        if entry_id in seen:
            raise ToolError(f"Duplicate change entry_id: {entry_id}")
        seen.add(entry_id)
        entry = manifest_entries.get(entry_id)
        if entry is None:
            raise ToolError(f"Change entry_id is not present in manifest: {entry_id}")
        targets, _errors, entry_warnings = _validate_change(entry, change)
        for warning in entry_warnings:
            warnings.append(
                {
                    "entry_id": entry_id,
                    "file": entry["file"],
                    "review_layer": entry["review_layer"],
                    "message": warning,
                }
            )
        changes_by_file[entry["file"]].append((entry, change, targets))

    project_root = Path(manifest["project_root"])
    file_manifest_by_path = {item["path"]: item for item in manifest["files"]}
    originals: dict[Path, bytes] = {}
    modes: dict[Path, int] = {}
    constructed: dict[Path, bytes] = {}
    changed_entries: list[dict[str, Any]] = []

    for display_path, file_changes in changes_by_file.items():
        file_manifest = file_manifest_by_path.get(display_path)
        if file_manifest is None:
            raise ToolError(f"File is not present in manifest: {display_path}")
        path = Path(display_path)
        if not path.is_absolute():
            path = project_root / path
        path = path.resolve()
        if not path.is_file():
            raise ToolError(f"Selected TS file no longer exists: {path}")
        text_file = read_utf8(path)
        if sha256_bytes(text_file.raw) != file_manifest["sha256"]:
            raise ToolError(f"{display_path}: file changed since manifest scan; rescan first")
        parsed = parse_ts_file(path, project_root)
        if (
            parsed["translation_opening_tags_hash"]
            != file_manifest["translation_opening_tags_hash"]
            or parsed["translation_opening_tag_count"]
            != file_manifest["translation_opening_tag_count"]
        ):
            raise ToolError(f"{display_path}: <translation> opening-tag snapshot changed")
        current_by_id = {entry["entry_id"]: entry for entry in parsed["entries"]}
        verified_changes: list[tuple[dict[str, Any], dict[str, Any], list[str]]] = []
        for manifest_entry, change, targets in file_changes:
            current_entry = current_by_id.get(manifest_entry["entry_id"])
            if current_entry is None:
                raise ToolError(
                    f"{manifest_entry['entry_id']}: current message identity does not match manifest"
                )
            if current_entry["translation_hash"] != manifest_entry["translation_hash"]:
                raise ToolError(f"{manifest_entry['entry_id']}: translation content is stale")
            verified_changes.append((current_entry, change, targets))
            after_review_layer = review_layer(
                active=current_entry["active"],
                unfinished=current_entry["unfinished"],
                has_translation_content=any(target.strip() for target in targets),
            )
            before_review_layer = manifest_entry["review_layer"]
            if (
                before_review_layer == "unfinished-empty"
                and after_review_layer == "unfinished-nonempty"
            ):
                operation = "fill-empty-unfinished"
            elif before_review_layer == "unfinished-nonempty":
                operation = "revise-unfinished-candidate"
            elif before_review_layer == "finished":
                operation = "revise-finished"
            else:
                operation = "revise-translation"
            changed_entries.append(
                {
                    "entry_id": manifest_entry["entry_id"],
                    "file": display_path,
                    "language": manifest_entry["language"],
                    "context": manifest_entry["context"],
                    "id": manifest_entry["id"],
                    "review_layer_before": before_review_layer,
                    "review_layer_after": after_review_layer,
                    "operation": operation,
                    "confidence": change.get("confidence", ""),
                    "reason": change.get("reason", ""),
                }
            )
        updated_bytes = _construct_updated_file(text_file, parsed, verified_changes)
        try:
            ET.fromstring(updated_bytes)
        except ET.ParseError as error:
            raise ToolError(f"{display_path}: constructed XML is invalid: {error}") from error
        updated_text = updated_bytes[
            len(codecs.BOM_UTF8) if updated_bytes.startswith(codecs.BOM_UTF8) else 0 :
        ].decode("utf-8")
        opening_hash, opening_count = translation_opening_snapshot(updated_text)
        if (
            opening_hash != file_manifest["translation_opening_tags_hash"]
            or opening_count != file_manifest["translation_opening_tag_count"]
        ):
            raise ToolError(
                f"{display_path}: constructed file altered <translation> opening tags"
            )
        originals[path] = text_file.raw
        modes[path] = path.stat().st_mode
        constructed[path] = updated_bytes

    if not dry_run:
        written: list[Path] = []
        try:
            for path, updated_bytes in constructed.items():
                _atomic_replace(path, updated_bytes, modes[path])
                written.append(path)
            for path in written:
                display_path = relative_display(path, project_root)
                file_manifest = file_manifest_by_path[display_path]
                current = read_utf8(path)
                ET.fromstring(current.raw)
                opening_hash, opening_count = translation_opening_snapshot(current.text)
                if (
                    opening_hash != file_manifest["translation_opening_tags_hash"]
                    or opening_count != file_manifest["translation_opening_tag_count"]
                ):
                    raise ToolError(
                        f"{display_path}: post-write review-state opening-tag verification failed"
                    )
        except Exception:
            for path in written:
                _atomic_replace(path, originals[path], modes[path])
            raise

    return {
        "dry_run": dry_run,
        "file_count": len(constructed),
        "change_count": len(changed_entries),
        "changes": changed_entries,
        "warnings": warnings,
        "safety": {
            "all_files_preflighted_before_write": True,
            "translation_opening_tags_unchanged": True,
            "source_files_modified": False,
            "new_files_or_languages_created": False,
            "lupdate_invoked": False,
            "lrelease_invoked": False,
        },
    }


def write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def _print_json(value: Any) -> None:
    rendered = json.dumps(value, ensure_ascii=False, indent=2) + "\n"
    output = getattr(sys.stdout, "buffer", None)
    if output is not None:
        output.write(rendered.encode("utf-8"))
        output.flush()
    else:
        print(rendered, end="")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Inventory, inspect, validate, and minimally patch existing Qt Linguist TS files."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    inventory = subparsers.add_parser(
        "inventory",
        help="Mechanically aggregate per-file, per-language, and total message counts.",
    )
    inventory.add_argument(
        "--directory",
        action="append",
        default=[],
        help="Translation directory or individual Qt TS file.",
    )
    inventory.add_argument("--crowdin", help="Crowdin YAML selector.")
    inventory.add_argument(
        "--project-root", default=".", help="Project root for relative paths."
    )
    inventory.add_argument(
        "--locale", action="append", default=[], help="Locale, comma list, or all."
    )
    inventory.add_argument("--output", help="Optional inventory JSON output path.")

    scan = subparsers.add_parser("scan", help="Discover TS files and write a review manifest.")
    scan.add_argument("--directory", action="append", default=[], help="Translation directory or TS file.")
    scan.add_argument("--crowdin", help="Crowdin YAML selector.")
    scan.add_argument("--project-root", default=".", help="Project root for relative paths.")
    scan.add_argument("--scope", choices=SCOPES, required=True)
    scan.add_argument("--locale", action="append", default=[], help="Locale, comma list, or all.")
    scan.add_argument("--source-locale", help="Explicit source locale when inference is ambiguous.")
    scan.add_argument("--batch-size", type=int, default=80)
    scan.add_argument("--output", required=True, help="Manifest JSON output path.")

    validate = subparsers.add_parser("validate", help="Validate files and translations from a manifest.")
    validate.add_argument("--manifest", required=True)
    validate.add_argument("--no-lcheck", action="store_true")
    validate.add_argument("--output", help="Optional JSON report output path.")

    apply_parser = subparsers.add_parser("apply", help="Preflight and apply a JSON change plan.")
    apply_parser.add_argument("--manifest", required=True)
    apply_parser.add_argument("--changes", required=True)
    apply_parser.add_argument("--dry-run", action="store_true")
    apply_parser.add_argument("--output", help="Optional JSON report output path.")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.command == "inventory":
            project_root = Path(args.project_root).resolve()
            report = build_inventory(
                directories=args.directory,
                crowdin=args.crowdin,
                project_root=project_root,
                locales=args.locale,
            )
            if args.output:
                write_json(Path(args.output).resolve(), report)
            _print_json(report)
            return 0
        if args.command == "scan":
            if args.batch_size < 1:
                raise ToolError("--batch-size must be positive")
            project_root = Path(args.project_root).resolve()
            manifest = build_manifest(
                directories=args.directory,
                crowdin=args.crowdin,
                project_root=project_root,
                scope=args.scope,
                locales=args.locale,
                source_locale=args.source_locale,
                batch_size=args.batch_size,
            )
            output = Path(args.output).resolve()
            write_json(output, manifest)
            _print_json(
                {
                    "manifest": str(output),
                    "manifest_hash": manifest["manifest_hash"],
                    "selected_file_count": manifest["selected_file_count"],
                    "selected_directory_count": manifest["selected_directory_count"],
                    "selected_entry_count": manifest["selected_entry_count"],
                    "selected_by_review_layer": manifest[
                        "selected_by_review_layer"
                    ],
                    "selected_by_language_and_review_layer": manifest[
                        "selected_by_language_and_review_layer"
                    ],
                    "summary_by_language": manifest["summary_by_language"],
                    "skipped_file_count": len(manifest["skipped_files"]),
                    "skipped_id_count": len(manifest["skipped_ids"]),
                    "source_issue_count": len(manifest["source_issues"]),
                    "localization_candidate_count": len(
                        manifest["localization_candidates"]
                    ),
                }
            )
            return 0
        if args.command == "validate":
            manifest = load_manifest(Path(args.manifest).resolve())
            report = validate_manifest(manifest, run_lcheck=not args.no_lcheck)
            if args.output:
                write_json(Path(args.output).resolve(), report)
            _print_json(report)
            return 0 if report["valid"] else 2
        if args.command == "apply":
            manifest = load_manifest(Path(args.manifest).resolve())
            changes = load_json(Path(args.changes).resolve())
            report = apply_changes(manifest, changes, args.dry_run)
            if args.output:
                write_json(Path(args.output).resolve(), report)
            _print_json(report)
            return 0
        raise ToolError(f"Unsupported command: {args.command}")
    except ToolError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
