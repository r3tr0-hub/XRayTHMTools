# Validation

The user reported the preceding combined 0.4.0-beta build working on Windows.
This standalone packaging revision preserves editor/decoder/model sources and the latest Esc-fixed Lister source. It removes the Explorer targets and changes packaging/docs only.

Portable decoder/document/texture test executables were built directly with g++ (C++17) for this revision. CMake/CTest are not installed in the current environment, so CMake configuration was reviewed but not executed. Windows MSVC build, native UI and PowerShell packaging are not run in this environment. The included Windows CI builds x86/x64 and runs the Windows safe-file tests before packaging.

Known unresolved issue: partial noise in embedded previews from some supplied texture THM files. Passing synthetic tests does not establish correct decoding of those images. Samples are not distributed.

All three portable test executables passed for the standalone revision. Source/package references and retention of the Esc fix were checked.
