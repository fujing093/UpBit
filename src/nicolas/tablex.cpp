#include "fastbit/bitvector.h"

#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <sys/mman.h>
#include <queue>	// priority queue

#include "table.h"
#include "util.h"

using namespace nicolas;

TableX::TableX(uint32_t cardinality) : cardinality(cardinality), number_of_rows(0) {
    for (int i = 0; i < cardinality; ++i) {
        bitmaps.push_back(new ibis::bitvector());
    }
}

void TableX::appendRow(int val) {
    for (int i = 0; i < cardinality; ++i) {
        bitmaps[i]->setBit(number_of_rows, i == val);
    }
    number_of_rows += 1;
}

void TableX::write(std::string dir) {
    boost::filesystem::path dir_path(dir);
    boost::filesystem::create_directory(dir_path);
    for (int i = 0; i < cardinality; ++i) {
        std::stringstream ss;
        ss << dir << i << ".bm";
        bitmaps[i]->write(ss.str().c_str());
    }
}
