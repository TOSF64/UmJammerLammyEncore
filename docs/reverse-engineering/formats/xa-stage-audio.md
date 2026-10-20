# Stage XA Audio

## Status

Initial characterization based on the PAL European release of
*Um Jammer Lammy* (`SCES-01753`).

The Stage 3 resource `S3/ST3.XA` has been examined in detail.
Some observations below may apply to other stage `.XA` resources,
but this has not yet been verified.

## Summary

`ST3.XA` contains multiple interleaved PlayStation XA ADPCM audio
streams used by Stage 3.

The file contains separate audio streams for the Lammy and PaRappa
versions of the stage, as well as complete Good, Bad, and Awful
Master/instrumental mixes.

Eight streams have been identified by direct listening.

The ISO 9660-visible representation of the file is not sufficient
for FFmpeg to recognize the XA streams. Preserving the original
2352-byte raw CD sectors, including their sector-level metadata,
allows FFmpeg to identify and decode the streams successfully.

## Disc Location

Release:

- Region: Europe / PAL
- Disc ID: `SCES-01753`
- File: `S3/ST3.XA`
- Starting LBA: `115398`
- ISO file size: `22,118,400` bytes

The ISO-visible size is exactly:

22,118,400 / 2048 = 10,800 sectors

At 75 CD sectors per second:
10,800 / 75 = 144 seconds
                 = 2 minutes 24 seconds

The decoded streams examined so far are also 2:24 long.
## Raw-Sector Requirement
Encore's current generic ISO 9660 extraction path reads the
2048-byte user-data portion of each sector.

This produces:

ST3.XA
22,118,400 bytes
10,800 * 2048 bytes

FFmpeg does not recognize this extracted representation as a
supported media stream.

## Example:
ST3.XA: Invalid data found when processing input

Forcing FFmpeg's psxstr demuxer on this representation also fails
to determine usable codec parameters.

The corresponding sectors were therefore extracted directly from
the original BIN while preserving the complete 2352-byte sectors:
10,800 * 2352 = 25,401,600 bytes

The raw-sector representation is immediately recognized by FFmpeg:
Input #0, psxstr

FFmpeg reports eight audio streams, each as:
Audio: adpcm_xa, 37800 Hz, mono

This demonstrates that the sector-level information discarded by
the generic 2048-byte ISO extraction path is necessary for standard
PlayStation XA tooling to interpret this resource correctly.
Importer implication
Generic ISO 9660 extraction is therefore not sufficient by itself
for preservation or external decoding of at least this XA resource.

Encore should retain access to the original raw-sector
representation when importing PlayStation XA resources.

The appropriate long-term representation for imported XA data is
not yet decided.

## ST3.XA Stream Layout
All eight streams were decoded independently and identified by
direct listening.

"Master" below refers to the stage's teacher/opponent performance
in the game's rhythm mechanics. It does not refer to audio mastering
or lossless/master-quality audio.

| Stream | Character | Rank  | Content |
|-------:|-----------|-------|---------|
| 0 | Lammy   | Good  | Instrumental only |
| 1 | PaRappa | Good  | Instrumental only |
| 2 | Lammy   | Good  | Master performance + instrumental |
| 3 | PaRappa | Good  | Master performance + instrumental |
| 4 | Lammy   | Bad   | Master performance + instrumental |
| 5 | PaRappa | Bad   | Master performance + instrumental |
| 6 | Lammy   | Awful | Master performance + instrumental |
| 7 | PaRappa | Awful | Master performance + instrumental |



For this resource, even-numbered streams contain Lammy material and
odd-numbered streams contain the corresponding PaRappa material.

This even/odd relationship is currently established only for
ST3.XA.
## Complete Bad and Awful Arrangements
The Bad and Awful streams contain complete arrangements beginning
at the start of the song.

This is notable because normal gameplay begins at Good rank.
Consequently, the beginning of the Bad and Awful arrangements cannot
normally be heard by simply starting Stage 3 at those ranks.

