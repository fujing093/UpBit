#include "fastbit/bitvector.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <cstdlib>

#include "naive/table.h"
#include "nicolas/util.h"

#ifdef LINUX
#include <omp.h>
#endif

using namespace naive;
using namespace std;

Table::Table(uint32_t cardinality, uint32_t n_rows) : BaseTable(cardinality, n_rows) {
    threadPool = new ThreadPool(nThreads);
    for (uint32_t i = 0; i < cardinality; ++i) {
        bitmaps[i] = new ibis::bitvector();
        bitmaps[i]->read(getBitmapName(i).c_str());
    }
}

void Table::append(int val) {
    if (on_disk) {

    }
    else {
        number_of_rows += 1;
        bitmaps[val]->setBit(number_of_rows, 1);
        for (int i = 0; i < cardinality; ++i) {
            bitmaps[i]->adjustSize(0, number_of_rows);
        }
    }
}

void Table::update(uint32_t rowid, int to_val) {
    struct timeval before, after;
    int from_val = get_value(rowid);
    if (from_val == to_val || from_val == -1)
        return;
    if (on_disk) {
        ibis::bitvector bitvector1, bitvector2;

        gettimeofday(&before, NULL);
        bitvector1.read(getBitmapName(from_val).c_str());
        bitvector2.read(getBitmapName(to_val).c_str());
        gettimeofday(&after, NULL);
        cout << "U READ " << time_diff(before , after) << endl;

        gettimeofday(&before, NULL);
        bitvector1.setBit(rowid, 0);
        bitvector2.setBit(rowid, 0);
        gettimeofday(&after, NULL);
        cout << "U SETBIT " << time_diff(before , after) << endl;

        gettimeofday(&before, NULL);
        bitvector1.write(getBitmapName(from_val).c_str());
        bitvector2.write(getBitmapName(to_val).c_str());
        gettimeofday(&after, NULL);
        cout << "U WRITE " << time_diff(before , after) << endl;
    }
    else {
        gettimeofday(&before, NULL);
        bitmaps[from_val]->setBit(rowid, 0);
        bitmaps[to_val]->setBit(rowid, 1);
        gettimeofday(&after, NULL);
        cout << "U SETBIT " << time_diff(before , after) << endl;
//        if (enable_fence_pointer) {
//            bitmaps[from_val]->buildIndex();
//            bitmaps[to_val]->buildIndex();
//        }
    }
}

void Table::remove(uint32_t rowid) {
    auto val = get_value(rowid);
    if (val == -1)
        return;
    if (on_disk) {
        ibis::bitvector bitvector;
        bitvector.read(getBitmapName(val).c_str());
        bitvector.setBit(rowid, 0);
        bitvector.write(getBitmapName(val).c_str());
    }
    else {
        bitmaps[val]->setBit(rowid, 0);
//        if (enable_fence_pointer) {
//            bitmaps[val]->buildIndex();
//        }
    }
}

int Table::evaluate(uint32_t val, ibis::bitvector &res) {
    struct timeval before, after;
    if (on_disk) {
        string name = getBitmapName(val);
        gettimeofday(&before, NULL);
        res.read(name.c_str());
        gettimeofday(&after, NULL);
        cout << "Q READ " << time_diff(before , after) << endl;
    }
    else {
        gettimeofday(&before, NULL);
        res.copy(*bitmaps[val]);
        gettimeofday(&after, NULL);
        cout << "Q COPY " << time_diff(before , after) << endl;
    }
    gettimeofday(&before, NULL);
    auto cnt = res.do_cnt();
    gettimeofday(&after, NULL);
    cout << "Q DEC " << time_diff(before , after) << endl;
    return cnt;
}

int Table::_get_value(uint32_t rowid, int curValue, volatile bool &flag) {
    int ret = -1;

    if (!flag) {
        auto bit1 = bitmaps[curValue]->getBit(rowid);
        if (flag)
            return ret;
        if (bit1 == 1) {
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
    uint64_t bitmap = 0;
    for (int i = 0; i < cardinality; ++i) {
        // bitmaps[i]->appendActive();
        bitmap += bitmaps[i]->getSerialSize();
    }
    std::cout << "M BM " << bitmap << std::endl;
}

void Table::printUncompMemory() {
    uint64_t bitmap = 0;
    for (int i = 0; i < cardinality; ++i) {
        // bitmaps[i]->appendActive();
        bitmaps[i]->decompress();
        bitmap += bitmaps[i]->getSerialSize();
        bitmaps[i]->compress();
    }
    std::cout << "UncM BM " << bitmap << std::endl;
}