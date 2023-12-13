# bfci
A simple, small, moderately optimizing compiling interpreter for the [brainfuck](https://esolangs.org/wiki/Brainfuck) programming language, targeting Linux x86-64.

### Compile
```
cc --std=c99 -Wall -Wextra -O3 -s -o bfci bfci.c src/*.c
```
### Usage
```
bfci -x program.bf
```
```
bfci -xc"+[++[<+++>->+++<]>+++++++]<<<--.<.<--..<<---.<+++.<+.>>.>+.>.>-.<<<<+.[<]>+."
```
