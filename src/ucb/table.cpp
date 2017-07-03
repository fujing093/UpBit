#include "fastbit/bitvector.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include "fastbit/bitvector.h"
#include <cstdlib>
#include <cstdio>

#include "ucb/table.h"
#include "nicolas/util.h"

using namespace ucb;

const ibis::bitvector::word_t UCB_PAD_BITS = (10000000/ 31) * 31;

Table::Table(uint32_t cardinality, uint32_t n_rows) : BaseTable(cardinality, n_rows) {
    total_rows = number_of_rows + UCB_PAD_BITS;
    existence_bitmap.adjustSize(n_rows, total_rows);
    existence_bitmap.compress();
    for (uint32_t i = 0; i < cardinality; ++i) {
        bitmaps[i] = new ibis::bitvector();
        bitmaps[i]->read(getBitmapName(i).c_str());
        if (enable_fence_pointer)
            bitmaps[i]->buildIndex();
        bitmaps[i]->adjustSize(number_of_rows, total_rows);
        bitmaps[i]->compress();
    }
    TBC = total_rows;
}

void Table::append(int val) {
    bitmaps[val]->setBit(number_of_rows, 1);
    existence_bitmap.setBit(number_of_rows, 1);
    append_rowids.push_back(number_of_rows);
    number_of_rows += 1;
}

void Table::appendBitmap(int to_val) {
    string name = getBitmapName(to_val);

    uint32_t * buffer = new uint32_t[2];
    FILE * file = fopen(name.c_str(), "rb+");
    fseek(file, 0L, SEEK_END);
    auto size = ftell(file);
    // 0a in Linux
    fseek(file, size - 9, SEEK_SET);
    fread(buffer, 2, 4, file);
    if (TBC - number_of_rows < buffer[1] * 31) {
        fseek(file, size - 5, SEEK_SET);
        ibis::bitvector::word_t fill0 = (number_of_rows - (TBC - buffer[1] * 31)) / 31;
        ibis::bitvector::word_t literal = (1U << (number_of_rows % 31)) & (1U << ibis::bitvector::MAXBITS);
        ibis::bitvector::word_t fill1 = (TBC - number_of_rows) / 31;
        if (fill0 > 0) {
            ibis::bitvector::word_t content[] = {fill0, literal, fill1};
            fwrite(content, 4, 3, file);
        } else {
            ibis::bitvector::word_t content[] = {literal, fill1};
            fwrite(content, 4, 2, file);
        }
        char end[] = {0x0A};
        fwrite(end, 1, 1, file);
    } else {
        ibis::bitvector::word_t literal[] = {(1U << (number_of_rows % 31)) | buffer[0] | (1U << ibis::bitvector::MAXBITS)};
        fseek(file, size - 9, SEEK_SET);
        fwrite(literal, 4, 1, file);
    }
    fclose(file);
}

void Table::fastAppend(int to_val) {
    bitmaps[to_val]->appendActive();
    auto size = bitmaps[to_val]->m_vec.size();
    uint32_t buffer[] = {bitmaps[to_val]->m_vec[size - 2], bitmaps[to_val]->m_vec[size - 1]};
    buffer[1] &= ibis::bitvector::MAXCNT;

    // std::cout << std::hex << buffer[0] << std::endl;
    // std::cout << std::hex << buffer[1] << std::endl;

    if (TBC - number_of_rows < buffer[1] * 31) {
        ibis::bitvector::word_t fill0 = ((number_of_rows - (TBC - buffer[1] * 31)) / 31) | (1U << ibis::bitvector::MAXBITS);
        ibis::bitvector::word_t literal = (1U << (number_of_rows % 31));
        ibis::bitvector::word_t fill1 = ((TBC - number_of_rows) / 31) | (1U << ibis::bitvector::MAXBITS);
        if (fill0 > ibis::bitvector::MAXCNT) {
            bitmaps[to_val]->m_vec[size - 1] = fill0;
            bitmaps[to_val]->m_vec.push_back(literal);
            bitmaps[to_val]->m_vec.push_back(fill1);
            // std::cout << std::hex << fill0 << std::endl;
            // std::cout << std::hex << literal << std::endl;
            // std::cout << std::hex << fill1 << std::endl;
        } else {
            bitmaps[to_val]->m_vec[size - 1] = literal;
            bitmaps[to_val]->m_vec.push_back(fill1);
        }
    } else {
        ibis::bitvector::word_t literal = (1U << (number_of_rows % 31)) | buffer[0];
        bitmaps[to_val]->m_vec[size - 2] = literal;
    }
}

void Table::update(uint32_t rowid, int to_val) {
    showEB = true;
    existence_bitmap.setBit(rowid, 0);
    existence_bitmap.setBit(number_of_rows, 1);
    bitmaps[to_val]->setBit(number_of_rows, 1);
    showEB = false;
    append_rowids.push_back(rowid);
    number_of_rows += 1;
}

void Table::remove(uint32_t rowid) {
    existence_bitmap.setBit(rowid, 0);
}

int Table::evaluate(uint32_t val, ibis::bitvector &res) {
    struct timeval before, after;
    gettimeofday(&before, NULL);
    if (existence_bitmap.all1s()) {
        res.copy(*bitmaps[val]);
    } else {
        res.copy(existence_bitmap);
        res &= *bitmaps[val];
    }
    gettimeofday(&after, NULL);
    cout << "Q And " << time_diff(before , after) << endl;
    gettimeofday(&before, NULL);
    auto cnt = res.do_cnt();
    gettimeofday(&after, NULL);
    cout << "Q Dec " << time_diff(before , after) << endl;
    return cnt;
}

void Table::printMemory() {
    uint64_t bitmap = 0, fence_pointers = 0;
    for (int i = 0; i < cardinality; ++i) {
        // bitmaps[i]->appendActive();
        bitmap += bitmaps[i]->getSerialSize();
        if (enable_fence_pointer) {
            fence_pointers += bitmaps[i]->index.size() * sizeof(int) * 2;
        }
    }
    std::cout << "M TT " << append_rowids.size() * sizeof(uint32_t) << std::endl;
    std::cout << "M EB " << existence_bitmap.getSerialSize() << std::endl;
    std::cout << "M BM " << bitmap << std::endl;
    std::cout << "M FP " << fence_pointers << std::endl;
}

void Table::printUncompMemory() {
    uint64_t bitmap = 0, fence_pointers = 0;
    for (int i = 0; i < cardinality; ++i) {
        // bitmaps[i]->appendActive();
        bitmaps[i]->decompress();
        bitmap += bitmaps[i]->getSerialSize();
        bitmaps[i]->compress();
        if (enable_fence_pointer) {
            fence_pointers += bitmaps[i]->index.size() * sizeof(int) * 2;
        }
    }
    std::cout << "UncM TT " << append_rowids.size() * sizeof(uint32_t) << std::endl;
    existence_bitmap.decompress();
    std::cout << "UncM EB " << existence_bitmap.getSerialSize() << std::endl;
    existence_bitmap.compress();
    std::cout << "UncM BM " << bitmap << std::endl;
    std::cout << "UncM FP " << fence_pointers << std::endl;
}
