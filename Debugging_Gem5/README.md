# Debugging gem5
In the previous chapters we covered how to create a very simple SimObject. In this chapter, we will replace the simple print to stdout with gem5’s debugging support.

gem5 provides support for printf-style tracing/debugging of your code via debug flags. These flags allow every component to have many debug-print statements, without all of them enabled at the same time. When running gem5, you can specify which debug flags to enable from the command line.

## Using debug flags
For instance, when running the first simple.py script from simple-config-chapter, if you enable the DRAM debug flag, you get the following output. Note that this generates a lot of output to the console (about 7 MB).

```
build/X86/gem5.opt --debug-flags=DRAM configs/learning_gem5/part1/simple.py | head -n 50
```

```
Global frequency set at 1000000000000 ticks per second
warn: No dot file generated. Please install pydot to generate the dot file and pdf.
      0: system.mem_ctrl.dram: Setting up DRAM Interface
      0: system.mem_ctrl.dram: Creating DRAM rank 0 
      0: system.mem_ctrl.dram: Creating DRAM rank 1 
build/X86/mem/dram_interface.cc      0: system.mem_ctrl.dram: Memory capacity 536870912 (536870912) bytes
:690: warn: DRAM device capacity (8192 Mbytes) does not match the address range assigned (512 Mbytes)
      0: system.mem_ctrl.dram: Row buffer size 8192 bytes with 128 bursts per row buffer
0: system.remote_gdb: listening for remote gdb on port 7000
build/X86/sim/simulate.cc:194: info: Entering event queue @ 0.  Starting simulation...
gem5 Simulator System.  https://www.gem5.org
gem5 is copyrighted software; use the --copyright option for details.

gem5 version 22.0.0.2
gem5 compiled Dec  7 2022 08:53:46
gem5 started Dec  7 2022 10:55:18
gem5 executing on ubuntu, pid 2493
command line: build/X86/gem5.opt --debug-flags=DRAM configs/learning_gem5/part1/simple.py

Beginning simulation!
      0: system.mem_ctrl.dram: Address: 0x190 Rank 0 Bank 0 Row 0
      0: system.mem_ctrl.dram: Timing access to addr 0x190, rank/bank/row 0 0 0
      0: system.mem_ctrl.dram: Activate at tick 0
      0: system.mem_ctrl.dram: Activate bank 0, rank 0 at tick 0, now got 1 active
      0: system.mem_ctrl.dram: Schedule RD/WR burst at tick 27500
  46250: system.mem_ctrl.dram: number of read entries for rank 0 is 0
  77000: system.mem_ctrl.dram: Address: 0x190 Rank 0 Bank 0 Row 0
  77000: system.mem_ctrl.dram: Timing access to addr 0x190, rank/bank/row 0 0 0
  77000: system.mem_ctrl.dram: Schedule RD/WR burst at tick 77000
  95750: system.mem_ctrl.dram: number of read entries for rank 0 is 0
 126000: system.mem_ctrl.dram: Address: 0x190 Rank 0 Bank 0 Row 0
 126000: system.mem_ctrl.dram: Timing access to addr 0x190, rank/bank/row 0 0 0
 126000: system.mem_ctrl.dram: Schedule RD/WR burst at tick 126000
 144750: system.mem_ctrl.dram: number of read entries for rank 0 is 0
 175000: system.mem_ctrl.dram: Address: 0x94db0 Rank 1 Bank 2 Row 4
 175000: system.mem_ctrl.dram: Timing access to addr 0x94db0, rank/bank/row 1 2 4
 175000: system.mem_ctrl.dram: Activate at tick 175000
 175000: system.mem_ctrl.dram: Activate bank 2, rank 1 at tick 175000, now got 1 active
 175000: system.mem_ctrl.dram: Schedule RD/WR burst at tick 188750
 207500: system.mem_ctrl.dram: number of read entries for rank 1 is 0
 238000: system.mem_ctrl.dram: Address: 0x190 Rank 0 Bank 0 Row 0
 238000: system.mem_ctrl.dram: Timing access to addr 0x190, rank/bank/row 0 0 0
 238000: system.mem_ctrl.dram: Schedule RD/WR burst at tick 238000
 256750: system.mem_ctrl.dram: number of read entries for rank 0 is 0
 287000: system.mem_ctrl.dram: Address: 0x198 Rank 0 Bank 0 Row 0
 287000: system.mem_ctrl.dram: Timing access to addr 0x198, rank/bank/row 0 0 0
 287000: system.mem_ctrl.dram: Schedule RD/WR burst at tick 287000
 305750: system.mem_ctrl.dram: number of read entries for rank 0 is 0
 336000: system.mem_ctrl.dram: Address: 0x198 Rank 0 Bank 0 Row 0
 336000: system.mem_ctrl.dram: Timing access to addr 0x198, rank/bank/row 0 0 0
 336000: system.mem_ctrl.dram: Schedule RD/WR burst at tick 336000
 354750: system.mem_ctrl.dram: number of read entries for rank 0 is 0
 385000: system.mem_ctrl.dram: Address: 0x198 Rank 0 Bank 0 Row 0
 385000: system.mem_ctrl.dram: Timing access to addr 0x198, rank/bank/row 0 0 0
Exception ignored in: <_io.TextIOWrapper name='<stdout>' mode='w' encoding='utf-8'>
BrokenPipeError: [Errno 32] Broken pipe
```
 
