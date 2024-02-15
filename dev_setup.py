#!/usr/bin/env python3

import argparse
import os
import shlex
import shutil
import subprocess
from pathlib import Path

# GENERAL BUILD / DEPENDENCY STRATEGY
# - Use pip in venv (pypi.org) for Python dependencies (like conan, meson, etc)
# - Use Conan (conan.io) to install C++ dependencies (ffmpeg, etc)
# - Use Meson (mesonbuild.com) and Ninja (ninja-build.org) to build C++

def run_shell(*av, **kw):
    av = [str(a) for a in av]
    if kw.pop("print", True):
        print(f"🐚 {shlex.join(av)}")
    subprocess.run(["mise", "exec", "--", *av], **{"check": True, **kw})

parser = argparse.ArgumentParser(description="Pivid dev environment setup")
parser.add_argument("--clean", action="store_true", help="Wipe build dir first")
parser.add_argument("--debug", action="store_true", help="Debug build for deps")
args = parser.parse_args()

if not shutil.which("mise"):
    print("🚨 Please install 'mise' (https://mise.jdx.dev)")
    exit(1)

top_dir = Path(__file__).resolve().parent
build_dir = top_dir / "build"
conan_dir = build_dir / "conan"
os.chdir(top_dir)

print(f"➡️ Build dir ({build_dir})")
if args.clean and build_dir.is_dir():
    print("🗑️ ERASING build dir (per --clean)")
    shutil.rmtree(build_dir)

build_dir.mkdir(exist_ok=True)
(build_dir / ".gitignore").open("w").write("/*\n")

print(f"\n➡️ Conan (C++ package manager) setup")
# Some recipes (eg. zlib) don't list cmake as a dep, so just install it first
run_shell("pip", "install", "conan~=2.0", "cmake~=3.28")
profile_path = conan_dir / "profiles" / "default"
run_shell("conan", "profile", "detect", "--name=detected", "--force")
print(f"⚙️ Writing: {profile_path}")
lines = ["include(detected)", "[settings]", "compiler.cppstd=17"]
profile_path.write_text("".join(f"{l}\n" for l in lines))

print(f"\n➡️ Build C++ dependencies")
run_shell(
    "conan",
    "install",
    f"--settings=build_type={'Debug' if args.debug else 'Release'}",
    "--build=missing",  # Allow source builds for all packages
    top_dir,
)

# Clean up cached packages that weren't used in this process
print(f"\n➡️ Clean C++ package cache")
run_shell("conan", "remove", "--lru=1d", "--confirm", "*")
run_shell("conan", "cache", "clean", "*")

print(f"\n😎 Setup complete, build with: conan build .")
