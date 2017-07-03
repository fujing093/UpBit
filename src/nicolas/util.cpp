#include "nicolas/util.h"
#include "fastbit/bitvector.h"

uint32_t cardinality = 100;
string DATA_PATH;
string INDEX_PATH;
uint32_t n_rows;
uint32_t n_deletes;
uint32_t n_queries;
uint32_t n_merge;
bool verbose;
bool enable_fence_pointer;
bool show_memory;
bool on_disk;
string approach;
bool breakdown;
unsigned int nThreads;
bool cache_misses;
bool perf;
unsigned int time_out;
uint32_t n_range;
string range_algo;
bool showEB;
bool decode;

double time_diff(struct timeval x, struct timeval y) {
    double x_ms , y_ms , diff;
    x_ms = (double) x.tv_sec * 1000000 + (double) x.tv_usec;
    y_ms = (double) y.tv_sec * 1000000 + (double) y.tv_usec;
    diff = y_ms - x_ms;
    return diff;
}

po::variables_map get_options(const int argc, const char *argv[]) {
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
            ("mode,m", po::value<string>(), "build / query / test")
            ("data-path,d", po::value<string>(), "data file path")
            ("index-path,i", po::value<string>(), "index file path")
            ("number-of-rows,n", po::value<uint32_t>(), "number of rows")
            ("number-of-queries", po::value<uint32_t>(), "number of queries")
            ("merge", po::value<uint32_t>()->default_value(0), "merge UB")
            ("verbose,v", po::value<bool>()->default_value(true), "verbose")
            ("removed,r", po::value<uint32_t>(), "number of deleted rows")
            ("fence-pointer", po::value<bool>()->default_value(false), "enable fence pointers")
            ("show-memory", po::value<bool>()->default_value(false), "show memory")
            ("help", "produce help message")
            ("on-disk", po::value<bool>()->default_value(false), "on disk")
            ("approach,a", po::value<string>()->default_value(std::string("ub")), "in-place, ucb, ub")
            ("fence-length", po::value<unsigned int>()->default_value(1000), "lengh of fence pointers")
            ("breakdown", po::value<bool>()->default_value(false), "breakdown")
            ("num-threads", po::value<unsigned int>()->default_value(16), "num of threads")
            ("cache-misses", po::value<bool>()->default_value(false), "measure cache misses")
            ("perf", po::value<bool>()->default_value(false), "enable perf")
            ("time-out,t", po::value<unsigned int>()->default_value(0), "time out (s)")
            ("range", po::value<uint32_t>()->default_value(0), "range length")
            ("range-algorithm", po::value<string>()->default_value(std::string("pq")), "pq, naive, uncompress")
            ("cardinality,c", boost::program_options::value<uint32_t >()->default_value(1), "cardinality");

    po::positional_options_description p;
    p.add("mode", -1);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << endl;
        exit(1);
    }

    return vm;
}

void init(po::variables_map options) {
    if (options.count("data-path"))
        DATA_PATH = options["data-path"].as<string>();
    if (options.count("cardinality"))
        cardinality = options["cardinality"].as<uint32_t>();
    if (options.count("index-path"))
        INDEX_PATH = options["index-path"].as<string>();
    if (options.count("number-of-rows"))
        n_rows = options["number-of-rows"].as<uint32_t>();
    if (options.count("removed"))
        n_deletes = options["removed"].as<uint32_t>();
    if (options.count("number-of-queries"))
        n_queries = options["number-of-queries"].as<uint32_t>();
    n_merge = options["merge"].as<uint32_t>();
    verbose = options["verbose"].as<bool>();
    enable_fence_pointer = options["fence-pointer"].as<bool>();
    show_memory = options["show-memory"].as<bool>();
    on_disk = options["on-disk"].as<bool>();
    approach = options["approach"].as<string>();
    INDEX_WORDS = options["fence-length"].as<unsigned int>();
    breakdown = options["breakdown"].as<bool>();
    nThreads = options["num-threads"].as<unsigned int>();
    cache_misses = options["cache-misses"].as<bool>();
    perf = options["perf"].as<bool>();
    time_out = options["time-out"].as<unsigned int>();
    n_range = options["range"].as<uint32_t>();
    range_algo = options["range-algorithm"].as<string>();
    showEB = false;
    decode = false;
}
