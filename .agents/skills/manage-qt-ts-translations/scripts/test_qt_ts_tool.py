from __future__ import annotations

import json
import io
import sys
import tempfile
import unittest
from unittest import mock
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import qt_ts_tool as tool


EN_MESSAGES = """
    <message>
      <location filename="../src/demo.cpp" line="10"/>
      <source>Open %1</source>
      <translation type="unfinished"></translation>
    </message>
    <message>
      <location filename="../src/demo.cpp" line="11"/>
      <source>Close</source>
      <translation type="unfinished">Candidate</translation>
    </message>
    <message>
      <location filename="../src/demo.cpp" line="12"/>
      <source>Ready</source>
      <translation>Ready</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/demo.cpp" line="13"/>
      <source>%n item(s)</source>
      <translation type="unfinished">
        <numerusform></numerusform>
        <numerusform></numerusform>
      </translation>
    </message>
    <message id="app.unannotated">
      <location filename="../src/demo.cpp" line="14"/>
      <source></source>
      <translation type="unfinished"></translation>
    </message>
    <message>
      <source>Old</source>
      <translation type="vanished">Old</translation>
    </message>
"""


ZH_MESSAGES = """
    <message>
      <location filename="../src/demo.cpp" line="10"/>
      <source>Open %1</source>
      <translation type="unfinished">打开 %1</translation>
    </message>
    <message>
      <location filename="../src/demo.cpp" line="11"/>
      <source>Close</source>
      <translation type="unfinished"></translation>
    </message>
    <message>
      <location filename="../src/demo.cpp" line="12"/>
      <source>Ready</source>
      <translation>就绪</translation>
    </message>
    <message numerus="yes">
      <location filename="../src/demo.cpp" line="13"/>
      <source>%n item(s)</source>
      <translation type="unfinished">
        <numerusform></numerusform>
      </translation>
    </message>
    <message id="app.unannotated">
      <location filename="../src/demo.cpp" line="14"/>
      <source></source>
      <translation type="unfinished"></translation>
    </message>
    <message>
      <source>Old</source>
      <translation type="obsolete">旧</translation>
    </message>
"""


def ts_document(language: str, messages: str) -> str:
    return (
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<!DOCTYPE TS>\n'
        f'<TS version="2.1" language="{language}" sourcelanguage="en_US">\n'
        '  <context>\n'
        '    <name>Demo</name>\n'
        f"{messages}"
        "  </context>\n"
        "</TS>\n"
    )


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(text.encode("utf-8"))


class QtTsToolTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name).resolve()

    def tearDown(self) -> None:
        self.temp.cleanup()

    def make_module(self) -> Path:
        translations = self.root / "module" / "translations"
        write_text(translations / "app_en_US.ts", ts_document("en_US", EN_MESSAGES))
        write_text(translations / "app_zh_CN.ts", ts_document("zh_CN", ZH_MESSAGES))
        write_text(
            translations / "component.ts",
            "export interface Component { value: string }\n",
        )
        return translations

    def test_directory_discovery_scope_and_typescript_exclusion(self) -> None:
        translations = self.make_module()
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[str(translations)],
            crowdin=None,
            scope="unfinished",
            locales=[],
            source_locale=None,
            batch_size=80,
        )

        self.assertEqual(manifest["selected_file_count"], 2)
        self.assertEqual(manifest["selected_directory_count"], 1)
        self.assertEqual(sorted(manifest["summary_by_language"]), ["en_US", "zh_CN"])
        self.assertEqual(
            sum(row["active"] for row in manifest["summary_by_language"].values()),
            10,
        )
        self.assertEqual(manifest["selected_entry_count"], 8)
        self.assertEqual(
            manifest["selected_by_review_layer"],
            {
                "unfinished-empty": 6,
                "unfinished-nonempty": 2,
                "finished": 0,
            },
        )
        self.assertEqual(
            sum(
                row["unfinished_empty"]
                for row in manifest["summary_by_language"].values()
            ),
            6,
        )
        self.assertEqual(
            sum(
                row["unfinished_nonempty"]
                for row in manifest["summary_by_language"].values()
            ),
            2,
        )
        self.assertEqual(
            sum(row["finished"] for row in manifest["summary_by_language"].values()),
            2,
        )
        self.assertEqual(
            sum(row["numerus"] for row in manifest["summary_by_language"].values()),
            2,
        )
        self.assertEqual(
            sum(
                row["id_without_engineering_source"]
                for row in manifest["summary_by_language"].values()
            ),
            2,
        )
        skipped = {item["path"]: item["reason"] for item in manifest["skipped_files"]}
        self.assertTrue(any(path.endswith("component.ts") for path in skipped))
        self.assertTrue(
            all(
                entry["translation_type"] not in {"vanished", "obsolete"}
                for entry in manifest["entries"]
                if entry["in_scope"]
            )
        )
        candidate = next(
            entry for entry in manifest["entries"] if entry["source"] == "Close" and entry["language"] == "en_US"
        )
        self.assertTrue(candidate["in_scope"])
        self.assertEqual(candidate["translation"], "Candidate")
        self.assertEqual(candidate["review_layer"], "unfinished-nonempty")

    def test_review_layer_scopes_are_distinct_and_composable(self) -> None:
        translations = self.make_module()
        expected = {
            "unfinished-empty": (3, {"unfinished-empty": 3, "unfinished-nonempty": 0, "finished": 0}),
            "unfinished-nonempty": (1, {"unfinished-empty": 0, "unfinished-nonempty": 1, "finished": 0}),
            "unfinished": (4, {"unfinished-empty": 3, "unfinished-nonempty": 1, "finished": 0}),
            "finished": (1, {"unfinished-empty": 0, "unfinished-nonempty": 0, "finished": 1}),
            "all": (5, {"unfinished-empty": 3, "unfinished-nonempty": 1, "finished": 1}),
        }
        for scope, (selected_count, layer_counts) in expected.items():
            with self.subTest(scope=scope):
                manifest = tool.build_manifest(
                    project_root=self.root,
                    directories=[str(translations)],
                    crowdin=None,
                    scope=scope,
                    locales=["en_US"],
                    source_locale=None,
                    batch_size=80,
                )
                self.assertEqual(manifest["selected_entry_count"], selected_count)
                self.assertEqual(manifest["selected_by_review_layer"], layer_counts)
                self.assertEqual(
                    manifest["selected_by_language_and_review_layer"],
                    {"en_US": layer_counts},
                )
                self.assertTrue(
                    all(
                        entry["review_layer"] in tool.REVIEW_LAYERS
                        for entry in manifest["entries"]
                        if entry["active"]
                    )
                )

    def test_inventory_aggregates_every_file_language_scope_and_total(self) -> None:
        translations = self.make_module()
        with mock.patch.object(
            tool,
            "engineering_source_from_code",
            side_effect=AssertionError("inventory must not inspect source code"),
        ):
            inventory = tool.build_inventory(
                project_root=self.root,
                directories=[str(translations)],
                crowdin=None,
                locales=[],
            )

        self.assertEqual(inventory["file_count"], 2)
        self.assertEqual(inventory["directory_count"], 1)
        self.assertTrue(inventory["safety"]["read_only"])
        self.assertFalse(inventory["safety"]["source_code_inspected"])
        self.assertTrue(
            any(item["path"].endswith("component.ts") for item in inventory["skipped_files"])
        )

        en_file = next(
            item for item in inventory["files"] if item["language"] == "en_US"
        )
        self.assertEqual(en_file["messages_total"], 6)
        self.assertEqual(en_file["active_total"], 5)
        self.assertEqual(en_file["historical_total"], 1)
        self.assertEqual(
            en_file["scopes"],
            {
                "unfinished-empty": 3,
                "unfinished-nonempty": 1,
                "unfinished": 4,
                "finished": 1,
                "all": 5,
            },
        )
        self.assertEqual(en_file["historical"], {"vanished": 1, "obsolete": 0})
        self.assertEqual(en_file["numerus_total"], 1)
        self.assertEqual(en_file["id_based_total"], 1)

        self.assertEqual(inventory["totals"]["messages_total"], 12)
        self.assertEqual(inventory["totals"]["active_total"], 10)
        self.assertEqual(inventory["totals"]["historical_total"], 2)
        self.assertEqual(
            inventory["totals"]["scopes"],
            {
                "unfinished-empty": 6,
                "unfinished-nonempty": 2,
                "unfinished": 8,
                "finished": 2,
                "all": 10,
            },
        )
        self.assertEqual(
            inventory["by_language"]["en_US"]["messages_total"],
            en_file["messages_total"],
        )
        self.assertEqual(
            inventory["by_language"]["en_US"]["scopes"],
            en_file["scopes"],
        )
        self.assertEqual(
            inventory["by_language"]["zh_CN"]["historical"],
            {"vanished": 0, "obsolete": 1},
        )

    def test_inventory_cli_accepts_an_individual_qt_ts_file(self) -> None:
        translations = self.make_module()
        selected = translations / "app_en_US.ts"
        output = io.StringIO()
        with mock.patch("sys.stdout", output):
            return_code = tool.main(
                [
                    "inventory",
                    "--directory",
                    str(selected),
                    "--project-root",
                    str(self.root),
                    "--locale",
                    "en_US",
                ]
            )
        self.assertEqual(return_code, 0)
        report = json.loads(output.getvalue())
        self.assertEqual(report["file_count"], 1)
        self.assertEqual(report["totals"]["messages_total"], 6)
        self.assertEqual(report["totals"]["scopes"]["all"], 5)

    def test_crowdin_source_discovers_existing_sibling_locales_only(self) -> None:
        self.make_module()
        write_text(
            self.root / "crowdin.yml",
            "files:\n"
            "  - source: '/**/translations/*_en_US.ts'\n"
            "    translation: '/%original_path%/%file_name%_%locale_with_underscore%.ts'\n",
        )
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[],
            crowdin=str(self.root / "crowdin.yml"),
            scope="all",
            locales=[],
            source_locale=None,
            batch_size=80,
        )
        self.assertEqual(manifest["selected_file_count"], 2)
        self.assertFalse(
            any(item["path"].endswith("component.ts") for item in manifest["files"])
        )
        self.assertTrue(
            all((self.root / item["path"]).exists() for item in manifest["files"])
        )

    def test_source_grammar_and_localization_readiness_candidates(self) -> None:
        translations = self.root / "translations"
        source = """
    <message>
      <location filename="../src/timer.cpp" line="3"/>
      <source>This was created with an newer version.</source>
      <translation>This was created with an newer version.</translation>
    </message>
    <message>
      <location filename="../src/timer.cpp" line="8"/>
      <source>Completed in %1 seconds</source>
      <translation>Completed in %1 seconds</translation>
    </message>
"""
        write_text(translations / "app_en_US.ts", ts_document("en_US", source))
        write_text(
            self.root / "src" / "timer.cpp",
            "void f(double elapsed) {\n"
            "  tr(\"unused\");\n"
            "  // filler\n"
            "  // filler\n"
            "  // filler\n"
            "  // filler\n"
            "  // filler\n"
            '  qDebug() << tr("Completed in %1 seconds").arg(elapsed, 0, \'f\', 3);\n'
            "}\n",
        )
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[str(translations)],
            crowdin=None,
            scope="all",
            locales=["en_US"],
            source_locale=None,
            batch_size=80,
        )
        grammar_codes = {issue["code"] for issue in manifest["source_issues"]}
        self.assertIn("article", grammar_codes)
        candidates = manifest["localization_candidates"]
        self.assertTrue(
            any(candidate["source"] == "Completed in %1 seconds" for candidate in candidates)
        )

    def test_placeholder_html_filter_and_shortcut_validation(self) -> None:
        good = tool.validate_translation_text(
            '<p title="x">File %1</p>',
            '<p title="x">文件 %1</p>',
        )
        self.assertFalse(good["errors"])

        placeholder = tool.validate_translation_text("File %1", "文件 %2")
        self.assertTrue(
            any("placeholder" in issue for issue in placeholder["errors"])
        )

        html = tool.validate_translation_text("<b>File</b>", "文件")
        self.assertTrue(any("rich-text" in issue for issue in html["errors"]))

        file_filter = tool.validate_translation_text(
            "Images (*.png *.jpg);;All files (*)",
            "图像 (*.png);;所有文件 (*)",
        )
        self.assertTrue(any("file-filter" in issue for issue in file_filter["errors"]))

        shortcut = tool.validate_translation_text("Ctrl+Shift+S", "Ctrl+S")
        self.assertTrue(any("shortcut" in issue for issue in shortcut["warnings"]))

    def test_apply_changes_is_dry_runnable_minimal_and_state_preserving(self) -> None:
        translations = self.make_module()
        source_file = translations / "app_en_US.ts"
        original = source_file.read_bytes()
        original_openings = tool.translation_opening_snapshot(original)
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[str(translations)],
            crowdin=None,
            scope="unfinished",
            locales=["en_US"],
            source_locale=None,
            batch_size=80,
        )
        plain = next(entry for entry in manifest["entries"] if entry["source"] == "Open %1")
        candidate = next(entry for entry in manifest["entries"] if entry["source"] == "Close")
        plural = next(entry for entry in manifest["entries"] if entry["source"] == "%n item(s)")
        changes = {
            "manifest_hash": manifest["manifest_hash"],
            "changes": [
                {"entry_id": plain["entry_id"], "translation": "Open %1"},
                {"entry_id": candidate["entry_id"], "translation": "Close"},
                {
                    "entry_id": plural["entry_id"],
                    "forms": ["%n item", "%n items"],
                },
            ],
        }

        dry_run = tool.apply_changes(
            manifest,
            changes,
            dry_run=True,
        )
        self.assertEqual(dry_run["file_count"], 1)
        self.assertEqual(source_file.read_bytes(), original)

        result = tool.apply_changes(
            manifest,
            changes,
            dry_run=False,
        )
        self.assertEqual(result["change_count"], 3)
        operations = {change["operation"] for change in result["changes"]}
        self.assertEqual(
            operations,
            {"fill-empty-unfinished", "revise-unfinished-candidate"},
        )
        self.assertTrue(
            all(
                change["review_layer_after"] == "unfinished-nonempty"
                for change in result["changes"]
            )
        )
        updated = source_file.read_bytes()
        self.assertNotEqual(updated, original)
        self.assertEqual(tool.translation_opening_snapshot(updated), original_openings)
        parsed = tool.parse_ts_file(source_file, self.root)
        self.assertEqual(
            next(entry for entry in parsed["entries"] if entry["source"] == "Open %1")["translation"],
            "Open %1",
        )
        self.assertEqual(
            next(entry for entry in parsed["entries"] if entry["source"] == "%n item(s)")["forms"],
            ["%n item", "%n items"],
        )
        self.assertEqual(updated.count(b'type="unfinished"'), original.count(b'type="unfinished"'))

    def test_validation_reports_each_review_layer_separately(self) -> None:
        translations = self.root / "translations"
        messages = """
    <message>
      <source>File %1</source>
      <translation type="unfinished"></translation>
    </message>
    <message numerus="yes">
      <source>%n item(s)</source>
      <translation type="unfinished">
        <numerusform>%n item</numerusform>
        <numerusform></numerusform>
      </translation>
    </message>
    <message>
      <source>Done</source>
      <translation></translation>
    </message>
"""
        write_text(translations / "app_en_US.ts", ts_document("en_US", messages))
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[str(translations)],
            crowdin=None,
            scope="all",
            locales=["en_US"],
            source_locale=None,
            batch_size=80,
        )
        report = tool.validate_manifest(manifest, run_lcheck=False)
        findings_by_source = {}
        for finding in report["findings"]:
            findings_by_source.setdefault(finding.get("source", ""), []).append(finding)

        empty_codes = {
            finding["code"] for finding in findings_by_source["File %1"]
        }
        self.assertEqual(empty_codes, {"unfinished-empty"})
        self.assertFalse(
            any(
                finding["code"] == "translation-structure"
                for finding in findings_by_source["File %1"]
            )
        )
        partial_codes = {
            finding["code"] for finding in findings_by_source["%n item(s)"]
        }
        self.assertIn("unfinished-partial-empty", partial_codes)
        finished_findings = findings_by_source["Done"]
        self.assertTrue(
            any(
                finding["code"] == "empty-finished"
                and finding["severity"] == "error"
                and finding["review_layer"] == "finished"
                for finding in finished_findings
            )
        )
        self.assertIn("unfinished-empty", report["findings_by_review_layer"])
        self.assertIn("unfinished-nonempty", report["findings_by_review_layer"])
        self.assertIn("finished", report["findings_by_review_layer"])

    def test_stale_manifest_rejects_entire_write(self) -> None:
        translations = self.make_module()
        source_file = translations / "app_en_US.ts"
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[str(translations)],
            crowdin=None,
            scope="unfinished",
            locales=["en_US"],
            source_locale=None,
            batch_size=80,
        )
        target = next(entry for entry in manifest["entries"] if entry["source"] == "Open %1")
        source_file.write_bytes(source_file.read_bytes().replace(b"<source>Ready</source>", b"<source>Set</source>"))
        stale_bytes = source_file.read_bytes()
        with self.assertRaises(tool.ToolError):
            tool.apply_changes(
                manifest,
                {
                    "manifest_hash": manifest["manifest_hash"],
                    "changes": [{"entry_id": target["entry_id"], "translation": "Open %1"}],
                },
                dry_run=False,
            )
        self.assertEqual(source_file.read_bytes(), stale_bytes)

    def test_unannotated_id_cannot_be_changed(self) -> None:
        translations = self.make_module()
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[str(translations)],
            crowdin=None,
            scope="unfinished",
            locales=["zh_CN"],
            source_locale=None,
            batch_size=80,
        )
        target = next(
            entry
            for entry in manifest["entries"]
            if entry["id"] == "app.unannotated" and entry["language"] == "zh_CN"
        )
        with self.assertRaises(tool.ToolError):
            tool.apply_changes(
                manifest,
                {
                    "manifest_hash": manifest["manifest_hash"],
                    "changes": [{"entry_id": target["entry_id"], "translation": "不可修改"}],
                },
                dry_run=False,
            )

    def test_xml_escaping_round_trip(self) -> None:
        translations = self.make_module()
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[str(translations)],
            crowdin=None,
            scope="unfinished",
            locales=["zh_CN"],
            source_locale=None,
            batch_size=80,
        )
        target = next(
            entry
            for entry in manifest["entries"]
            if entry["source"] == "Close" and entry["language"] == "zh_CN"
        )
        translated = '关闭 <窗口> & "保存"'
        result = tool.apply_changes(
            manifest,
            {
                "manifest_hash": manifest["manifest_hash"],
                "changes": [{"entry_id": target["entry_id"], "translation": translated}],
            },
            dry_run=False,
        )
        self.assertEqual(result["change_count"], 1)
        parsed = tool.parse_ts_file(translations / "app_zh_CN.ts", self.root)
        actual = next(entry for entry in parsed["entries"] if entry["source"] == "Close")
        self.assertEqual(actual["translation"], translated)

    def test_batching_caps_size_and_never_splits_a_message_or_plural_forms(self) -> None:
        translations = self.make_module()
        manifest = tool.build_manifest(
            project_root=self.root,
            directories=[str(translations)],
            crowdin=None,
            scope="all",
            locales=[],
            source_locale=None,
            batch_size=2,
        )
        self.assertTrue(manifest["batches"])
        self.assertLessEqual(max(map(len, manifest["batches"])), 2)
        for entry in manifest["entries"]:
            memberships = [
                index
                for index, batch in enumerate(manifest["batches"])
                if entry["entry_id"] in batch
            ]
            self.assertEqual(len(memberships), int(entry["in_scope"]))

    def test_json_output_uses_utf8_even_with_a_narrow_text_encoding(self) -> None:
        raw = io.BytesIO()
        text = io.TextIOWrapper(raw, encoding="ascii")
        previous = sys.stdout
        try:
            sys.stdout = text
            tool._print_json({"copyright": "©", "translation": "翻译"})
            text.flush()
        finally:
            sys.stdout = previous
        self.assertEqual(
            json.loads(raw.getvalue().decode("utf-8")),
            {"copyright": "©", "translation": "翻译"},
        )


if __name__ == "__main__":
    unittest.main()
