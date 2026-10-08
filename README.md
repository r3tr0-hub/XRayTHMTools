# X-Ray THM Tools

Standalone Total Commander Lister plugin and texture THM editor for S.T.A.L.K.E.R. / X-Ray SDK. Version **0.4.0-beta**.

- F3 / Ctrl+Q: embedded THM previews and texture metadata.
- `.object`: preview from the adjacent `.thm`; no geometry rendering.
- Native texture settings editor; Edit (F4), reload, configurable external F4 editor.
- Classic v0x0012 texture fields and flags, DDS header import and THM generation.
- Backups on overwrite; preservation of unknown chunks and embedded thumbnails.
- Windows x86/x64 builds. Source implementation licensed under MIT.

## Install

Download `XRayTHMTools-v0.4.0-beta.zip` from Releases and open it in Total Commander. This source archive is not the plugin installer. To update, fully close TC and extract the binary ZIP over the installed plugin folder.

See [Russian guide](README_RU.md), [Lister guide](LISTER_README_RU.md), and [editor/F4 guide](EDITOR_README_RU.md).

## Build

Visual Studio 2022 Build Tools: C++ x86/x64 MSVC v143, Windows SDK, CMake.
Run `build.bat` from Developer Command Prompt. Output: `dist/XRayTHMTools-v0.4.0-beta.zip` and `.zip.sha256`.
GitHub Actions builds and tests both architectures and uploads the same package.

## Known limitations

Some supplied texture THM previews show partial colored noise with the current decoder; cause remains unresolved. Texture THM files may legitimately contain thumbnails. The editor preserves their image bytes, but does not repair or regenerate previews. Modified SDK schemas may differ from the supported classic format.

This product does not install a Windows Explorer thumbnail handler. The separate XRayTHMThumbnail project has its own repository and releases.

See [format notes](FORMAT_NOTES.md), [validation](VALIDATION.md), and [acknowledgments](ACKNOWLEDGMENTS.md). No game resources or third-party editor source code are bundled.
