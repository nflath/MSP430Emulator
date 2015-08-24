This project contains a MSP430 emulator/debugger I wrote in order to solve the
microcorruption CTF.  It provides several features that were useful in debugging the
problems, compared to the web interface - limited reversible debugging and
instruction tracing, along with other features.

--------------------------------------------------------------------------------
Usage
--------------------------------------------------------------------------------
To run a binary:
./emulator <binary>
>> c

The emulator always starts in debug mode; 'c' represents continuing.  A full list of commands
available at the prompt is as follows (all arguments are in hex unless specified):

load \$ARG - reloads the state of the emulator from the 'binary' file \$ARG

break \$ARG - when the PC is at \$ARG, return to the debug prompt

watch \$ARG - When the value at \$ARG (either a memory adress or register) changes,
return to the debug prompt

rb \$ARG - remove the breakpoint or watchpoint at \$ARG

reset - Resets the emulator to the initial state

c - Continue(run instructions until a breakpoint or watchpoint is hit).

s [\$ARG] - Exeutes one instruction.  If \$ARG is given(in decimal), repeat \$ARG times.

r [\$ARG] - reverse-step.  Return to the state before the instruction was executed.
If \$ARG is given(in decimal), repeat \$ARG times.

f - run instructions until a function return occurs

p $\ARG - print the given memory address or register (examples : p *4400, p r14)

run \$ARG - runs all instructions in the file \$ARG

exit-on-finished - when the processor stops running, exit the program.

exit - exit

read-break \$ARG - When a memory read of location \$ARG occurs, return to the debug
prompt.

compare \$ARG - Compares the memory contents of the current state and the binary file
\$ARG and prints the difference.

dump - Prints a 'binary' file representing the current state.

--print [\$ARG] - Always print executed instructions, even when not debugging (EG, if
you are continuing until a breakpoint, every instruction is printed.  If given an
argument, the print will only occur if the instruction being executed is at that
address.

rand $ARG - when a random number is generated, generates \$ARG instead

--trace - When executing an instruction, print the bytes of thae assembly.

--break-on-print - If an instruction causes a character to be printed, return to the debug prompt

--break-on-input - If an instruction causes a character to be input, return to the debug prompt

steps - Prints the number of instructions since starting, or since the last 'reset-steps'
   command

reset-steps - resets the number of instructions displayed by the 'steps' command to 0

echo - echoes the argument

--------------------------------------------------------------------------------
Data file format
--------------------------------------------------------------------------------
The 'binary' format to load is essentially the copy-pasted 'Live Memory Dump' and
'Register State' fields on microcorruption - look at neworleans.data for an example.

--------------------------------------------------------------------------------
Building
--------------------------------------------------------------------------------

execute 'make' in the source directory

However, it is a very rudimentary makefile, so may not work on all platforms.  Please
let me know if there are any issues.

--------------------------------------------------------------------------------

Patches are welcome.
