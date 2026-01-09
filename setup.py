import setuptools
import setuptools.command.build
import subprocess
from pathlib import Path
from shutil import which, rmtree
import platform


class BuildC(setuptools.Command):
    """Custom command to compile the C++ code.
    
    The 'run' method of this class will be called by setuptools during the build.
    """
    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        # Determine the name of the CMake preset to use
        system = platform.system()
        if system == "Linux":
            preset = "linux-gcc-release"
        elif system == "Darwin":
            preset = "macos-clang-release"
        elif system == "Windows":
            preset = "windows-msvc-release"
        else:
            raise ValueError("Unsupported OS")
        
        # Detect name of pkg directory since it is platform-dependant
        pkgdirpath = next(Path(".").glob("build/lib.*"))

        # Recreate the directory to remove the extra stuff inside
        rmtree(pkgdirpath)
        pkgdirpath.mkdir()

        # CMake configuration
        subprocess.run([which("cmake"),
                        "-B", "build/cmake",
                        "--preset", preset,
                        "-DMPI=OFF",
                        "-DTESTING=OFF"],
                        check=True)
        
        # CMake compilation
        subprocess.run([which("cmake"), "--build", "build/cmake", "--target", "umodl"], check=True)

        # Copy the files to the directory representing the final package content
        for filename in ["README.md", "LICENSE"]:
            self.copy_file(filename, pkgdirpath / filename)
        (pkgdirpath / "umodl").mkdir()
        self.copy_tree("src/umodl", pkgdirpath / "umodl")
        if system == "Windows":
            self.copy_file("build/cmake/bin/umodl.exe", pkgdirpath / "umodl/umodl.exe")
        else:
            self.copy_file("build/cmake/bin/umodl", pkgdirpath / "umodl/umodl")


class Build(setuptools.command.build.build):
    """Custom build class.
    
    Derive the build class to specify an addtional sub-command to run during the build.
    This subcommand is defined by the BuildC class.
    """
    sub_commands = setuptools.command.build.build.sub_commands + [('BuildC', None)]


class BinaryDistribution(setuptools.Distribution):
    """Custom distribution class.
    
    This trick makes setuptools treat the Python package as containing extension modules.
    It is not the case of this package. However, since it contains C++ code to be compiled,
    and because extension modules are modules consisting of code to be compiled, setuptools
    understands it has to build a wheel (binary distribution) specific to the platform it
    is running on. So the wheel will not be tagged as compatible with **any** platform but
    only with **one**, for example: Linux or macOS or Windows.
    """
    def has_ext_modules(self):
        return True


setuptools.setup(
    cmdclass={
        # Override the build class: see the docstring of the Build class for explanations
        'build': Build,
        # Tell setuptools which class we refer to when adding the 'BuildC' subcommand
        # to our custom 'Build' class. See the docstring of the BuildC class for
        # explanations.
        'BuildC': BuildC
    },
    # See the docstring of the BinaryDistribution class for explanations
    distclass=BinaryDistribution
)
