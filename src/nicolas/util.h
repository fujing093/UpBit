#include <string>
#include <iostream>
#include <numeric> // std::accumulate
#include <cmath>   // std::minus
#include <boost/program_options.hpp>

using namespace std;

namespace po = boost::program_options;

double time_diff(struct timeval x , struct timeval y);
po::variables_map get_options(const int argc, const char *argv[]);
void init(po::variables_map options);

extern uint32_t cardinality;
extern string DATA_PATH;
extern string INDEX_PATH;
extern uint32_t n_rows;
extern uint32_t n_deletes;
extern uint32_t n_queries;
extern uint32_t n_merge;
extern bool verbose;
extern bool enable_fence_pointer;
extern bool show_memory;
extern bool on_disk;
extern string approach;
extern bool breakdown;
extern unsigned int nThreads;
extern bool cache_misses;
extern bool perf;
extern unsigned int time_out;
extern uint32_t n_range;
extern string range_algo;
extern bool showEB;
extern bool decode;
