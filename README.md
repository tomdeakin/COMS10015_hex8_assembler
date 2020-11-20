# COMS10015: Hex 8 ISA assembler

This is a simple assembler to convert Hex 8 assembly programs into machine code, suitable to be loaded into the ModuleSim NRAM.

## Building

The code should build just by running:

    make 

The code uses C++14, and so g++ version 5 or higher is required.

## Running

The binary takes a single argument: a `*.hex8` file with Hex 8 assembly instructions.
The machine code is output to a file called `a.hex` in the current working directory.

## Input file format

The `*.hex8` input file should be a valid Hex 8 program.

The assembler performs some basic error checking, but this is not exhaustive.

### Operands
There are 16 Hex 8 instruction mnemonics which can be used.
In the assembly program, the opcode mnumonic is followed by a single space and then an opcode represented in decimal (base 10).

Negative numbers are allowed.
The range of valid operands is therefore `[-127, 255]` inclusive.

The assembler will insert appropriate `PFIX` instructions for negative or large operands.

### Labels
Labels are supported.
Labels are of the form `L123`: the capital letter `L` followed by a number.
They must be on their own line.

Labels can be used as operands.

For example:

    L1
    ADD
    BR L1

The assembler will always insert a `PFIX` instruction before any instruction using a label.
A better assembler would remove ones that were not needed with additional passes.

### Data statements
Support is also included for the `DATA` mnemonic.

For example:

   DATA 42
   DATA L1

The assembler simply outputs the operand into the instruction stream.
For large absolute numbers and all labels, the assembler outputs `PFIX` instructions.

Labels can be applied to `DATA` statements so they can be used as memory addresses.
For example:

    L1 
    DATA 0
    STAM L1
