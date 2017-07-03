#include "fastbit/bitvector.h"

#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <sys/mman.h>
#include "fastbit/bitvector.h"

#include "table.h"
#include "util.h"

#ifdef LINUX
#include <omp.h>
#endif

using namespace nicolas;
using namespace std;

const ibis::bitvector::word_t UB_PAD_BITS = (n_rows / 310) * 31;

Table::Table(uint32_t cardinality, uint32_t n_rows) : BaseTable(cardinality, n_rows) {
    update_bitmaps.reserve(cardinality);
    total_rows = n_rows + UB_PAD_BITS;
    for (uint32_t i = 0; i < cardinality; ++i) {
        bitmaps[i] = new ibis::bitvector();
        bitmaps[i]->read(getBitmapName(i).c_str());
        if (enable_fence_pointer)
            bitmaps[i]->buildIndex();
        bitmaps[i]->adjustSize(0, total_rows);
        update_bitmaps[i] = new ibis::bitvector();
        update_bitmaps[i]->adjustSize(0, total_rows);
    }
    threadPool = new ThreadPool(nThreads);
}

void Table::append(int val) {
    bitmaps[val]->setBit(number_of_rows, 1);
    number_of_rows += 1;
}

void Table::update(unsigned int rowid, int to_val) {
    int from_val = get_value(rowid);
    if (from_val == -1 || to_val == from_val)
        return;
    update_bitmaps[to_val]->setBit(rowid, !update_bitmaps[to_val]->getBit(rowid));
    update_bitmaps[from_val]->setBit(rowid, !update_bitmaps[from_val]->getBit(rowid));
}

void Table::remove(unsigned int rowid) {
    int val = get_value(rowid);
    if (val == -1)
        return;
    update_bitmaps[val]->setBit(rowid, !update_bitmaps[val]->getBit(rowid));
}


int Table::evaluate(uint32_t val, ibis::bitvector &res) {
    auto cnt = update_bitmaps[val]->cnt();

    if (cnt == 0) {
        res.copy(*bitmaps[val]);
    } else {
        res.copy(*update_bitmaps[val]);
        res ^= *bitmaps[val];
    }

    if (cnt > n_merge) {
        bitmaps[val]->index.clear();
        bitmaps[val]->copy(res);
        if (enable_fence_pointer)
            bitmaps[val]->buildIndex();
        update_bitmaps[val]->clear();
        update_bitmaps[val]->adjustSize(0, total_rows);
    }

    if (decode) {
        std::vector<uint32_t> dummy;
        res.decode(dummy);
        return 0;
    } else {
        return res.do_cnt();
    }
}

int Table::_get_value(uint32_t rowid, int curValue, volatile bool &flag) {
    int ret = -1;

    if (!flag) {
        auto bit1 = bitmaps[curValue]->getBitWithIndex(rowid);
        auto bit2 = update_bitmaps[curValue]->getBit(rowid);
        if ((bit1 ^ bit2) == 1) {
            flag = true;
            ret = curValue;
        }
    }

    return ret;
}

int Table::get_value(uint32_t rowid) {
    volatile bool flag = false;
    std::vector<std::future<int> > localResults;
    int begin = 0, offset = cardinality / nThreads;

    for (int i = 1; i <= nThreads; ++i) {
        if (flag)
            break;
        if (i == nThreads)
            offset += cardinality % nThreads;
        localResults.emplace_back(threadPool->enqueue(
                [this](uint32_t rowid, int value, volatile bool &flag, int begin, int offset) {
                    for (int j = 0; j < offset; ++j) {
                        int res = this->_get_value(rowid, begin + j, flag);
                        if (res != -1)
                            return res;
                    }
                    return -1;
                }, rowid, i, flag, begin, offset));
        begin += offset;
    }

    int ret = -1;
    for (auto &&result : localResults) {
        int res = result.get();
        if (res != -1) {
            ret = res;
            break;
        }
    }

    return ret;
}

void Table::printMemory() {
    uint64_t bitmap = 0, updateable_bitmap = 0, fence_pointers = 0;
    for (int i = 0; i < cardinality; ++i) {
        bitmap += bitmaps[i]->getSerialSize();
        fence_pointers += bitmaps[i]->index.size() * sizeof(int) * 2;
        updateable_bitmap += update_bitmaps[i]->getSerialSize();
    }
    std::cout << "M FP " << fence_pointers << std::endl;
    std::cout << "M UB " << updateable_bitmap << std::endl;
    std::cout << "M BM " << bitmap << std::endl;
}

void Table::printUncompMemory() {
    uint64_t bitmap = 0, updateable_bitmap = 0, fence_pointers = 0;
    for (int i = 0; i < cardinality; ++i) {
        // bitmaps[i]->appendActive();
        bitmaps[i]->decompress();
        bitmap += bitmaps[i]->getSerialSize();
        bitmaps[i]->compress();
        update_bitmaps[i]->decompress();
        updateable_bitmap += update_bitmaps[i]->getSerialSize();
        update_bitmaps[i]->compress();
        fence_pointers += bitmaps[i]->index.size() * sizeof(int) * 2;
    }
    std::cout << "UncM FP " << fence_pointers << std::endl;
    std::cout << "UncM UB " << updateable_bitmap << std::endl;
    std::cout << "UncM BM " << bitmap << std::endl;
}

int Table::range(uint32_t start, uint32_t range) {
    ibis::bitvector res;
    res.set(0, total_rows);
    if (range_algo == "naive") {
        for (uint32_t i = 0; i < range; ++i) {
            res |= *(bitmaps[start + i]);
        }
    } else if (range_algo == "pq") {
        typedef std::pair<ibis::bitvector*, bool> _elem;
        // put all bitmaps in a priority queue
        std::priority_queue<_elem> que;
        _elem op1, op2, tmp;
        tmp.first = 0;

        // populate the priority queue with the original input
        for (uint32_t i = 0; i < range; ++i) {
            op1.first = bitmaps[start + i];
            op1.second = false;
            que.push(op1);
        }

        while (! que.empty()) {
            op1 = que.top();
            que.pop();
            if (que.empty()) {
                res.copy(*(op1.first));
                if (op1.second) delete op1.first;
                break;
            }

            op2 = que.top();
            que.pop();
            tmp.second = true;
            tmp.first = *(op1.first) | *(op2.first);

            if (op1.second)
                delete op1.first;
            if (op2.second)
                delete op2.first;
            if (! que.empty()) {
                que.push(tmp);
                tmp.first = 0;
            }
        }
        if (tmp.first != 0) {
            if (tmp.second) {
                res |= *(tmp.first);
                delete tmp.first;
                tmp.first = 0;
            }
            else {
                res |= *(tmp.first);
            }
        }
    } else {
        auto end = start + range;
        while (start < end && bitmaps[start] == 0)
            ++ start;
        if (start < end) {
            res |= *(bitmaps[start]);
            ++ start;
        }
        res.decompress();
        for (uint32_t i = start; i < end; ++ i) {
            res |= *(bitmaps[i]);
        }
    }
    if (decode) {
        std::vector<uint32_t> dummy;
        res.decode(dummy);
        return 0;
    } else {
        return res.do_cnt();
    }
}
