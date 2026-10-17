# Reverse Engineering

This directory contains reverse-engineering research for **Um Jammer Lammy: Encore!**

The primary reference release is currently:

- **Region:** Europe (PAL)
- **Disc ID:** SCES-01753

The goal of this research is to understand the behavior and structure of
the original game well enough to support an independent, portable
implementation while preserving the behavior of the original release.

This directory is a research notebook, not a declaration that every
documented hypothesis is correct.

## Research principles

Reverse-engineering notes should distinguish between three kinds of
information:

### Observed

Behavior or data directly demonstrated by the original game, disc,
executable, generated recompilation output, debugger, or another
reproducible experiment.

Example:

> `0x8004CFC0` executes very frequently during prerecorded story FMV
> playback.

### Inferred

An interpretation supported by observations but not yet demonstrated
directly.

Example:

> `0x8004CFC0` probably belongs to the prerecorded FMV subsystem.

### Unknown

Questions for which the available evidence is insufficient.

Example:

> It is not yet known whether `0x8004CFC0` performs video decoding,
> streaming, synchronization, or another FMV-related operation.

Whenever possible, preserve the evidence that produced an inference so
that conclusions can be revisited later.

## Directory structure

### `functions/`

Case files for individual functions or code regions discovered in the
original executable.

Function documents should record information such as:

- original PlayStation address;
- release/version;
- static observations;
- runtime observations;
- known callers or callees;
- working semantic labels;
- confidence levels;
- rejected hypotheses;
- unresolved questions.

Files are named using the original guest address where practical.

Example:

    functions/8004CFC0.md

### `experiments/`

Reproducible dynamic-analysis experiments.

These documents describe what was tested, how it was tested, what was
observed, and what conclusions the experiment does and does not support.

Examples include:

- runtime breakpoint tracing;
- CD-ROM access tracing;
- input experiments;
- memory watchpoints;
- comparisons between gameplay states;
- comparisons between regional releases.

Experimental observations should be preserved even when later research
changes their interpretation.

### `notes/`

General reverse-engineering methodology and technical notes that are
not specific to one function or experiment.

Examples include:

- PSXRecomp runtime behavior;
- debugger workflows;
- PlayStation executable structure;
- overlay behavior;
- file formats;
- hardware interfaces.

## Naming policy

Unknown functions should retain address-based names until sufficient
evidence exists for a semantic name.

Prefer:

    func_8004CFC0

or a clearly provisional research label:

    fmv_related_8004CFC0

over prematurely assigning a specific name such as:

    PlayFMV

A semantic name should describe demonstrated behavior rather than merely
a plausible interpretation.

Provisional names should be clearly identified as provisional.

## Confidence

When useful, findings may use informal confidence levels:

- **High** — supported by repeated or direct evidence.
- **Medium** — supported by multiple observations but still ambiguous.
- **Low** — plausible working hypothesis requiring additional evidence.

Confidence in an association does not imply confidence in an exact
semantic interpretation.

For example, a function may have high-confidence association with FMV
playback while its precise role remains unknown.

## Runtime analysis

PSXRecomp is currently used as one dynamic-analysis environment for the
original game.

Generated functions retain names based on their original PlayStation
addresses, making it possible to correlate:

    original PS1 address
        ↓
    generated recompilation function
        ↓
    native runtime execution
        ↓
    observed game behavior

GDB can therefore be used to test whether particular original game
functions execute during controlled gameplay states.

See:

    notes/psxrecomp-runtime.md

for the current runtime-analysis workflow.

## Current investigations

The first documented runtime investigation concerns:

    0x8004CFC0

This function has a strong observed association with prerecorded story
FMV playback, although its exact role remains unknown.

See:

    functions/8004CFC0.md
    experiments/fmv-runtime-tracing.md

## Regional research

SCES-01753 is currently the primary reference release.

Other releases, including SCUS-94448, may be analyzed to identify:

- equivalent functions;
- code and data differences;
- regional content differences;
- timing differences;
- release-specific behavior.

Do not assume that addresses, layouts, or implementation details are
identical between releases.

## Preservation of evidence

Research documentation should favor reproducibility over certainty.

When a hypothesis is disproven, preserve the original observation and
record the revised conclusion rather than rewriting the history of the
investigation.

The objective is not to make the notes look consistently correct.

The objective is to make them useful.
