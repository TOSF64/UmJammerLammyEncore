# PSXRecomp Runtime Notes

## Purpose

PSXRecomp is currently useful to Encore as a dynamic-analysis
environment.

Encore does not depend on PSXRecomp as its eventual implementation.
Instead, the recompiled runtime provides a convenient way to observe
the original game's behavior while reverse engineering SCES-01753.

## Generated game code

The PAL build generates game code into files named approximately:

    generated/SCES_017.53_full_00.c
    ...
    generated/SCES_017.53_full_21.c

Generated functions retain names derived from their original
PlayStation addresses:

    func_800139F8
    func_8004CFC0
    ...

For example:

    func_8004CFC0

corresponds to original guest address:

    0x8004CFC0

The native address of the generated function is unrelated and is
assigned by the host executable/linker.

## Native symbols

The release executable is not stripped, although it does not contain
full source-level debugging information.

Useful command:

    nm -C build-release/UmJammerLammy_Recompiled |
      grep -E 'func_800|SCES|recomp'

GDB can resolve generated function symbols:

    info address func_8004CFC0

and disassemble their native implementations:

    disassemble func_8004CFC0

The resulting assembly is host x86-64 code.

## Running under GDB

On systems using Yama with:

    /proc/sys/kernel/yama/ptrace_scope = 1

attaching GDB to an independently launched process may fail with:

    ptrace: Operation not permitted

There is no need to disable this protection.

Instead, launch the game as a child of GDB:

    cd ~/UmJamLamRecomp/build-release

    gdb --args ./UmJammerLammy_Recompiled \
      --disc "/path/to/game.cue"

Then:

    (gdb) run

Interrupt execution using Ctrl+C in the GDB terminal.

Useful commands:

    bt
    x/10i $pc
    info registers
    info address func_XXXXXXXX
    disassemble func_XXXXXXXX

## Breakpointing generated game functions

Example:

    break func_8004CFC0
    continue

If execution reaches the generated function, GDB will break at its
native host address.

This is useful for determining which original game functions are active
during specific controlled game states.

## Automatic breakpoint tracing

For a breakpoint that fires repeatedly:

    commands 1
    silent
    bt 3
    continue
    end

WARNING: hot functions can generate enormous amounts of terminal output.

Interrupt automatic continuation using Ctrl+C.

Disable the breakpoint with:

    disable 1

Re-enable with:

    enable 1

## Dispatch stack

A generated game function may appear in a host stack resembling:

    func_XXXXXXXX
    psx_dispatch_game_compiled
    dirty_ram_dispatch
    psx_dispatch_impl
    psx_scheduler_run
    main

The presence of `dirty_ram_dispatch` or interpreter-support code in the
runtime must not by itself be interpreted as evidence that all game code
is being interpreted.

Generated game functions have been directly observed executing as
native host code.

## Debug server

The runtime source contains a native TCP debug server.

Default native port:

    4370

The tested release build does NOT expose it because the build defines:

    PSX_NO_DEBUG_TOOLS=1

The initialization site is guarded approximately as:

    #ifndef PSX_NO_DEBUG_TOOLS
        debug_server_init(debug_port);
    #else
        (void)debug_port;
    #endif

Therefore:

    nc -vz 127.0.0.1 4370

returns connection refused for the tested release build.

This is expected for that configuration.

A separate debug-enabled development build should eventually be created
rather than modifying the known-good release build.

## Beetle oracle

PSXRecomp also contains a separate Beetle-based oracle/debug target.

Do not confuse the existence of this oracle with the native recompiled
runtime.

During investigation of the native UJL executable:

- generated UJL native symbols were present;
- generated UJL objects were linked into the executable;
- no Beetle/libretro symbols were found by the initial symbol search;
- GDB directly observed generated UJL functions executing.

The oracle remains potentially useful for behavioral comparison.

## CD-ROM tracing

The PSXRecomp runtime source contains CD-read history support exposed
through the debug command:

    cd_read_log

This records CD DMA transfers including information such as:

- LBA;
- destination RAM address;
- transfer size.

This functionality is currently unavailable in the tested release build
because the debug server is compiled out.

Future work:

1. create a separate debug-enabled runtime build;
2. verify port 4370;
3. exercise `cd_read_log`;
4. correlate observed LBAs with Encore's ISO 9660 filesystem map;
5. record controlled transitions such as boot, title, FMV, stage load,
   and gameplay.

## RE discipline

Runtime observations should be recorded separately from semantic
interpretations.

Prefer:

    0x8004CFC0
    Observed: executes frequently during prerecorded FMVs.
    Hypothesis: belongs to FMV subsystem.

Avoid prematurely recording:

    0x8004CFC0 = PlayMovie()

Function names should become increasingly semantic only as evidence
accumulates.
