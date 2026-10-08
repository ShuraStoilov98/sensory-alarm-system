#!/usr/bin/env python3
"""Compile and exercise the real firmware with host-side hardware stubs."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[1]
    source = root / "test/host/firmware_test.cpp"
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    scenarios = re.findall(r'\{"([a-z_]+)", \w+\}', source.read_text())
    with tempfile.TemporaryDirectory(prefix="curtain-tests-") as directory:
        executable = Path(directory) / "firmware-tests"
        subprocess.run(
            [*compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
             "-I", str(root / "test/host/stubs"), str(source), "-o", str(executable)],
            check=True,
        )
        for scenario in scenarios:
            subprocess.run([str(executable), scenario], check=True, timeout=10)
    print(f"{len(scenarios)} firmware scenarios passed")


if __name__ == "__main__":
    main()
