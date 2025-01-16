# Custom Linux Shell
**Developed by Owen Treanor**

## Overview
This project is a custom Linux shell implemented in C. It mimics the functionality of a basic Unix-like shell, supporting built-in commands, process control, and interaction with the filesystem.

## Features
- Built-in commands:
  - cd: Change directories, including support for `~` as a home directory shortcut.
  - pwd: Print the current working directory.
  - lf: List files in the current directory, similar to the standard ls command.
  - lp: List running processes with user and command information.
  - exit: Exits the shell.
- Executes external commands using execvp.
- Handles SIGINT (Ctrl+C) gracefully by terminating child processes but not the shell itself.
- Provides detailed error messages for invalid commands or operations.

## Usage
1. Make sure you are running Linux.
2. Clone the repo.
3. Compile the shell.c file using GCC.
4. Run the executable.
5. Try out the built in commands or run external commands.
6. Type 'exit' to exit the program.

## Technologies Used
- C
- Linux

## License
This program is licensed under the MIT License.
