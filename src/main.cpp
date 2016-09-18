/*
 * Copyright (c) 2016 Jason White
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <system_error>
#include <stdlib.h>
#include <vector>
#include <string>

#include "patch_image.h"
#include "pe_file.h"
#include "msf.h"
#include "pdb.h"
#include "version.h"

/**
 * Thrown when there is an error parsing the command line options.
 */
class InvalidCommandLine
{
private:
    std::string _why;

public:

    InvalidCommandLine(const std::string& why) : _why(why) {}

    const std::string& why() const {
        return _why;
    }
};

/**
 * Thrown when an unknown option is passed to the command line.
 */
template<typename CharT = char>
class UnknownOption {
private:
    const CharT* _name;

public:
    UnknownOption(const CharT* name) : _name(name) {}

    const CharT* name() const {
        return _name;
    }
};

/**
 * Thrown when help is requested from the command line.
 */
class CommandLineHelp {
};

/**
 * Thrown when version information is requested from the command line.
 */
class CommandLineVersion {
};

/**
 * String literals for the appropriate character type.
 *
 * FIXME: I'm sure there is a better way to do this...
 */
template<typename CharT>
struct OptionNames {};

template<>
struct OptionNames<char> {
    const char* helpLong    = "--help";
    const char* helpShort   = "-h";
    const char* versionLong = "--version";
    const char* dryrunLong  = "--dryrun";
    const char* dryrunShort = "-n";
    const char* dashDash    = "--";
};

template<>
struct OptionNames<wchar_t> {
    const wchar_t* helpLong    = L"--help";
    const wchar_t* helpShort   = L"-h";
    const wchar_t* versionLong = L"--version";
    const wchar_t* dryrunLong  = L"--dryrun";
    const wchar_t* dryrunShort = L"-n";
    const wchar_t* dashDash    = L"--";
};

/**
 * Command line options.
 */
template<typename CharT = char>
class CommandOptions
{
private:

    static const OptionNames<CharT> opt;

public:

    const CharT* image;
    const CharT* pdb;
    bool dryrun;

    CommandOptions() : image(NULL), pdb(NULL), dryrun(false) {}

    /**
     * Parses the command line arguments.
     */
    void parse(int argc, CharT** argv) {

        typedef std::basic_string<CharT> string;

        // Look for the help option
        for (int i = 1; i < argc; ++i) {
            const string arg = argv[i];
            if (arg == opt.helpLong || arg == opt.helpShort) {
                throw CommandLineHelp();
            } else if (arg == opt.dashDash) {
                break;
            }
        }

        // Look for the version option
        for (int i = 1; i < argc; ++i) {
            const string arg = argv[i];
            if (arg == opt.versionLong) {
                throw CommandLineVersion();
            } else if (arg == opt.dashDash) {
                break;
            }
        }

        // Set to true if only positional arguments can occur.
        bool onlyPositional = false;

        // Positional arguments
        std::vector<const CharT*> positional;

        for (int i = 1; i < argc; ++i) {
            const string arg = argv[i];

            if (onlyPositional) {
                positional.push_back(argv[i]);
            }
            else if (arg == opt.dashDash) {
                onlyPositional = true;
            }
            else if (arg == opt.dryrunLong || arg == opt.dryrunShort) {
                dryrun = true;
            }
            else if (arg.length() > 0 && arg.front() == '-') {
                throw UnknownOption<CharT>(argv[i]);
            }
            else {
                positional.push_back(argv[i]);
            }
        }

        switch (positional.size()) {
            case 2:
                pdb = positional[1];
                // Fall through
            case 1:
                image = positional[0];
                break;
            case 0:
                throw InvalidCommandLine("Missing positional argument");
                break;
            default:
                throw InvalidCommandLine("Too many positional arguments given");
                break;
        }
    }
};

template<typename CharT>
const OptionNames<CharT> CommandOptions<CharT>::opt = OptionNames<CharT>();

const char* usage =
    "Usage: ducible image [pdb] [--help] [--dryrun]";

const char* help =
R"(
This is a simple tool to make builds of Portable Executables (PEs) reproducible.

Timestamps and other non-deterministic data are embedded in DLLs, EXEs, and
PDBs. If a DLL or EXE is compiled and linked twice in a row, without changing
any of the source, the files will not be bit-for-bit identical. This tool fixes
that.

Files are modified in-place.

Positional arguments:
  image         The PE or PE+ file to patch. This can be an .exe or .dll file.
  pdb           The PDB file associated with the image. Optional.

Optional arguments:
  --help, -h    Prints this help.
  --dryrun, -n  No files are modified, only what would have been patched are
                printed.
)";

template<typename CharT = char>
int ducible(int argc, CharT** argv)
{
    CommandOptions<CharT> opts;

    try {
        opts.parse(argc, argv);
    }
    catch (const InvalidCommandLine& error) {
        std::cout << "Error parsing arguments: " << error.why() << std::endl;
        std::cout << usage << std::endl;
        return 1;
    }
    catch (const UnknownOption<CharT>& error) {
        std::cout << "Error parsing arguments: Unknown option '" << error.name()
            << "'" << std::endl;
        std::cout << usage << std::endl;
        return 1;
    }
    catch (const CommandLineHelp&) {
        std::cout << usage << std::endl;
        std::cout << help;
        return 0;
    }
    catch (const CommandLineVersion&) {
        std::cout << "ducible version " << DUCIBLE_PRETTY_VERSION <<
            std::endl;
        return 0;
    }

    try {
        patchImage(opts.image, opts.pdb, opts.dryrun);
    }
    catch (const InvalidImage& error) {
        std::cerr << "Error: Invalid image (" << error.why() << ")\n";
        return 1;
    }
    catch (const InvalidMsf& error) {
        std::cerr << "Error: Invalid PDB MSF format (" << error.why() << ")\n";
        return 1;
    }
    catch (const InvalidPdb& error) {
        std::cerr << "Error: Invalid PDB format (" << error.why() << ")\n";
        return 1;
    }
    catch (const std::system_error& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}

#if defined(_WIN32) && defined(UNICODE)

int wmain(int argc, wchar_t** argv) {
    return ducible<wchar_t>(argc, argv);
}

#else

int main(int argc, char** argv) {
    return ducible<char>(argc, argv);
}

#endif
