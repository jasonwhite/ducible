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

#include "msf.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <system_error>

#include "util/file.h"

#include "msf/file_stream.h"
#include "msf/readonly_stream.h"

namespace {

// A good page size to use when writing out the MSF.
const size_t kPageSize = 4096;

// A blank page. Used to write uninitialized pages to the MSF file.
const uint8_t kBlankPage[kPageSize] = {0};

/**
 * Helper function for finding the size of the given file.
 */
int64_t getFileSize(FILE* f) {
    // Save our spot...
    int64_t pos = ftell(f);
    if (pos == -1) {
        throw std::system_error(errno, std::system_category(),
                                "ftell() failed");
    }

    // Go to the end
    if (fseek(f, 0, SEEK_END) == -1) {
        throw std::system_error(errno, std::system_category(),
                                "fseek() failed");
    }

    // Current position is the size of the file.
    int64_t size = ftell(f);
    if (size == -1) {
        throw std::system_error(errno, std::system_category(),
                                "ftell() failed");
    }

    // Go back to our old spot
    if (fseek(f, (long)pos, SEEK_SET) == -1) {
        throw std::system_error(errno, std::system_category(),
                                "fseek() failed");
    }

    return size;
}

/**
 * Returns true if the given page number should be a free page map page.
 *
 * The free page map is spread out across the file at regular intervals. There
 * are always two FPMs right next to each other in order to allow atomic commits
 * to the PDB. Given a page size of 4096 bytes, one FPM can keep track of 4096*8
 * pages. However, there are two free page maps every 4096 pages. Thus, there
 * are 8x too many pages dedicated to the FPM. This is a bug in the original
 * Microsoft implementation and fixing it at this point would break every PDB
 * out there or add an unreasonable complexity to the file format, so we're
 * stuck with it for the foreseeable future.
 */
bool isFpmPage(size_t page, size_t pageSize = kPageSize) noexcept {
    switch (page & (pageSize - 1)) {
        case 1:
        case 2:
            return true;
        default:
            return false;
    }
}

/**
 * Writes a page to the given file handle.
 */
void writePage(FileRef f, const uint8_t* data, size_t pageSize,
               std::vector<uint32_t>* pagesWritten, uint32_t& pageCount) {
    if (fwrite(data, 1, pageSize, f.get()) != pageSize) {
        throw std::system_error(errno, std::system_category(),
                                "failed writing page");
    }

    if(pagesWritten)
      pagesWritten->push_back(pageCount);
    pageCount++;
}

/**
 * Writes a stream to the given file handle.
 *
 * The non-FPM pages that are written are appended to the given vector.  Page
 * count is incremented for both normal and FPM pages.
 */
void writeStream(FileRef f, MsfStreamRef stream,
                 std::vector<uint32_t>& pagesWritten, uint32_t& pageCount) {
    if (!stream || stream->length() == 0) return;

    uint8_t buf[kPageSize];

    stream->setPos(0);

    while (size_t bytesRead = stream->read(kPageSize, buf)) {
        assert(bytesRead <= kPageSize);

        size_t leftOver = kPageSize - bytesRead;

        // Pad the rest of the buffer with zeros
        memset(buf + bytesRead, 0, leftOver);

        if (isFpmPage(pageCount)) {
            writePage(f, kBlankPage, sizeof(kBlankPage), nullptr,
                      pageCount);
            writePage(f, kBlankPage, sizeof(kBlankPage), nullptr,
                      pageCount);
        }

        writePage(f, buf, sizeof(buf), &pagesWritten, pageCount);
    }
}

/**
 * The Free Page Map (FPM). This is used to keep track of free pages in the MSF.
 *
 * A page is "free" if its index is a 1 in this bit map. Conversely, a page is
 * "used" if its index is a 0 in this bit map.
 */
class FreePageMap {
   private:
    std::vector<uint8_t> _data;

   public:
    /**
     * Initialize the free page map. By default, all pages are initially marked
     * as "used".
     */
    FreePageMap(size_t pageCount, uint8_t initValue = 0x00)
        : _data((pageCount + 7) / 8, initValue) {
        // Mark the left over bits at the end as free
        _data.back() |= ~(0xFF >> (_data.size() * 8 - pageCount));
    }

    /**
     * Mark a page as free.
     */
    void setFree(size_t page) { _data[page / 8] |= 1 << (page % 8); }

    /**
     * Mark a page as used.
     */
    void setUsed(size_t page) { _data[page / 8] &= ~(1 << (page % 8)); }

