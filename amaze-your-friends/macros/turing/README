
			Vi Turing Machine Emulation
			---------------------------

This directory contains a set of macros which allow vi to simulate a
Turing Machine.  The file "tm" contains the macros.

The "TM*" files each contain a turing machine description.

To execute the TMupdown machine, do the following:

	$ vi TMupdown
	:so tm

Then, from escape mode in vi, type 'g' for go.

I've included a simple turing machine description to use as an example
in explaining the format.

----------------------- cut here for sample turing machine ---------------------

START{####:####:R:DOWN:}
DOWN{up:down:R:DOWN:}{%%%%:%%%%:L:UP:}
UP{down:up:L:UP:}{####:####:R:DOWN:}
####
up
up
up
up
%%%%
--------------------------- end of turing machine ------------------------------

The top line is used as a scratch pad by the macros and must not be
removed. The lines from the second line to the line containing "####"
encode the turing machine's state table, and the lines from "####" to
"%%%%" represent the turing machine's tape.

The tape is lying on its side such that left is up and right is down.
Each line represents one tape symbol.  "####" is the start symbol on
the tape, and "%%%%" is the end symbol.

Each line above "####" represents the information for one state
of the turing machine.   I'll describe the format using an example:

	DOWN{up:down:R:DOWN:}{%%%%:%%%%:L:UP:}

The name of the state, in this case "DOWN", comes first.  Following that
comes any number of 4-tuples describing the action to take for a particular
tape symbol.   The first 4-tuple states that if the current tape symbol
is "up", then that symbol should be replaced by "down", the current position
on the tape should be moved "R" -- that is, to the right -- and the
turing machine should enter the "DOWN" state.

The general format of these 4-tuples is:

   '{' <current symbol> ':' <new symbol> ':' <direction> ':' <next state> ':}'

Where <direction> is "R", "L", or "N" for move left, move right, or no move.
The other fields can contain any alpha-numeric string.  (In fact, any string
that does not include "{}:" or any vi magic characters will probably work.)

When a turing machine first starts, after the 'g' command, it is in the
"START" state with its head positioned on the "####" symbol on the tape.
