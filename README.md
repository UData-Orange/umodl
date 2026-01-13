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
- [./packaging/python/umodl/\_\_init__.py](./packaging/python/umodl/__init__.py)
- [./packaging/python/umodl/runumodl.py](./packaging/python/umodl/runumodl.py)

The build backend of the wrapper is **scikit-build-core**.

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
