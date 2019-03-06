# Loki RPC Documentation Generator
A self contained tool that only depends on the C++ standard library to auto-generate documentation for RPC commands used in the Loki daemon.

## Building
Use the supplied build scripts `build_msvc.bat` (for Visual Studio) or `build_gcc.sh` (for GCC), or simply invoke the compiler on the single CPP file.
The build script compiles the executable to the Bin folder at the root of this repository.

## Usage
The executable outputs the contents to standard output. Redirect this to your file of choice, i.e.
`loki_rpc_doc_generator.exe <cpp header file> > generated_document.html`
