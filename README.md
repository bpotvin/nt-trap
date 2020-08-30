# NT-TRAP

I was recently working to get the QED editor running on NT. A fairly detailed history of the editor, [An Incomplete Hisotry of QED](https://www.bell-labs.com/usr/dmr/www/qed.html) was written by dmr, if you're interested in more information. The editor has a few functions that needed a bit of extra work because they used fork and exec calls, 'bang', 'crunch' and 'zap':
```
  b    ! - runs a command.
  c    < - takes stdout from a command and reads it into the edit buffer.
  z    > - takes the edit buffer and pushes it to the command's stdin.
```
The crunch function made me think of the [TSO/E Rexx OUTTRAP](https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.ikja300/outtrap.htm) function, which is very roughly similar to the script(1) utility, that populates variables accessible to a Rexx EXEC with output from your TSO console session. It's a useful external function and I implemented it for Rexx on NT in 1994.

The code here re-implements part of my 1994 code and adds the ability to push data to a spawn'd process's standard input. The code organization is a bit odd; it was pulled from a larger library and factored into this stand-alone example.

To demonstrate the function, three console utilities are used: rev(1), seq(1) and ls(1). If you don't have those utilities on your system, update the examples to use the utilities you prefer. A short video is included that shows the code running here.

## USAGE

One function and two structures are provided for user programs.

### **_TrapOuttrap**

Captures console output from the specified command.

### *Syntax*
```
BOOL
NTAPI
_TrapOuttrap (
    char* cmdline,
    PTRAP_INFO pparam
    );
```
### *Parameters*

**cmdline**<br>
The command line to run.

**pinfo**<br>
A pointer to a *TRAP_INFO* structure. The structure is used to provide input for the child process and to return the console output from the specified command.

### *Return Value*
If the function succeeds, the return value is nonzero.

If the function fails, the return value is zero (0). To get extended error information, call GetLastError.

### **TRAP_INFO**
Provide input and output for the **_TrapOuttrap** function.
### *Syntax*
```
typedef struct _TRAP_INFO
{
    DWORD flags;
    DWORD status;
    TRAP_BUFFER input;
    TRAP_BUFFER output;
} TRAP_INFO, *PTRAP_INFO;
```
### *Members*

**flags**<br>
Option flags:

FLAG | DESCRIPTION | VALUE
-----|-------------|------
_TRAP_DEFAULT | do-nothing placeholder | 0x00000000
_TRAP_INPUT_FILENAME | the input buffer holds a filename | 0x00000001

**status**<br>
Child process return code.

**input**<br>
A *TRAP_BUFFER* structure specifying input for the child process.

**output**<br>
A *TRAP_BUFFER* structure that will hold the child process output.

### *Remarks*
On input to the _TrapOuttrap function, the .flags and .input fields are the only ones that might need to be touched. When the function returns, the .status field will hold the return code from the child process and the .output field will have the output from the child process.

Storage for the child command console output is allocated using Win32 Heap functions and must be free'd using the **HeapFree** function.

### **TRAP_BUFFER**
```
typedef struct _TRAP_BUFFER
{
    ULONG length;
    ULONG max_length;
    char* buffer;
} TRAP_BUFFER, *PTRAP_BUFFER;
```
### *Members*
**length**<br>

The length, in bytes, of the string stored in .buffer.

**max_length**<br>

The length, in bytes, of .buffer.

**buffer**<br>

Pointer to a buffer used to hold a string.

### *Remarks*
The *TRAP_BUFFER* structure is sort of like an ANSI_STRING structure with the length fields extended to 32-bits.

## Build
The code can be built as a console or a graphical app.

Open a "vc tools" command prompt, either 32-bit or 64-bit, change to the directory containing the r0.c file and then:
```
# cl -W4 r0.c
```
For a graphical application, use:
```
# cl -W4 -D_TRAP_GUI r0.c -link user32.lib
```

## Files
The following files are included:
```
NT-VER
|   outtrap-test.mp4            short video of build/run.
|   r0.c                        source.
\   README.md                   this.
```
That is all.
```
  SR  15,15
  BR  14
```
