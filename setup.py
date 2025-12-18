import setuptools
import setuptools.command.build
import subprocess
from pathlib import Path
from shutil import which, rmtree
import platform


class BuildC(setuptools.Command):
    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        system = platform.system()
        if system == "Linux":
            preset = "linux-gcc-release"
        elif system == "Darwin":
            preset = "macos-clang-release"
        elif system == "Windows":
            preset = "windows-msvc-release"
        else:
            raise ValueError("Unsupported OS")
        pkgdirpath = next(Path(".").glob("build/lib.*"))  # Detect name of pkg directory since it is platform-dependant
        rmtree(pkgdirpath)
        pkgdirpath.mkdir()
        subprocess.run([which("cmake"),
                        "-B", "build/cmake",
                        "--preset", preset,
                        "-DMPI=OFF",
                        "-DTESTING=OFF"],
                        check=True)  # Generate build config files
        subprocess.run([which("cmake"), "--build", "build/cmake", "--target", "umodl"], check=True)  # Build
        for filename in ["README.md", "LICENSE"]:
            self.copy_file(filename, pkgdirpath / filename)
        (pkgdirpath / "umodl").mkdir()
        self.copy_tree("src/umodl", pkgdirpath / "umodl")
        if system == "Windows":
            self.copy_file("build/cmake/bin/umodl.exe", pkgdirpath / "umodl/umodl.exe")
        else:
            self.copy_file("build/cmake/bin/umodl", pkgdirpath / "umodl/umodl")


class Build(setuptools.command.build.build):
    sub_commands = setuptools.command.build.build.sub_commands + [('BuildC', None)]


class BinaryDistribution(setuptools.Distribution):
    def has_ext_modules(self):
        return True


setuptools.setup(
    cmdclass={
        'build': Build,
        'BuildC': BuildC
    },
    distclass=BinaryDistribution
)
