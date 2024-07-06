#!/usr/bin/env python3

import argparse
import os
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

# GENERAL BUILD / DEPENDENCY STRATEGY
# - Use Mise (mise.jdx.dev) to get Python and make a venv (see .mise.toml)
# - Use pip (pypi.org) in that venv to install Python tools (conan, cmake, etc)
# - Use Conan (conan.io) to install C++ dependencies (openfst, etc)
# - Use Meson (mesonbuild.com) and Ninja (ninja-build.org) to build C++

def run_shell(*av, **kw):
    av = [str(a) for a in av]
    print(f"🐚 {shlex.join(av)}")
    if av[:1] != ["mise"]:
        av = ["mise", "exec", "--", *av]
    return subprocess.run(av, **{"check": True, **kw})

parser = argparse.ArgumentParser(description="Nutrimatic dev environment setup")
parser.add_argument("--clean", action="store_true", help="Wipe build dir first")
parser.add_argument("--debug", action="store_true", help="Debug build for deps")
args = parser.parse_args()

top_dir = Path(__file__).resolve().parent
build_dir = top_dir / "build"
conan_dir = build_dir / "conan"
os.chdir(top_dir)

print(f"\n➡️ Build dir ({build_dir})")
if args.clean and build_dir.is_dir():
    print("🗑️ WIPING build dir (per --clean)")
    shutil.rmtree(build_dir)
    build_dir.mkdir()
elif build_dir.is_dir():
    print("🏠 Using existing build dir")
else:
    print("🏗️ Creating build dir")
    build_dir.mkdir()

(build_dir / ".gitignore").open("w").write("/*\n")

print(f"➡️ Mise (tool manager) setup")
if not shutil.which("mise"):
    print("🚨 Please install 'mise' (https://mise.jdx.dev/)")
    exit(1)

run_shell("mise", "install")
py_version = run_shell("python3", "-V", capture_output=True, text=True).stdout
if py_version.startswith("Python 3.10."):
    print(f"{py_version.strip()} (looks good!)")
else:
    print(f"🚨 Wrong Python after 'mise install': {py_version}")
    exit(1)

print(f"\n➡️ Python setup (pip packages)")
run_shell("python3", "-m", "pip", "install", "-r", "dev_requirements.txt")

# Link 'pkg-config' to 'pykg-config.py' to avoid relying on system pkg-config
venv_bin_dir = build_dir / "python_venv" / "bin"
pykg_config_path = venv_bin_dir / "pykg-config.py"
pkg_config_path = venv_bin_dir / "pkg-config"
pkg_config_path.unlink(missing_ok=True)
pkg_config_path.symlink_to(pykg_config_path)
pkg_config_version = run_shell(
    "pkg-config", "--version", capture_output=True, text=True
).stdout.strip()
if pkg_config_version:
    print(f"pkg-config {pkg_config_version} (looks good!)")
else:
    print(f"🚨 No output from pkg-config --version")
    exit(1)

print(f"\n➡️ Conan (C++ package manager) setup")
profile_path = conan_dir / "profiles" / "default"
run_shell("conan", "profile", "detect", "--name=detected", "--force")
print(f"⚙️ Writing: {profile_path}")
lines = ["include(detected)", "[settings]", "compiler.cppstd=17"]
profile_path.write_text("".join(f"{l}\n" for l in lines))

print(f"\n➡️ Install C++ dependencies")
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

env = os.environ
if not env.get("MISE_SHELL") and "mise/shims" not in env.get("PATH", ""):
    print("⚠️  Mise isn't shell activated! (mise.jdx.dev/getting-started.html)")
    print("   Build commands may not work as a result.")
