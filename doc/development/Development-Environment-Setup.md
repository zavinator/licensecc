# Developer's environment setup
This guide is just help in case you haven't decided your development environment or your development environment doesn't match ours.
We don't have any specific dependency on development tools, you can choose the one you prefer. However, if you want to contribute
you're required to format the code using `clang-format` before you submit the pull request (see `CONTRIBUTING.md`).

## Linux setup

First download the source code and compile it from the command line as described in [build the library](Build-the-library).

```console
sudo apt-get install clang-format ninja-build
```

`cpplint` is optional (it is no longer used in CI):

```console
sudo curl -L "https://raw.githubusercontent.com/google/styleguide/gh-pages/cpplint/cpplint.py" -o /usr/local/bin/cpplint.py
sudo chmod a+x /usr/local/bin/cpplint.py
```

## Windows setup

Formatting works the same way as on Linux: the repository ships a `.clang-format` file that is picked up by
Visual Studio and the VS Code C/C++ extension. No additional setup is required.