# FMV Runtime Tracing

## Purpose

Identify code paths used specifically during Um Jammer Lammy's
prerecorded FMV playback.

Initial experiments use the native PSXRecomp build as a dynamic-analysis
environment for the PAL SCES-01753 release.

## Environment

Game:

    Um Jammer Lammy (Europe)
    Disc ID: SCES-01753

Runtime:

    Um Jammer Lammy Recompiled
    Native Linux x86-64 build
    OpenBIOS
    OpenGL renderer

Debugger:

    GNU GDB 18.1

The tested release build was compiled with:

    PSX_NATIVE_BUILD=1
    PSX_HAS_GAME_CODEGEN=1
    PSX_HAS_GAME_DISPATCH=1
    PSX_NO_DEBUG_TOOLS=1

Because `PSX_NO_DEBUG_TOOLS` disables the runtime TCP debug server,
GDB was used for the initial experiment.

## Native-code verification

Before using PSXRecomp for runtime RE, we verified that generated UJL
code actually executes natively.

The executable contains native symbols such as:

    func_800139F8
    func_8004CFC0
    ...

No Beetle/libretro symbols were found using the initial symbol search.

GDB recognizes the generated functions as native ELF symbols.

Example:

    info address func_800139F8

reported a native address for the symbol.

During Jet Baby playback, interrupting execution produced:

    #0 psx_cyc_charge()
    #1 func_8004CFC0()
    #2 psx_dispatch_game_compiled()
    #3 dirty_ram_dispatch()
    #4 psx_dispatch_impl()
    #5 psx_scheduler_run()
    #6 main()

Disassembling `func_8004CFC0` produced x86-64 instructions.

A breakpoint placed directly on `func_8004CFC0` was subsequently hit by
the running game.

This establishes that the generated function is part of live native
execution and is not merely unused generated code in the executable.

## Experiment: 0x8004CFC0

Breakpoint:

    break func_8004CFC0

### Observation 1 — Jet Baby

The breakpoint triggered almost immediately during the Jet Baby
prerecorded movie.

Repeated continuation caused repeated breakpoint hits throughout movie
playback.

Result:

    HIT — very frequent

### Observation 2 — Title/menu

The breakpoint was disabled to allow the game to leave the initial
movie sequence.

After reaching ordinary title/menu execution, it was re-enabled.

Result:

    No continuous hits observed

### Observation 3 — Chief Puddle demo

The game progressed into Stage 2 and played the Chief Puddle demo with
the breakpoint enabled.

The entire demo completed.

Result:

    NO HIT

This provides a useful negative control because the sequence is
noninteractive/scripted but is not the same kind of prerecorded movie
as the story FMVs.

### Observation 4 — Next prerecorded story FMV

During the transition/loading sequence preceding the next prerecorded
story movie, the breakpoint triggered again.

Once the movie was visibly playing, repeated continuation produced
continuous/frequent hits.

Result:

    HIT — very frequent

## Automatic breakpoint trace

The following GDB breakpoint commands were used:

    commands 1
    silent
    bt 3
    continue
    end

During movie playback this repeatedly produced:

    #0 func_8004CFC0()
    #1 psx_dispatch_game_compiled()
    #2 dirty_ram_dispatch()

The test was manually interrupted with Ctrl+C.

## Interpretation

The experiment establishes a strong correlation between
`0x8004CFC0` and prerecorded movie playback.

It does NOT yet establish:

- that the function itself is the movie player;
- that it performs MDEC decoding;
- that it executes exactly once per video frame;
- that it performs CD-ROM streaming;
- that it is exclusive to FMV playback under every possible game state.

The observed invocation rate is described as "very frequent", not
"once per frame", until cadence is measured.

## Follow-up experiments

1. Identify the purpose of `0x8004D24C`.
2. Inspect arguments supplied to `0x8004CFC0`.
3. Record invocation counts over a measured interval.
4. Correlate execution with CD-ROM reads.
5. Correlate execution with MDEC activity.
6. Test additional story FMVs.
7. Test gameplay, menus, loading screens, and other non-FMV states as
   negative controls.
8. Compare equivalent behavior against the US release.
