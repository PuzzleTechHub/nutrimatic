#!/usr/bin/env python3

import argparse
import json
import os
import shlex
import shutil
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser(description="Nutrimatic installertool")
parser.add_argument("output_dir", type=Path, help="Root directory to deploy to")
args = parser.parse_args()
output_path = args.output_dir.resolve()  # Resolve before changing directory

top_dir = Path(__file__).resolve().parent
os.chdir(top_dir)

# Build a Conan package and load it into the cache
conan_export = ["conan", "export-pkg", "--format=json", "."]
print(f"💪 {shlex.join(conan_export)}")
export_run = subprocess.run(conan_export, stdout=subprocess.PIPE, check=True)
export_json = json.loads(export_run.stdout)
node = next(iter(export_json["graph"]["nodes"].values()))
package_path = Path(node["package_folder"])

print(
    f"\n📁 Copying files...\n"
    f"  from {package_path}\n"
    f"  to   {output_path}"
)
output_path.mkdir(parents=True, exist_ok=True)
shutil.copytree(package_path, output_path, dirs_exist_ok=True)

print("\n📦 Install complete")
