# bfci
A simple, small, moderately optimizing compiling interpreter for the [brainfuck](https://esolangs.org/wiki/Brainfuck) programming language, targeting Linux x86-64.

## Compile
```
cc -Wall -Wextra -O3 -s -o bfci bfci.c src/*.c
```
## Usage
As a compiler:
```
bfci -o program program.bf
./program
```
```
bfci -o hello -c"+[++[<+++>->+++<]>+++++++]<<<--.<.<--..<<---.<+++.<+.>>.>+.>.>-.<<<<+.[<]>+."
./hello
```
As a compiling interpreter:
```
bfci -x program.bf
```
```
bfci -xc"+[++[<+++>->+++<]>+++++++]<<<--.<.<--..<<---.<+++.<+.>>.>+.>.>-.<<<<+.[<]>+."
```
## Implementation Details
<dl>
  <dt>Cell Behavior</dt>
  <dd>Unsigned 8-bit, wrapping</dd>

  <dt>Tape Behavior</dt>
  <dd>65536 cells, circular</dd>

  <dt>EOF Behavior</dt>
  <dd>Leaves cell unmodified</dd>
</dl>
