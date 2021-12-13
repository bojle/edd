# edd - line-based text editor

`edd` is my attempt at writing the [ed](https://www.gnu.org/software/ed/) text-editor from scratch. 
Initially, I was planning to only write a subset of ed, but ended up writing
it in its entirety. I decided it would
be nice to have some extra features like:

* Unlimited undos and redos.
* Hot swappable prompt string.

Future features:

* Line editing with readline or linenoise.
* "edit current line" 

`edd` shares its syntax and idiosyncracies (to some extent) with ed. The ed
[manual](https://www.gnu.org/software/ed/manual/ed_manual.html) (or `man ed`)
sufficiently describes and explains how `ed` and, by extension, `edd`, works. 

## Differences and `edd`'isms

1. There are no special commands (`h` and `H`) for "Error Explanations," errors
are succintly explained as they occur.

2. Syntax is liberal. For example,
	`s/RE/SUBS/FLAG`
is legal in `ed` but 
	`s /RE/SUBS/FLAG`
is not. In `edd`, however, this is allowed. `edd` allows any number of space (or
tabs) between commands and their arguments.

3. Command `l` has been omitted.

4. Command `U` does redos.

5. Like `ed`, recursively calling any global command (g,G,v,V) is prohibited.
   This, however, does not prevent the parent global command to execute all the 
legal commands that come before it. For example,
```
	g/RE/a \
	line 1\
	line 2\
	.\
	g/RE/
```
will append "line 1" and "line 2" but signal an error for illegal command in the
command list once it encounters 'g'.

6. `edd` does not support all command line options that `ed` does. `edd -h`
   lists available options.

7. Like `ed`, the prompt string can be set with the `-p` option. And while the
   program is running, can be changed with the `P` command. `P arg` sets the 
current prompt to `arg`. "Hot swappable prompt string" is a fancy description of
this feature.

## Install 

```
git clone https://github.com/bojle/edd.git
cd edd
make
sudo make install
make clean
```

## Uninstall

```
cd dir/where/you/installed/edd
make unistall
```

## Todo

1. Add features mentioned in the future features section.
2. Thorough testing.

## Reporting Bugs

Report bugs at [https://github.com/bojle/edd/issues](https://github.com/bojle/edd/issues).

Any improvement ideas are welcome.