The disc nevertheless contains audio for those portions of the
arrangements.

This suggests that the rank-specific audio may exist as parallel
full-length arrangements sharing a common song timeline.

One possible runtime implementation would be to transition between
synchronized XA streams as the player's rank changes.

This is currently an inference from the asset layout and must be
verified against runtime behavior.

## Relationship to .SNG
Stage 3 also contains resources including:
COMOD3.SNG
COMOD13.SNG
CMP3SNG.INT
CMP13SNG.INT

COMOD3.SNG and COMOD13.SNG contain human-readable dialogue and
song-related text among structured binary data.

Observed text includes material associated with both the Lammy and
PaRappa versions of Stage 3.

Because the actual prerecorded Stage 3 audio has now been confirmed
inside ST3.XA, the .SNG resources do not appear to contain the
primary prerecorded stage audio.

Their exact purpose remains unknown.

Possible responsibilities worth investigating include:

- lyric/subtitle presentation
- song scripting
- timing information
- gameplay metadata
- references or indices into other stage resources
These are hypotheses only.



# Unknowns
The following remain unresolved:
- Whether other ST#.XA files use the same eight-stream organization.
- How the game selects XA streams at runtime.
- How stream synchronization is maintained during rank transitions.
- Whether rank changes actually switch between these parallel streams.
- Where Cool-rank audio is stored or generated.
- Where Lammy and PaRappa player-performance audio is stored.
- Whether player-performance audio is streamed, sequenced, synthesized,
  or constructed through another mechanism.
- The exact relationship between .XA, .SNG, and associated .INT
  resources.
- Whether the COMOD3 / COMOD13 naming distinction corresponds to a
  meaningful runtime concept.
- Whether all XA resources require preservation of raw 2352-byte
  sectors in the same way as ST3.XA.
  
# Reproduction
The following experiment was performed against a legally obtained
PAL SCES-01753 disc image.

ST3.XA begins at LBA 115398 and spans 10,800 sectors.

A raw-sector copy can be produced from the corresponding BIN:

dd \
  if="Um Jammer Lammy (Europe) (En,Fr,De,Es,It).bin" \
  of="/tmp/ST3_raw.xa" \
  bs=2352 \
  skip=115398 \
  count=10800
  
## Expected size:
25,401,600 bytes

FFmpeg can then inspect the result:
ffprobe -hide_banner /tmp/ST3_raw.xa

Expected stream structure:

Stream #0:0: Audio: adpcm_xa, 37800 Hz, mono
Stream #0:1: Audio: adpcm_xa, 37800 Hz, mono
Stream #0:2: Audio: adpcm_xa, 37800 Hz, mono
Stream #0:3: Audio: adpcm_xa, 37800 Hz, mono
Stream #0:4: Audio: adpcm_xa, 37800 Hz, mono
Stream #0:5: Audio: adpcm_xa, 37800 Hz, mono
Stream #0:6: Audio: adpcm_xa, 37800 Hz, mono
Stream #0:7: Audio: adpcm_xa, 37800 Hz, mono

An individual stream can be decoded to PCM WAV:

ffmpeg \
  -f psxstr \
  -i /tmp/ST3_raw.xa \
  -map 0:0 \
  -c:a pcm_s16le \
  /tmp/ST3_stream0.wav
  
For stream 0, FFmpeg produces a 37,800 Hz mono PCM stream with a
duration of exactly 2:24.
Next Experiments
The highest-value next test is to repeat this analysis on another
stage .XA resource.

If another stage exhibits the same stream organization, the current
Stage 3-specific observations can begin to be generalized into a
game-wide stage audio format description.

Runtime tracing should eventually determine how the original game
selects and transitions between XA streams during rank changes.

The .SNG and associated .INT resources should be investigated
separately and correlated with the XA stream layout before assigning
them semantic names.
