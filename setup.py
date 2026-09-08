import os
import shutil
import subprocess
import sys
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        super().__init__(name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def run(self):
        try:
            subprocess.check_call(["cmake", "--version"])
        except OSError:
            raise RuntimeError("CMake must be installed to build tcamviewer")
        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        target_dir = os.path.join(extdir, "tcamviewer")
        os.makedirs(target_dir, exist_ok=True)

        # Build directory for CMake
        build_temp = os.path.join(self.build_temp, ext.name)
        os.makedirs(build_temp, exist_ok=True)

        cfg = "Release"
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={target_dir}",
            f"-DCMAKE_BUILD_TYPE={cfg}",
        ]

        build_args = [
            "--build", ".",
            "--target", "tcamviewer",
            "--config", cfg,
            "-j", str(os.cpu_count() or 2)
        ]

        subprocess.check_call(["cmake", ext.sourcedir] + cmake_args, cwd=build_temp)
        subprocess.check_call(["cmake"] + build_args, cwd=build_temp)

        # Copy libtcamviewer.so into source tree python/tcamviewer for editable installs
        built_lib = os.path.join(target_dir, "libtcamviewer.so")
        in_tree_dir = os.path.join(ext.sourcedir, "python", "tcamviewer")
        if os.path.exists(built_lib) and os.path.isdir(in_tree_dir):
            shutil.copy2(built_lib, in_tree_dir)

readme_path = Path(__file__).parent / "README.md"
long_description = readme_path.read_text(encoding="utf-8") if readme_path.exists() else ""

setup(
    name="tcamviewer",
    version="0.1.0",
    author="wkqco33",
    description="High-performance Terminal Video Player & Camera Monitor Library",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/wkqco33/tcamviewer",
    license="Apache-2.0",
    package_dir={"": "python"},
    packages=["tcamviewer"],
    package_data={"tcamviewer": ["*.so", "*.dylib", "*.dll"]},
    ext_modules=[CMakeExtension("tcamviewer._core", sourcedir=".")],
    cmdclass={"build_ext": CMakeBuild},
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Multimedia :: Video :: Display",
        "Topic :: Software Development :: Libraries",
    ],
    python_requires=">=3.8",
    install_requires=[],
)
