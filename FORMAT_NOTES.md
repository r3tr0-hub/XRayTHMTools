# THM inspection scope

Each chunk is `u32 id, u32 payload_size, payload` (little endian).
Bit 31 is the compressed marker. 0x0810 holds a u16 version; 0x0813 holds
a u32 resource kind. Tested object=0, texture=1, both version=0x0012.

Object and texture documents are different. 0x0816 is context-dependent:
object statistics must not be displayed as texture material.

Texture v0x0012 field interpretation implemented for the classic layout:

| Chunk | Interpretation |
|---|---|
| 0x0812 | 8 u32: format, flags, border, fade color, fade amount, mip filter, width, height |
| 0x0814 | texture type ID, u32 |
| 0x0815 | detail texture stringZ, detail scale float32 |
| 0x0816 | material ID u32, weight float32 |
| 0x0817 | bump height float32, bump mode u32, bump texture stringZ |
| 0x0818 | external normal map stringZ |
| 0x0819 | fade delay u8 |

Field enum/flag tables were checked against VaIeroK/THM-Editor commit
`b7645f02a7db7f0ded2dac3500b20f895e5e1acb`. See ACKNOWLEDGMENTS.md.
Modified SDK layouts may still differ. The writer refuses unsupported versions
and extended/compressed known setting chunks, preserving unrelated chunks.

The reference editor's optional fifth byte in 0x0813 is the SoC compatibility
marker. Value 1 swaps the meanings of the first u32 in 0x0812 and the u32 in
0x0814 (type / format). The UI always exposes semantic format and type.
Switching the marker rewrites the relevant chunks together. An unchanged
existing marker byte, including explicit 0, remains byte-identical.

Known flags: 0 mipmaps, 1 binary alpha, 4 alpha border, 5 color border,
6 fade to color, 7 fade to alpha, 8 dither color, 9 dither each mip,
23 diffuse detail, 24 implicit lighted, 25 has alpha, 26 bump detail.
The full uint32 mask is preserved, including all unknown bits.

Observed in supplied briks_big_01.thm (not redistributed): 185 bytes, texture
kind 1, version 18, parameters width/height 1024, flags 0x04800001 (bits 0,23,26),
format ID 0, mip filter ID 14, texture type ID 0, detail scale 8, material ID 0,
weight 0.5, bump height approximately 0.05, bump mode ID 3, no embedded image.
These are stored THM settings; the paired DDS was not supplied or inspected.

The parser rejects duplicate chunks and malformed boundaries. Unknown chunks
remain in memory and are listed. Unknown versions display raw data only.
Malformed known-field layouts produce warnings, preserving the overall report.
The editor writer preserves chunk order, raw payloads and compression markers
for unmodified chunks. Missing optional chunks are created only if their values
change from defaults. New files include all classic metadata chunks, but no image.
Strings use CP1251 in the Windows UI; serialization retains bytes as supplied.
Float32 is formatted with max_digits10 to avoid rounding on a no-op form save.

DDS import reads only the header (including DX10 where present); it is not a
pixel decoder or a DDS-to-THM-image converter. BC1 alpha cannot be inferred
reliably from a header. Generation refuses unrepresentable formats.

Atomic save uses a same-directory flushed temp file and Windows ReplaceFile
with a backup of the replaced file. It compares against the opening snapshot
and refuses an existing active writer. This is a local editor conflict check,
not a multi-user transaction system; avoid concurrent filesystem renames or
network/cloud sync changes during a save. Multi-file generation commits each
file separately and reports partial completion on failure.

WLX API signatures and lifecycle checked against official docs:
- https://ghisler.github.io/WLX-SDK/listload.htm
- https://ghisler.github.io/WLX-SDK/listloadnext.htm
- https://ghisler.github.io/WLX-SDK/listgetdetectstring.htm

The implementation declares the small exported ABI itself; no third-party
SDK source code is bundled.
