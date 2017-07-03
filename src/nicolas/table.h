#ifndef TABLE_H_
#define TABLE_H_

#include <string>
#include <vector>

#include "fastbit/bitvector.h"
#include <thread>
#include "threadpool.h"
#include <future>

class BaseTable {
public:
    BaseTable(uint32_t cardinality, uint32_t n_rows) : cardinality(cardinality), number_of_rows(n_rows) {
        bitmaps.reserve(cardinality);
    }

    virtual void update(unsigned int rowid, int to_val) { return; }

    virtual void remove(unsigned int rowid) { return; }

    virtual void append(int val) { return; }

    virtual int evaluate(uint32_t val, ibis::bitvector &res) { return 0; }

    virtual void printMemory() { return; }

    virtual void printUncompMemory() { return; }

    std::vector<ibis::bitvector *> bitmaps;
protected:
    const uint32_t cardinality;
    uint32_t number_of_rows;

    std::string getBitmapName(int val) {
        std::stringstream ss;
        ss << INDEX_PATH << val << ".bm";
        return ss.str();
    }
};

namespace nicolas {

    class Table : public BaseTable {
    public:
        Table(uint32_t cardinality, uint32_t n_rows);

        void update(unsigned int rowid, int to_val);

        void remove(unsigned int rowid);

        void append(int val);

        int evaluate(uint32_t val, ibis::bitvector &res);

        void printMemory();

        void printUncompMemory();

        int get_value(uint32_t rowid);

        int range(uint32_t start, uint32_t range);

    protected:
        std::vector<ibis::bitvector *> update_bitmaps;
        uint32_t total_rows;
        ThreadPool *threadPool;

        int _get_value(uint32_t rowid, int curValue, volatile bool &flag);
    };

    class TableX {
    public:
        TableX(uint32_t cardinality);

        void appendRow(int val);

        void write(std::string dir);

    protected:
        const uint32_t cardinality;
        uint32_t number_of_rows;

        std::vector<ibis::bitvector *> bitmaps;
    };

};

#endif
