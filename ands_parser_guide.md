# ANDS Parser & Macros Guide

The `ands` parser used in Skode is a highly efficient, event-driven parser with some unique and powerful quirks. Understanding how it evaluates commands, buffers data, and processes macros is essential for writing advanced Skode scripts.

## 1. The Delayed Execution Model (One-Step-Behind)

The most important concept to understand about the `ands` parser is that it executes atoms (commands) **lazily**. 

When the parser scans a line of code from left to right:
1. When it encounters an **atom** (like `/SS` or `+`), it *does not* execute it immediately. Instead, it caches that atom as "pending".
2. It continues parsing the next tokens (numbers, arrays, strings) and pushes them into temporary buffers.
3. The pending atom is finally forced to execute only when the parser hits the *next* atom, or when it reaches the end of the chunk (a semicolon `;`, a newline, or EOF).

Because of this, both prefix and postfix styles can appear to work for single commands:
```skred
/SS 0 ( 1 2 3 )  # Preferred: The command comes first, followed by its arguments.
0 ( 1 2 3 ) /SS  # Also valid! The buffers accumulate exactly the same way.
```

## 2. Parser State Buffers

The parser maintains three primary states that persist until overwritten:
- **Numeric Argument Stack**: Numbers (`0`, `3.14`) are pushed here. Cleared after an atom executes.
- **Array Buffer**: Created using `( ... )`. Only one array is held at a time. It persists across commands until a new array is parsed.
- **String Buffer**: Created using `[ ... ]`. Like arrays, it persists until a new string is parsed.

## 3. Text Macros

Macros are purely text-substitution replacements handled by the preprocessor before the main parser even runs. 

### Defining a Macro
```skred
[mac] : [Hello world] ( 10 20 30 ) ;
```
- **Syntax**: `[name] : body ;`
- **Naming Rule**: Macro names are **strictly limited to 4 characters**. They can only consist of valid "atom" characters. See the ASCII Symbol Reference section below for exactly which characters are allowed. Numbers are forbidden.

### Macro Arguments (`$$N`)
Macros can accept arguments by placing `$$0`, `$$1`, etc., in the body. The preprocessor will look at the highest number to determine how many arguments the macro requires.

When you call the macro, it greedily grabs the next contiguous blocks of text—even entire strings or arrays!

```skred
[foo] : /SS 0 $$0 ;
foo ( 1 2 3 )
```
*Expands to:* `/SS 0 ( 1 2 3 )`

## 4. NaN Placeholders (`-` and `.`)

When the parser sees characters that *look* like they might start a number (specifically `-` and `.`), it attempts to parse them into the numeric stack. 

If you type one of these characters by itself (e.g., just `-`), the parser evaluates it as `NaN` (Not a Number) and pushes `NaN` onto the numeric stack. 

Skode utilizes this `NaN` behavior heavily as a **"keep existing value" sentinel** for commands that take many arguments. For example, the `DL` (delay) command allows you to selectively update only specific parameters by passing `-` for the ones you want to ignore:
```skred
# Update only the feedback (3rd argument), leave length and time alone
- - 0.75 DL 
```

## 5. Runtime String Formatting & Returns

While macros handle text-substitution at parse time, you often need runtime evaluation.

### String Formatting (`s%`)
You can dynamically format strings using the `@0`, `@1` placeholders and the `s%` command.
```skred
[The result is @0 and @1] 42 69 s% ?s
```
This evaluates the string buffer, consumes `42` and `69` from the numeric stack, and replaces `@0` and `@1`.

### Macro Return Values (`*R`)
If a macro needs to pass numeric calculations back to the caller, it can use the `*R` command. `*R` takes the current numeric stack and permanently saves it into a special "return array".

You can read those values back out in subsequent commands using `@0`, `@1`...

```skred
[calc] : 42 69 *R ;
[show] : @0 @1 s% ?s ;

calc
[Values: @0, @1] show
```

## 6. Critical Gotchas

1. **Multiple Commands on One Line Can Overwrite Buffers:**
   Because of the delayed execution model, chaining commands can cause unexpected buffer overwrites:
   ```skred
   ( 10 20 ) /SS 3 ( 30 40 ) /SS 4
   ```
   *What happens:* `/SS` is cached. Then `( 30 40 )` is parsed, which *overwrites* `( 10 20 )` in the array buffer. Then the second `/SS` is encountered, forcing the first `/SS` to execute using `( 30 40 )`!
   *Fix:* Separate them with semicolons or newlines to force execution before continuing.

