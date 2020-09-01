SDDBG
=====

This is a fork of Ricky White's [ec2drv](https://github.com/paragonRobotics/ec2-new),
with support for a [homegrown ti-cc debugger](https://github.com/bkleiner/cc-flasher)
and many fixes and improvments

New features include:
- built in [dap-server](https://microsoft.github.io/debug-adapter-protocol)
- full cmake built system
- improved step logic
- register inspection
- call stack inspection
- automatic disassembly of uncovered code

## Building

Before building, fetch the git submodules with:

```bash
cd <path-to-sddbg>
git submodule update --init
```

Generate the build files & build the project:

```bash
cd <path-to-sddbg>
mkdir build
cd build
cmake .. # alternatively cmake .. -GNinja
make
```
