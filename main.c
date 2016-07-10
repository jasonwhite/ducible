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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "pefile.h"

/**
 * Command line options.
 */
struct CommandOptions
{
    const char* image;
    const char* pdb;
};

/**
 * Prints program usage. Returns the program's exit code.
 */
int usage();

/**
 * Reads an entire file into memory. Returns NULL upon failure.
 *
 * The returned pointer must freed with free().
 */
uint8_t* read_file(const char* path, size_t* len);

/**
 * Patches the PE image.
 */
int patchImage(uint8_t* image, size_t len);

int main(int argc, char** argv)
{
    int ret;

    struct CommandOptions opts;

    size_t imageSize;
    uint8_t* image;

    size_t pdbSize;
    uint8_t* pdb;

    if (argc <= 1)
        return usage();

    if (argc > 1)
        opts.image = argv[1];

    if (argc > 2)
        opts.pdb = argv[2];
    else
        opts.pdb = NULL;

    // Read the image into memory
    // FIXME: It would be best to use a memory map instead of wastefully reading
    // the entire file into memory (which could be quite large, especially if it
    // has embedded resources).
    image = read_file(opts.image, &imageSize);
    if (!image) {
        perror("Failed to open image");
        return 1;
    }

    // Read the PDB into memory
    if (opts.pdb) {
        pdb = read_file(opts.pdb, &pdbSize);
        if (!pdb) {
            perror("Failed to open PDB");
            return 1;
        }
    }

    if ((ret = patchImage(image, imageSize)) != 0)
        return ret;

    if (pdb)   free(pdb);
    if (image) free(image);

    return 0;
}

int usage()
{
    puts("Usage: peclean IMAGE [PDB]");
    return 1;
}

uint8_t* read_file(const char* path, size_t* len) {

    FILE* f;

    f = fopen(path, "rb");
    if (!f)
        return NULL;

    // Find the length of the file
    fseek(f, 0, SEEK_END);
    *len = (size_t)ftell(f);
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    uint8_t* buf = (uint8_t*)malloc(*len);

    if (!buf || fread(buf, 1, *len, f) != *len) {
        fclose(f);
        return NULL;
    }

    fclose(f);
    return buf;
}

int patchImage(uint8_t* image, size_t len)
{
    IMAGE_DOS_HEADER* dosHeader;

    dosHeader = (IMAGE_DOS_HEADER*)image;

    if (len < sizeof(IMAGE_DOS_HEADER))
        goto error;

    if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
        // TODO
    }
    else
    {
        goto error;
    }

    return 0;

error:
    fputs("Image is not a valid PE file", stderr);
    return 1;
}