2. **Atoms Clear the Return Registers:**
   The parser wipes the return registers clean immediately *before* executing any new atom. 
   If you want to read values returned by a previous macro, you **must** read them before triggering any new commands:
   ```skred
   [bad]  : /SS 0 @0 @1 s% ; # Fails! /SS executes and clears @0 and @1.
   [good] : @0 @1 s% /SS 0 ; # Works! @0 and @1 are read and safely pushed to the stack first.
   ```


## 7. Deferred Execution (`+` and `~`)

Skode provides two special sigils that let you defer the execution of a line of code into the future. They instruct the parser to compile the remainder of the line into a background event and schedule it.

- `+` defers by **beats** (quarter notes based on the current tempo).
- `~` defers by **seconds**.

```skred
+ 4 /SS 0 ( 1 2 3 ) # Executes "/SS 0 ( 1 2 3 )" exactly 4 beats from now
~ 1.5 f 440         # Executes "f 440" exactly 1.5 seconds from now
```

> [!WARNING]
> You **cannot** defer just anything! Because deferred code is sent to the background audio compiler, it can **only contain Real-Time Safe commands**. If you try to defer an Immediate-Only command (like `?S`, `d!`, or `s%`), it will be rejected. See **Section 9** for more details.

When the parser sees a defer sigil, it immediately forces any pending atom to execute, and then captures the number and the rest of the chunk as a string to be scheduled. 

**Pro Tip:** Because the defer string is terminated by a "chunk end" (a semicolon `;` or a newline), you can use a semicolon to break out of the defer and execute code immediately on the same line!

```skred
# Defers running `mac` by 4 beats, but executes `0.1 p` IMMEDIATELY
+ 4 mac ; 0.1 p
```

## 8. State Sigils: Variables (`$`) and Streams (`&`)

There are two other major sigils integrated directly into the `ands` token loop: variables (`$1`, `$2`) and streams (`&1`, `&2`). 

When the parser sees one of these, it doesn't treat it as a standalone command (atom). Instead, it attaches metadata to the numeric argument stack.
- `$N` reads the value of variable N and pushes it to the stack, but also records the variable ID in `arg_var`. This allows commands like `=` (assignment) to know *which* variable you meant to overwrite.
- `&N` pushes `0.0` to the stack, but sets a special `ANDS_STREAM_FLAG` in `arg_var`. This is how audio commands know you want to modulate a parameter with a stream rather than setting it to a static value!

---

## 9. The Compiler vs. The Interpreter

In Skode, execution happens in two distinctly different modes depending on how you invoke a command. Understanding the pass-off between the live interpreter and the background compiler is critical for advanced sequencing.

### Immediate Execution (The Interpreter)
When you type a command directly into the console or execute a raw macro, the parser runs in **Immediate Mode**. 
- It maintains the active state buffers (the string buffer, the array buffer, and the return registers).
- It can execute *any* command in the dictionary, including `?S`, `s%`, `*R`, etc.

### Deferred Execution (The Compiler)
When you schedule a command for the future (e.g., using the `+` or `~` defer sigils, or repeating a macro with `RR`), the parser string is passed off to the **Background Compiler** (`skode_compile_scheduled`).

The compiler's job is to translate that text into a highly optimized sequence of binary opcodes (`event_program_t`) that the audio thread can execute safely in the background. Because the audio thread cannot safely allocate memory, manipulate strings, or interact with the REPL state, **the compiler is extremely strict about what it accepts:**

1. **No State Buffers:** If the compiler encounters a string `[...]`, an array `(...)`, or an attempt to read a return register `@N`, it will immediately reject the line as `IMMEDIATE_ONLY` and refuse to schedule it.
2. **Safe Words Only:** Commands in the dictionary are explicitly tagged by the developer as either `WORD_REAL_TIME_SAFE` or `WORD_IMMEDIATE_ONLY`. Audio commands (like `/SS`, filter settings, oscillator pitches) are safe. Utility commands (like `s%`, `?M`, file loading, macro definitions) are immediate-only and will fail to compile.

If you ever see the error `# command is not schedulable`, it means you tried to defer or schedule a line of code that contained an array, a string, a return read, or an immediate-only dictionary word!

## 10. The Semicolon (Chunk Terminator)
While the semicolon `;` is critical for terminating a `+` or `~` defer block, its primary role is as a **Chunk Terminator**. 

Because `ands` executes atoms one step behind, an atom doesn't execute until the parser hits the *next* token, a newline, or a semicolon. The semicolon explicitly ends the current chunk of execution. 
- In immediate execution, it forces the pending atom to execute immediately.
- In macro definitions (`[name] : body ;`), the semicolon tells the parser to stop recording the macro body and return to normal execution.


