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

#include "pepatch.h"

/**
 * Command line options.
 */
struct CommandOptions
{
    const char* image;
    const char* pdb;

    CommandOptions() : image(NULL), pdb(NULL) {}
};

/**
 * Prints program usage. Returns the program's exit code.
 */
int usage();

int main(int argc, char** argv)
{
    CommandOptions opts;

    if (argc <= 1)
        return usage();

    if (argc > 1)
        opts.image = argv[1];

    if (argc > 2)
        opts.pdb = argv[2];
    else
        opts.pdb = NULL;

    try {
        // TODO: Don't hardcode dry run
        patchImage(opts.image, opts.pdb, true);
    }
    catch (const InvalidImage& error) {
        std::cerr << "Error: Invalid image (" << error.why() << ")\n";
        return 1;
    }
    catch (const std::system_error& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}

int usage()
{
    puts("Usage: peclean IMAGE [PDB]");
    return 1;
}
