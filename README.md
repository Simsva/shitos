# ShitOS

A small, pretty terrible, 32-bit x86 "operating system" we are trying to make to
learn low-level stuff.

## Building

To build ShitOS you need a working GCC cross-compiler and GNU binutils (target
triplet `i686-shitos`) and the Netwide Assembler (`nasm`). These can either be
in your PATH permanently, or you can use as tool such as direnv to limit it to
the project directory. The modified GNU toolchain can be found on
[GitLab](https://gitlab.com/Simsva), in the repositories
[shitos-gcc](https://gitlab.com/Simsva/shitos-gcc) and
[shitos-binutils](https://gitlab.com/Simsva/shitos-binutils). For a guide on
building a cross-compiler, refer to the [OSDev
Wiki](https://wiki.osdev.org/GCC_Cross-Compiler). (At the time of writing, use
the branch `releases/gcc-12` in `shitos-gcc`, and `binutils-2_40-branch` in
`shitos-binutils`. Also remember to pass the
`--with-sysroot=/path/to/shitos/sysroot` option to `configure`.)

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

- Better directory structure for C code
- More structured kernel logging
- libc alltypes.h and no namespace pollution
