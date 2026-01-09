<img src=https://github.com/khiopsML/khiops/blob/dev/packaging/common/images/khiops.png width=60 />

# UModl
This repository contains:
- UModl, a C++ program forked from Khiops
- umodl, a Python wrapper around the C++ program.

## About the Python wrapper
The Python wrapper is a Python package that, when built, compiles the C++ program.
It also contains [a bit of Python code](./src/umodl) exporting a single function that runs the
compiled C++ program as a subprocess.

Besides the files needed for the compilation of the C++ program, the list of the files composing
the Python wrapper is the following:
- [./README.md](./README.md) (this file)
- [./LICENSE](./LICENSE)
- [./pyproject.toml](./pyproject.toml)
- [./setup.py](./setup.py)
- [./MANIFEST.in](./MANIFEST.in)
- [./src/umodl/\_\_init__.py](./src/umodl/__init__.py)
- [./src/umodl/runumodl.py](./src/umodl/runumodl.py)

The build backend of the wrapper is **setuptools**.
The Python wrapper uses a standard *pyproject.toml* file for what can be specified in a declarative
way, and a *setup.py* for what cannot. At the time of this writing, the use of the *setup.py* file is
not deprecated for specifying what cannot be in a *pyproject.toml* or *setup.cfg* file. The *setup.py*
file is needed to program the compilation of the C++ code, because it is not an extension module
and because it makes use of a custom **CMake** configuration.
The *MANIFEST.in* file describes what will be copied in the temporary directory creating by setuptools
during the build, and therefore what is needed to compile the C++ program, plus the files that have to
be included in the final package (README, license etc).
See the [GitHub CI workflow](./.github/workflows/python-publish.yml) for the steps needed to build the
source distribution and the binary distributions of the wrapper.


# Khiops
Khiops is an AutoML suite for supervised and unsupervised learning

## Installation
For the installation instructions [go to Khiops website][khiops-web].

## Documentation
See the documentation [at the Khiops website][khiops-web]

## Development
See the [developer's documentation wiki][wiki-dev].

## License
This software is distributed under the BSD 3-Clause-clear License, the text of which is available at
https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the [LICENSE](./LICENSE) for more
details.

## Help Contact
khiops.team@orange.com


[khiops-web]: https://khiops.org
[wiki-dev]: https://github.com/KhiopsML/khiops/wiki