Or, you may want to debug based on the exact instruction the CPU is executing. For this, the Exec debug flag may be useful. This debug flags shows details of how each instruction is executed by the simulated CPU.
 
```
build/X86/gem5.opt --debug-flags=Exec configs/learning_gem5/part1/simple.py | head -n 50
```
 
```
gem5 Simulator System.  http://gem5.org
gem5 is copyrighted software; use the --copyright option for details.

gem5 compiled Jan  3 2017 16:03:38
gem5 started Jan  3 2017 16:11:47
gem5 executing on chinook, pid 19234
command line: build/X86/gem5.opt --debug-flags=Exec configs/learning_gem5/part1/simple.py

Global frequency set at 1000000000000 ticks per second
      0: system.remote_gdb.listener: listening for remote gdb #0 on port 7000
warn: ClockedObject: More than one power state change request encountered within the same simulation tick
Beginning simulation!
info: Entering event queue @ 0.  Starting simulation...
  77000: system.cpu T0 : @_start    : xor   rbp, rbp
  77000: system.cpu T0 : @_start.0  :   XOR_R_R : xor   rbp, rbp, rbp : IntAlu :  D=0x0000000000000000
 132000: system.cpu T0 : @_start+3    : mov r9, rdx
 132000: system.cpu T0 : @_start+3.0  :   MOV_R_R : mov   r9, r9, rdx : IntAlu :  D=0x0000000000000000
 187000: system.cpu T0 : @_start+6    : pop rsi
 187000: system.cpu T0 : @_start+6.0  :   POP_R : ld   t1, SS:[rsp] : MemRead :  D=0x0000000000000001 A=0x7fffffffee30
 250000: system.cpu T0 : @_start+6.1  :   POP_R : addi   rsp, rsp, 0x8 : IntAlu :  D=0x00007fffffffee38
 250000: system.cpu T0 : @_start+6.2  :   POP_R : mov   rsi, rsi, t1 : IntAlu :  D=0x0000000000000001
 360000: system.cpu T0 : @_start+7    : mov rdx, rsp
 360000: system.cpu T0 : @_start+7.0  :   MOV_R_R : mov   rdx, rdx, rsp : IntAlu :  D=0x00007fffffffee38
 415000: system.cpu T0 : @_start+10    : and    rax, 0xfffffffffffffff0
 415000: system.cpu T0 : @_start+10.0  :   AND_R_I : limm   t1, 0xfffffffffffffff0 : IntAlu :  D=0xfffffffffffffff0
 415000: system.cpu T0 : @_start+10.1  :   AND_R_I : and   rsp, rsp, t1 : IntAlu :  D=0x0000000000000000
 470000: system.cpu T0 : @_start+14    : push   rax
 470000: system.cpu T0 : @_start+14.0  :   PUSH_R : st   rax, SS:[rsp + 0xfffffffffffffff8] : MemWrite :  D=0x0000000000000000 A=0x7fffffffee28
 491000: system.cpu T0 : @_start+14.1  :   PUSH_R : subi   rsp, rsp, 0x8 : IntAlu :  D=0x00007fffffffee28
 546000: system.cpu T0 : @_start+15    : push   rsp
 546000: system.cpu T0 : @_start+15.0  :   PUSH_R : st   rsp, SS:[rsp + 0xfffffffffffffff8] : MemWrite :  D=0x00007fffffffee28 A=0x7fffffffee20
 567000: system.cpu T0 : @_start+15.1  :   PUSH_R : subi   rsp, rsp, 0x8 : IntAlu :  D=0x00007fffffffee20
 622000: system.cpu T0 : @_start+16    : mov    r15, 0x40a060
 622000: system.cpu T0 : @_start+16.0  :   MOV_R_I : limm   r8, 0x40a060 : IntAlu :  D=0x000000000040a060
 732000: system.cpu T0 : @_start+23    : mov    rdi, 0x409ff0
 732000: system.cpu T0 : @_start+23.0  :   MOV_R_I : limm   rcx, 0x409ff0 : IntAlu :  D=0x0000000000409ff0
 842000: system.cpu T0 : @_start+30    : mov    rdi, 0x400274
 842000: system.cpu T0 : @_start+30.0  :   MOV_R_I : limm   rdi, 0x400274 : IntAlu :  D=0x0000000000400274
 952000: system.cpu T0 : @_start+37    : call   0x9846
 952000: system.cpu T0 : @_start+37.0  :   CALL_NEAR_I : limm   t1, 0x9846 : IntAlu :  D=0x0000000000009846
 952000: system.cpu T0 : @_start+37.1  :   CALL_NEAR_I : rdip   t7, %ctrl153,  : IntAlu :  D=0x00000000004001ba
 952000: system.cpu T0 : @_start+37.2  :   CALL_NEAR_I : st   t7, SS:[rsp + 0xfffffffffffffff8] : MemWrite :  D=0x00000000004001ba A=0x7fffffffee18
 973000: system.cpu T0 : @_start+37.3  :   CALL_NEAR_I : subi   rsp, rsp, 0x8 : IntAlu :  D=0x00007fffffffee18
 973000: system.cpu T0 : @_start+37.4  :   CALL_NEAR_I : wrip   , t7, t1 : IntAlu :
1042000: system.cpu T0 : @__libc_start_main    : push   r15
1042000: system.cpu T0 : @__libc_start_main.0  :   PUSH_R : st   r15, SS:[rsp + 0xfffffffffffffff8] : MemWrite :  D=0x0000000000000000 A=0x7fffffffee10
1063000: system.cpu T0 : @__libc_start_main.1  :   PUSH_R : subi   rsp, rsp, 0x8 : IntAlu :  D=0x00007fffffffee10
1118000: system.cpu T0 : @__libc_start_main+2    : movsxd   rax, rsi
1118000: system.cpu T0 : @__libc_start_main+2.0  :   MOVSXD_R_R : sexti   rax, rsi, 0x1f : IntAlu :  D=0x0000000000000001
1173000: system.cpu T0 : @__libc_start_main+5    : mov  r15, r9
1173000: system.cpu T0 : @__libc_start_main+5.0  :   MOV_R_R : mov   r15, r15, r9 : IntAlu :  D=0x0000000000000000
1228000: system.cpu T0 : @__libc_start_main+8    : push r14
Exception ignored in: <_io.TextIOWrapper name='<stdout>' mode='w' encoding='utf-8'>
BrokenPipeError: [Errno 32] Broken pipe

```

