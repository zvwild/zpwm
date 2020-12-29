# zpwm
A password manager operating on zip files.

## Getting started
To get started, build the program from source, the only dependency required is [libzip](https://libzip.org/).

> Arch Linux users can install libzip directly using the [libzip](https://archlinux.org/packages/extra/x86_64/libzip/) package avaialbe in the offical repositories.

Having libzip installed, simply run `make` to produce a binary.

## Usage
Start the program giving it a file name as its first parameter.

`zpwm my_super_secret_passwords`

After which, you enter your password and are then inside the command prompt.

Inside the command prompt, the following commands are available:

| Comand |Description | Arguments |
| - | - | - |
| help | Display the help | - |
| get | Read the given entries stored under the given name | name |
| set | Set entries under a section | name entry1 entry2 ... |
| exit (or quit) | Save all changes | - |
| discard | Discard all changes (same as hitting Ctrl+C) | - |
| list | List all sections | - |
