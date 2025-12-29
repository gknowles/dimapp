<!--
Copyright Glen Knowles 2016 - 2025.
Distributed under the Boost Software License, Version 1.0.
-->

# dimapp

| MSVC 2022 | Test Coverage |
| :-------: | :-----------: |
| [![Build Status][ci-badge]][ci-link] | [![Coverage][cov-badge]][cov-link] |

[ci-link]: https://github.com/gknowles/dimapp/actions/workflows/github-build.yml
[ci-badge]: https://github.com/gknowles/dimapp/actions/workflows/github-build.yml/badge.svg

[cov-link]: https://codecov.io/gh/gknowles/dimapp/tree/master
[cov-badge]: https://codecov.io/gh/gknowles/dimapp/branch/master/graph/badge.svg?token=AsZbKqf6lA

Windows framework for server and console applications.

## Working on the dimapp project
### Prerequisites
* CMake >= 3.10
* Visual Studio 2022

### Configure

~~~ console
git clone https://github.com/gknowles/dimapp.git
cd dimapp
git submodule update --init
configure.bat
~~~

### Build

Build from command line:

~~~ console
msbuild /p:Configuration=Release /m /v:m build\dimapp.sln
~~~

Or in Visual Studio:

~~~ console
devenv build\dimapp.sln
~~~

Binaries are placed in dimapp/bin, and can be run from there for simple
testing.
