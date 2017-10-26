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
#pragma once

#include <stdint.h>
#include <vector>

#include "ducible/patch.h"

/**
 * Keeps track of a list of patches to apply.
 */
class Patches {
   private:
    uint8_t* _buf;

   public:
    // List of patches
    std::vector<Patch> patches;

    Patches(uint8_t* buf);

    void add(Patch patch);

    /**
     * Convenience function for adding patches.
     */
    template <typename T>
    void add(const T* addr, const T* data, const char* name = NULL) {
        add(Patch((const uint8_t*)addr - _buf, data, name));
    }

    /**
     * Sort the patches. The patches will be ordered according to the offset in
     * the file. This is useful once all the patches have been added, but not
     * applied so that we can take the checksum of the file in the areas between
     * the patches.
     */
    void sort();

    /**
     * Applies the patches.
     */
    void apply(bool dryRun = false);
};