## 11. Complete ASCII Symbol Reference
The parser categorizes all printable ASCII symbols into two strict groups: **Allowed** (can be used to name macros and atoms) and **Forbidden** (reserved by the parser for special syntax). 

### Allowed (Valid Atom Characters)
You may use any alphanumeric letter (`a-z`, `A-Z`) and the following 18 symbols to name your macros:
`!`, `%`, `^`, `*`, `_`, `=`, `:`, `"`, `'`, `<`, `>`, `?`, `/`, `\`, `|`, `` ` ``

### Forbidden (Special Syntax)
The following symbols are structurally reserved by the parser. **They cannot be used in macro names.**

| Symbol | Purpose in Skode |
|--------|------------------|
| `0-9` | Numbers. |
| `-` `.` | Numbers, decimals, or the `NaN` sentinel. |
| `;` | Chunk Terminator (forces execution, ends macro definitions, ends defer blocks). |
| `[ ]` | String buffer / Macro definition block. |
| `( )` | Array buffer. |
| `$` | Variable read sigil (e.g., `$1`). |
| `&` | Stream read sigil (e.g., `&1`). |
| `@` | Return register read sigil (e.g., `@1`). |
| `+` | Defer execution by beats. |
| `~` | Defer execution by seconds. |
| `#` | Comment (ignores the rest of the line). |
| `{ }` | Reserved for future block-scoping use (currently invalid). |
| `,` | Alias for space / whitespace token separator. |



## 12. Meta-Commands (The `-` and `.` prefixes)

There is an emerging idiom in the broader ecosystem regarding lines that begin with `.` (period) or `-` (dash) after initial whitespace. 

Because a line starting with `.` or `-` would ordinarily parse as a floating-point number, it is safely treated as an out-of-band **meta-command flag** by the surrounding environment (e.g., the editor or wrapper script) and is **not sent** to the Skode/Skred engine at all.

For example, a line like:
```text
-restart
```
...is intercepted by the host environment to shut down and restart the current Skode/Skred instance with different parameters, bypassing the internal parser entirely.


## Addendum: ANDS vs. Forth, Tcl, Lisp, and Erlang/Elixir

If you are coming from other text-oriented languages, the `ands` parser will feel familiar but possesses some fundamental differences.

### vs. Forth
- **Similarities:** Space-separated tokens, seemingly postfix syntax (`1 2 +`), no mandatory grouping or punctuation.
- **Differences:** **ANDS is not stack-based!** In Forth, you push numbers to a global data stack, and commands consume them. In ANDS, numeric arguments are buffered locally for the *currently pending atom*. Once the atom executes, the numeric argument buffer is wiped. Additionally, Forth executes words immediately; ANDS uses the delayed "one-step-behind" execution model.

### vs. Tcl
- **Similarities:** Both treat macros and substitution as pure text manipulation operations that happen before the code is "run."
- **Differences:** Tcl evaluates nested commands in-place (e.g., `set x [expr 1 + 2]`). ANDS macros (`$$N`) are purely text copy-paste operations, and runtime evaluation (like `s%`) requires explicit commands to modify the active state buffers. ANDS does not evaluate arbitrary strings inline.


### vs. Erlang / Elixir
- **Similarities:** Skode shares a fundamental architectural philosophy with the BEAM VM: a strong separation between the interactive layer and concurrent "processes." When you use defer sigils (`+`, `~`), Skode compiles your command into an isolated event program and "sends it as a message" to be executed by a specific synthesizer voice on the real-time audio thread, much like sending a message to a PID in Erlang.
- **Differences (Parsing):** Elixir uses a sophisticated Abstract Syntax Tree (AST) where everything is an expression that returns a value. Skode has **no expressions and no AST**. In Skode, parsing is a destructive, mutable process: executing a command permanently consumes the numeric arguments buffered for it, and there is no concept of "returning a value" to an outer function (except manually pushing numbers into the return array `@N`). Furthermore, Elixir evaluates eagerly, whereas Skode's token evaluation is strictly delayed by one step.

### vs. Lisp
- **Similarities:** Prefix commands are supported (e.g., `/SS 0`).
- **Differences:** Lisp uses parentheses to define a deeply nested Abstract Syntax Tree (AST) where expressions evaluate into other expressions. ANDS has **no AST**. The execution model is completely flat and linear from left to right. In ANDS, parentheses `( ... )` simply load numbers into an array buffer—they do not dictate execution order or scoping.


