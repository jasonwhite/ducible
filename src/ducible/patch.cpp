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

#include "ducible/patch.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <tuple>

Patch::Patch(size_t offset, size_t length, const uint8_t* data,
             const char* name)
    : offset(offset), length(length), data(data), name(name) {}

void Patch::apply(uint8_t* buf, bool dryRun) {
    // Only apply the patch if necessary. This makes it easier to see what
    // actually changed in the output.
    if (memcmp(buf + offset, data, length) == 0) return;

    std::cout << *this << std::endl;

    if (!dryRun) memcpy(buf + offset, data, length);
}

std::ostream& operator<<(std::ostream& os, const Patch& patch) {
    os << "Patching '" << patch.name << "' at offset 0x" << std::hex
       << patch.offset << std::dec << " (" << patch.length << " bytes)";
    return os;
}

bool operator<(const Patch& a, const Patch& b) {
    return std::tie(a.offset, a.length) < std::tie(b.offset, b.length);
}
