multiple
========

A tool-set for prototyping and implementing usable 
programming languages with dynamic type system


Introduction
------------

The project was as the backend and virtual machine parts of 
another programming language 
and been separated and restarted as a new project for
the following purpose:

1. Create a virtual machine with rich features to discovery a sutable
architecture with effective instrument-set for running dynamic-type 
programming languages.
2. Create a framework and a series of tools to provide an easy and clear
way for designers of programming languages to create a usable prototype 
and verify their ideas.
3. Create a platform for people who use different programming languages
to cooperate and communicate together.


Features
--------

1. Rich built-in Data Types 
2. Mark-Sweep Garbage Collector
3. Exchange Data by writting extensions in C/C++
4. Source Level Error output
5. Interoperate between different programming languages
6. Rich tools for helping generating intermediate representation


Source Structure
----------------

```
doc           -- Documents
src           -- Source Code
  core        -- Core
  gc          -- Garbage Collector
  lang        -- Programming Languages Frontends
  lib         -- Libraries
  misc        -- Miscellaneous
  special     -- Launcher & Library Part
  tools       -- Tools
    multiply  -- IR Generating Tools
  vm          -- Virtual Machine
```


Building
--------

Python2 is required to generate the Makefile. 
(Python3 is not supported yet)

Use the following command to generate the Makefile:
```
$ ./configure
```

And the following commands to build the project:
```
$ make # for building the test program
$ make static # for building the static library
$ make shared # for building the shared (dynamic) library
```


Usage
-----

```
Usage : multiple [options] <file> [arguments]

General Options:
  -c <file>                     Source code file
  -o <file>                     Output file (default:stdout)
  -g                            Include Debug Information
Frontend Options:
  -f, --frontend <frontend>     Specify frontend (default:auto)
  -lf, --list-frontends         List frontends
Backend Options:
  -r, --run                     Virtual Machine (default)
  -b                            Bytecode
  -S                            Assembly language
  -d, --debug                   Debugger
Optimization Options:
  -O<num>                       Enable Optimization
Virtual Machine Options:
  Memory Usage:
      --vm-mem <item> <type> <size>
        item: [infrastructure|primitive|reference]
        type: [default|libc|4k|64b|128b]
        size: (0 for unlimited)
Additions:
  --completion <cmd>            Completion

  --help                        Show help information
  --build-info                  Show build information
  --experimental                Show experimental functions
  --version                     Show version information
```


License
-------

GPLv3

