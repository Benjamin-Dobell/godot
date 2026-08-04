#!/usr/bin/env python3
import re
import sys
from pathlib import Path

SCRIPT_LOCATION_REPLACEMENTS = (
    ("_scriptName = self.location.href;", "_scriptName = self.location?.href;"),
    ("_scriptName=self.location.href;", "_scriptName=self.location?.href;"),
)

NODE_ENVIRONMENT_PATTERN = re.compile(
    r"var ENVIRONMENT_IS_NODE(?P<before_equals>\s*)=(?P<after_equals>\s*)"
    r"(?P<expression>[^;]*process(?:\?\.|\.)versions(?:\?\.|\.)node[^;]*);"
)


def apply_replacements(source: str, replacements: tuple[tuple[str, str], ...]) -> tuple[str, int]:
    patched = source
    replacement_count = 0

    for needle, replacement in replacements:
        count = patched.count(needle)
        if count:
            patched = patched.replace(needle, replacement)
            replacement_count += count

    return patched, replacement_count


def harden_node_environment_check(source: str) -> tuple[str, int]:
    def replace(match: re.Match[str]) -> str:
        expression = match.group("expression")
        if "__dirname" in expression and "__filename" in expression:
            return match.group(0)

        return (
            f"var ENVIRONMENT_IS_NODE{match.group('before_equals')}={match.group('after_equals')}"
            f'typeof __dirname != "undefined" && typeof __filename != "undefined" && ({expression});'
        )

    return NODE_ENVIRONMENT_PATTERN.subn(replace, source)


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: patch_worker_script_location.py <generated-js>", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    source = path.read_text(encoding="utf-8")
    patched, script_location_count = apply_replacements(source, SCRIPT_LOCATION_REPLACEMENTS)
    patched, node_environment_count = harden_node_environment_check(patched)

    if script_location_count == 0:
        print("patch_worker_script_location.py: no self.location.href worker script marker found", file=sys.stderr)
        return 1

    has_loose_node_check = NODE_ENVIRONMENT_PATTERN.search(source) is not None
    has_hardened_node_check = (
        NODE_ENVIRONMENT_PATTERN.search(patched) is None
        and "typeof __dirname" in patched
        and "typeof __filename" in patched
    )
    if has_loose_node_check and node_environment_count == 0 and not has_hardened_node_check:
        print("patch_worker_script_location.py: no loose ENVIRONMENT_IS_NODE marker patched", file=sys.stderr)
        return 1

    path.write_text(patched, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