    /**
     * Writes the FPM to the MSF.
     */
    void write(FILE* f, size_t pageSize = kPageSize) const;
};

void FreePageMap::write(FILE* f, size_t pageSize) const {
    // The FPM is spread out across the MSF at regular intervals. There are two
    // FPM pages every 4096 pages (or whatever the page size is), starting at
    // page index 1. We do not write to the second FPM page in each pair.
    // The second page in each pair is used by Microsoft's PDB updater to do
    // atomic commits. That is, after new pages of a stream are written, the
    // updated free page map is written to every second page of each FPM pair.
    // Then, to commit the changes, the FPM page is set to 2 in the MSF header.
    //
    // Note also that there are 8 times as many FPM pages as necessary. Thus, a
    // large portion of them are never used and are just wasted space in the
    // file. This is due to a bug in Microsoft's PDB implementation and is
    // unlikely to be fixed in the future.

    // Start at the first page.
    size_t page = 1;

    size_t chunks = _data.size() / pageSize;

    const uint8_t* data = _data.data();

    for (size_t i = 0; i < chunks; ++i) {
        // Seek to the FPM page
        if (fseek(f, (long)(page * pageSize), SEEK_SET) != 0) {
            throw std::system_error(errno, std::system_category(),
                                    "Failed to seek to MSF page");
        }

        // Write a page of the FPM
        if (fwrite(data, 1, pageSize, f) != pageSize) {
            ;
            throw std::system_error(errno, std::system_category(),
                                    "Failed to write FPM page");
        }

        page += pageSize;
        data += pageSize;
    }

    // Write the remainder of the FPM and fill with 0xFF.
    if (const size_t leftOver = _data.size() % pageSize) {
        // Seek to the FPM page
        if (fseek(f, (long)(page * pageSize), SEEK_SET) != 0) {
            throw std::system_error(errno, std::system_category(),
                                    "Failed to seek to final MSF page");
        }

        // Write a partial page of the FPM
        if (fwrite(data, 1, leftOver, f) != leftOver) {
            ;
            throw std::system_error(errno, std::system_category(),
                                    "Failed to write final FPM page");
        }

        // Fill the rest with 1s to indicate free pages.
        std::vector<uint8_t> ones(pageSize - leftOver, 0xFF);
        if (fwrite(ones.data(), 1, ones.size(), f) != ones.size()) {
            ;
            throw std::system_error(errno, std::system_category(),
                                    "Failed to write final FPM page");
        }
    }
}

}  // namespace

MsfFile::MsfFile(FileRef f) {
    MSF_HEADER header;

    // Read the header
    if (fread(&header, sizeof(header), 1, f.get()) != 1)
        throw InvalidMsf("Missing MSF header");

    // Check that this is indeed an MSF header
    if (memcmp(header.magic, kMsfHeaderMagic, sizeof(kMsfHeaderMagic)) != 0)
        throw InvalidMsf("Invalid MSF header");

    // Check that the file size makes sense
    if (header.pageSize * header.pageCount != getFileSize(f.get()))
        throw InvalidMsf("Invalid MSF file length");

    // The number of pages required to store the pages of the stream table
    // stream.
    size_t stPagesPagesCount =
        ::pageCount(header.pageSize, header.streamTableInfo.size);

    // Read the stream table page directory
    std::unique_ptr<uint32_t> streamTablePagesPages(
        new uint32_t[stPagesPagesCount]);

    if (fread(streamTablePagesPages.get(), sizeof(uint32_t), stPagesPagesCount,
              f.get()) != stPagesPagesCount) {
        throw InvalidMsf("Missing root MSF stream table page list");
    }

    MsfFileStream streamTablePagesStream(f, header.pageSize,
                                         stPagesPagesCount * sizeof(uint32_t),
                                         streamTablePagesPages.get());

    // Read the list of stream table pages.
    std::vector<uint32_t> streamTablePages(stPagesPagesCount);
    if (streamTablePagesStream.read(&streamTablePages[0]) !=
        stPagesPagesCount * sizeof(uint32_t)) {
        throw InvalidMsf("failed to read stream table page list");
    }

    // Finally, read the stream table itself
    MsfFileStream streamTableStream(
        f, header.pageSize, header.streamTableInfo.size, &streamTablePages[0]);
    std::vector<uint32_t> streamTable(header.streamTableInfo.size /
                                      sizeof(uint32_t));
    if (streamTableStream.read(&streamTable[0]) != header.streamTableInfo.size)
        throw InvalidMsf("failed to read stream table");

    // The first element in the stream table is the total number of streams.
    const uint32_t& streamCount = streamTable[0];

    // The sizes of each stream then follow.
    const uint32_t* streamSizes = &streamTable[1];

    // After all the sizes, there are the lists of pages for each stream. We
    // calculate the number of pages required for the stream using the stream
    // size.
    const uint32_t* streamPages = streamSizes + streamCount;

    uint32_t pagesIndex = 0;
    for (uint32_t i = 0; i < streamCount; ++i) {
        // If we were given a bogus stream count, we could potentially overflow
        // the stream table vector. Detect that here.
        if (pagesIndex >= streamTable.size())
            throw InvalidMsf("invalid stream count in stream table");

        uint32_t size = streamSizes[i];

        // Microsoft's PDB implementation sometimes sets the size of a stream to
        // -1. We can't ignore this stream as it will invalidate the stream
        // IDs everywhere. Instead, just set it to a length of 0.
        if (size == (uint32_t)-1) size = 0;

        addStream(new MsfFileStream(f, header.pageSize, size,
                                    streamPages + pagesIndex));

        pagesIndex += ::pageCount(header.pageSize, size);
    }
}

