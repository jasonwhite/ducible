# PE Cleaner

*This is a work in progress.*

This is a simple tool to make builds of Portable Executables (PEs) reproducible.

Timestamps and other non-deterministic data are embedded in DLLs, EXEs, and
PDBs. If a DLL is compiled and linked twice without changing any source, the
DLLs will not be bit-by-bit identical. This tool fixes that.

Builds that are reproducible are very useful for a number of reasons. The site
https://reproducible-builds.org/ does an excellent job of enumerating those
reasons.

## Using It

Usage is as follows:

    $ peclean IMAGE [PDB]

The EXE/DLL is specified as the first parameter and the PDB is optionally
specified as the second. The PDB must be modified because changing the image
invalidates the signature for the PDB.

As a post-build step, simply run:

    $ peclean MyModule.dll MyModule.pdb

The files are overwritten in-place.

## Building It

This is a very simple C++ program. There are no dependencies and it should be
buildable and runnable on any platform (even non-Windows!).

*Build instructions are coming soon to a README near you.*

## Related Work

I am only aware of the [zap_timestamp][] tool in [Syzygy][]. Unfortunately, it
has a couple of problems:

 1. It does not work with 64-bit PE files (i.e., the PE+ format).
 2. It is a pain to build. It is part of a larger suite of tools that operate on
    PE files. That suite then requires Google's depot_tools. The end result is
    that you're required to download hundreds of megabytes of tooling around
    something that should be very simple.

[zap_timestamp]: https://github.com/google/syzygy/tree/master/syzygy/zap_timestamp
[Syzygy]: https://github.com/google/syzygy

## License

As always, this tool uses the very liberal [MIT License](/LICENSE). Use it for
whatever nefarious purposes you like.