In fact, the Exec flag is actually an agglomeration of multiple debug flags. You can see this, and all of the available debug flags, by running gem5 with the --debug-help parameter.

```
build/X86/gem5.opt --debug-help
```

```
Base Flags:
    Activity: None
    AddrRanges: None
    Annotate: State machine annotation debugging
    AnnotateQ: State machine annotation queue debugging
    AnnotateVerbose: Dump all state machine annotation details
    BaseXBar: None
    Branch: None
    Bridge: None
    CCRegs: None
    CMOS: Accesses to CMOS devices
    Cache: None
    CacheComp: None
    CachePort: None
    CacheRepl: None
    CacheTags: None
    CacheVerbose: None
    Checker: None
    Checkpoint: None
    ClockDomain: None
...
Compound Flags:
    All: Controls all debug flags. It should not be used within C++ code.
        All Base Flags
    AnnotateAll: All Annotation flags
        Annotate, AnnotateQ, AnnotateVerbose
    CacheAll: None
        Cache, CacheComp, CachePort, CacheRepl, CacheVerbose, HWPrefetch
    DiskImageAll: None
        DiskImageRead, DiskImageWrite
...
XBar: None
    BaseXBar, CoherentXBar, NoncoherentXBar, SnoopFilter
```

## Adding a new debug flag
In the previous chapters, we used a simple std::cout to print from our SimObject. While it is possible to use the normal C/C++ I/O in gem5, it is highly discouraged. So, we are now going to replace this and use gem5’s debugging facilities instead.