MsfFile::~MsfFile() {}

size_t MsfFile::addStream(MsfStream* stream) {
    _streams.push_back(MsfStreamRef(stream));
    return _streams.size() - 1;
}

MsfStreamRef MsfFile::getStream(size_t index) {
    if (index < _streams.size()) {
        return _streams[index];
    }

    return nullptr;
}

void MsfFile::replaceStream(size_t index, MsfStreamRef stream) {
    _streams[index] = stream;
}

size_t MsfFile::streamCount() const { return _streams.size(); }

void MsfFile::write(FileRef f) const {
    uint32_t pageCount = 0;

    // Write out 4 blank pages: one for the header, two for the FPM, and one
    // superfluous blank page. We'll come back at the end and write in the
    // header and free page map. We can't do it now, because we don't have that
    // information yet.
    for (; pageCount < 4; ++pageCount) {
        if (fwrite(kBlankPage, 1, kPageSize, f.get()) != kPageSize) {
            throw std::system_error(errno, std::system_category(),
                                    "failed writing MSF preamble");
        }
    }

    // Initialize the stream table.
    std::vector<uint32_t> streamTable;
    streamTable.push_back((uint32_t)streamCount());

    for (auto&& stream : _streams) {
        if (stream)
            streamTable.push_back((uint32_t)stream->length());
        else
            streamTable.push_back(0);
    }

    // Write out each stream and add the stream's page numbers to the stream
    // table. Note that stream 0 is special, we need to keep track of which
    // pages it was written to so we can mark them as free later.
    size_t streamZeroStart = streamTable.size();
    if (_streams.size() > 0) {
        writeStream(f, _streams[0], streamTable, pageCount);
    }
    size_t streamZeroEnd = streamTable.size();

    for (size_t i = 1; i < _streams.size(); ++i) {
        writeStream(f, _streams[i], streamTable, pageCount);
    }

    // Write the stream table stream at the end of the file, keeping track of
    // which pages were written.
    std::vector<uint32_t> streamTablePages;
    MsfStreamRef streamTableStream(new MsfReadOnlyStream(
        streamTable.size() * sizeof(streamTable[0]), streamTable.data()));

    writeStream(f, streamTableStream, streamTablePages, pageCount);

    // Write the stream table pages, keeping track of which pages were written.
    // These pages in turn will be written after the MSF header.
    std::vector<uint32_t> streamTablePgPg;
    MsfStreamRef streamTableStreamPages(new MsfReadOnlyStream(
        streamTablePages.size() * sizeof(uint32_t), streamTablePages.data()));

    writeStream(f, streamTableStreamPages, streamTablePgPg, pageCount);

    // Write the header
    MSF_HEADER header = {};
    memcpy(header.magic, kMsfHeaderMagic, sizeof(kMsfHeaderMagic));
    header.pageSize    = kPageSize;
    header.freePageMap = 1;
    header.pageCount   = pageCount;
    header.streamTableInfo.size =
        (uint32_t)streamTable.size() * sizeof(streamTable[0]);
    header.streamTableInfo.index = 0;

    if (fseek(f.get(), 0, SEEK_SET) != 0) {
        throw std::system_error(errno, std::system_category(),
                                "fseek() failed");
    }

    if (fwrite(&header, sizeof(header), 1, f.get()) != 1) {
        throw std::system_error(errno, std::system_category(),
                                "failed writing MSF header");
    }

    // Make sure there aren't too many root stream table pages. This could only
    // happen for ridiculously large PDBs or if there is a bug in this program.
    const size_t streamTablePgPgLength =
        streamTablePgPg.size() * sizeof(streamTablePgPg[0]);

    if (streamTablePgPgLength > kPageSize - sizeof(header)) {
        throw InvalidMsf(
            "root stream table pages are too large to fit in one page");
    }

    // Write the root pages for the stream table.
    if (fwrite(streamTablePgPg.data(), sizeof(streamTablePgPg[0]),
               streamTablePgPg.size(), f.get()) != streamTablePgPg.size()) {
        throw std::system_error(errno, std::system_category(),
                                "failed writing MSF header");
    }

    // Construct the free page map.
    FreePageMap fpm(pageCount);
    fpm.setFree(3);  // The omnipresent superfluous page

    // Mark stream 0 pages as free
    for (size_t i = streamZeroStart; i < streamZeroEnd; ++i) {
        fpm.setFree(streamTable[i]);
    }

    // Write the free page map.
    fpm.write(f.get());
}
