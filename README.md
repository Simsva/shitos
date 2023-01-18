# shitos

A small, pretty terrible, 32-bit x86 "operating system" we are trying to make to
learn low-level stuff.

## Building

To build the OS you need a working i686 GCC cross-compiler and linker
(`i686-elf-gcc` and `i686-elf-ld`) and the Netwide Assembler (`nasm`). These can
either be in your PATH permanently, or you can use as tool such as direnv to
limit it to the project directory. For a guide on building a cross-compiler,
refer to the [OSDev Wiki](https://wiki.osdev.org/GCC_Cross-Compiler).

(NOTE: building is only tested on Linux with GNU coreutils and Glibc)

To make only a working disk image (`shitos.iso`):

``` shell
$ make iso
```

To make a working disk image and debugging support:

``` shell
$ make debug iso
```

In case of errors relating to missing directories, run:

``` shell
$ make dirs
```

## Running on hardware

The only official target is QEMU i386, so good luck running it on hardware.

## Running in QEMU

Obviously, running in QEMU requires either `qemu-system-i386` or
`qemu-system-x86_64` (the former is the default in `run.sh`). You can either run
manually using QEMU or use the provided `run.sh` script.

To run without debugging:

``` shell
$ ./run.sh
```

To run with GDB:

``` shell
$ ./run.sh -d debug_mode
```

For more information see `./run.sh -h`.

## TODO (remove when done)

- Global header files (`stddef.h`, `stdarg.h`, `io.h`, etc.)
- Better directory structure for C code