When creating a new debug flag, we first have to declare it in a SConscript file. Add the following to the SConscript file in the directory with your hello object code (src/learning_gem5/).

```
DebugFlag('HelloExample')
```

This declares a debug flag of “HelloExample”. Now, we can use this in debug statements in our SimObject.

By declaring the flag in the SConscript file, a debug header is automatically generated that allows us to use the debug flag. The header file is in the debug directory and has the same name (and capitalization) as what we declare in the SConscript file. Therefore, we need to include the automatically generated header file in any files where we plan to use the debug flag.

In the hello_object.cc file, we need to include the header file.

```
#include "base/trace.hh"
#include "debug/HelloExample.hh"
```

Now that we have included the necessary header file, let’s replace the std::cout call with a debug statement like so.
 
```
DPRINTF(HelloExample, "Created the hello object\n");
```

DPRINTF is a C++ macro. The first parameter is a debug flag that has been declared in a SConscript file. We can use the flag HelloExample since we declared it in the src/learning_gem5/SConscript file. The rest of the arguments are variable and can be anything you would pass to a printf statement.

Now, if you recompile gem5 and run it with the “HelloExample” debug flag, you get the following result. 
 
```
build/X86/gem5.opt --debug-flags=HelloExample configs/learning_gem5/part2/hello.py
```
 
```
gem5 Simulator System.  http://gem5.org
gem5 is copyrighted software; use the --copyright option for details.

gem5 compiled Jan  4 2017 09:40:10
gem5 started Jan  4 2017 09:41:01
gem5 executing on chinook, pid 29078
command line: build/X86/gem5.opt --debug-flags=HelloExample configs/learning_gem5/part2/hello.py

Global frequency set at 1000000000000 ticks per second
      0: hello: Created the hello object
Beginning simulation!
info: Entering event queue @ 0.  Starting simulation...
Exiting @ tick 18446744073709551615 because simulate() limit reached
```

You can find the updated SConcript file [here](https://gem5.googlesource.com/public/gem5/+/refs/heads/stable/src/learning_gem5/part2/SConscript) and the updated hello object code [here](https://gem5.googlesource.com/public/gem5/+/refs/heads/stable/src/learning_gem5/part2/hello_object.cc).

## Debug output
For each dynamic DPRINTF execution, three things are printed to stdout. First, the current tick when the DPRINTF is executed. Second, the name of the SimObject that called DPRINTF. This name is usually the Python variable name from the Python config file. However, the name is whatever the SimObject name() function returns. Finally, you see whatever format string you passed to the DPRINTF function.

You can control where the debug output goes with the --debug-file parameter. By default, all of the debugging output is printed to stdout. However, you can redirect the output to any file. The file is stored relative to the main gem5 output directory, not the current working directory.

## Using functions other than DPRINTF
DPRINTF is the most commonly used debugging function in gem5. However, gem5 provides a number of other functions that are useful in specific circumstances.

These functions are like the previous functions :cppDDUMP, :cppDPRINTF, and :cppDPRINTFR except they do not take a flag as a parameter. Therefore, these statements will always print whenever debugging is enabled.

All of these functions are only enabled if you compile gem5 in “opt” or “debug” mode. All other modes use empty placeholder macros for the above functions. Therefore, if you want to use debug flags, you must use either “gem5.opt” or “gem5.debug”. 
 
## Extend Lab: Print down and understand the Debug output
In the Using debug flags part, we limited the output in to 50 lines, it is not helpful for a real debugging situation. To get all the information we want we cannot print everything on the terminal, we should use a file to get the debug output. 

Using the command to get the output into a txt file

```
build/X86/gem5.opt --debug-flags=DRAM --debug-file=debug.txt configs/learning_gem5/part1/simple.py
```

Then you can find the debug.txt in the m5out file. From the debug.txt find out 5 tick the DRAM is activated.
 