# LuaVM - A Virtual Machine for the Lua language

LuaVM is a stack-based bytecode virtual machine made without any external depedencies to interpret and correctly execute Lua scripts that adheres to the `Lua 5.1` syntax.

The behaviors of the virtual machine is also based on the expected behavior of the `Lua 5.1` specifications. This means that very modern features such as `vararg` supports are not implemented, while older features such as the `arg` dictionary are are not deprecated.

## Current Features

Right now, the virtual machine supports all built-in Lua functionalities that comes natively with the language:

+ Arithmetic operations
+ Variable declarations
+ Function declarations
+ Function calls with argument adjustment
+ Assignment adjustment and multilple return values
+ Closures and recursions 
+ `vararg` functions
+ Tables and arrays
+ Metatables

Apart from those, the virtual machine also currently supports partial functionalities from the **standard library**, the **string library**, the **table library**, and the **math library**.

## Running the VM

Clone the directory and all its source code as well as build script with 

```bash
git clone https://github.com/Hy-LeTuan/luavm.git
```

The project is built with `CMake`, and there are presets saved in `CMakePresets.json` for quicker set up.

The default preset `dev-debug` includes with it the flags to print out all the bytecodes that the VM generates as well as an update on the stack during each execution loop.

After building the project with `CMake`, run `LuaVM` with 

```bash
luavm input.lua
```

where `input.lua` is any Lua file you have created.

### Running test

You will need to have `git-lfs` enabled to pull down input files for tests.

After pulling test files, you can choose the `ci-test` preset or create your own testing directory and run `ctest`.

### Running the demo

In order to run the demo, build the project in release mode or choose the `demo` preset. There currently are **2 demo Lua files** stored in the `demo/` directory, which are:

+ A `TwoSum` program
+ A `BinaryTree` program

Running the demo file is like running any other `.lua` file:

```bash
luavm ./demo/twosum.lua
```

or

```bash
luavm ./demo/binary_tree.lua
```

## Future Works

This is an ongoing project, and I apologize for the missing features. That said, here are some of the functionalities that I will be working towards in the near future: 

+ Coroutines
+ Threads
+ The supporting libraries
+ Type casting
+ API for garbage collection
+ Integrating with C