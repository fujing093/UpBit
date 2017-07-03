#include <iostream>
#include <sys/mman.h>
#include <vector>
#include <random>
#include <chrono>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include "fastbit/bitvector.h"
#include "nicolas/table.h"
#include "nicolas/util.h"
#include "ucb/table.h"
#include "naive/table.h"

#ifdef LINUX
#include "nicolas/perf.h"
#endif

using namespace std;
using namespace nicolas;

void build_index() {
    ibis::horometer timer;
    timer.start();

    int *src;
    int fdes = open(DATA_PATH.c_str(), OPEN_READONLY);
    struct stat statbuf;
    fstat(fdes, &statbuf);
    src = (int *) mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fdes, 0);

    TableX table(cardinality);
    for (int i = 0; i < n_rows; ++i) {
        table.appendRow(src[i] - 1);
    }
    table.write(INDEX_PATH);

    timer.stop();
    std::cout << "nicolas:: build_index() took "
    << timer.CPUTime() << " CPU seconds, "
    << timer.realTime() << " elapsed seconds" << endl;
}

void evaluate(string mode) {
    auto seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);
    uniform_int_distribution<int> distribution(0, cardinality - 1);
    uniform_int_distribution<uint32_t> rid_distribution(0, n_rows - 1);

    BaseTable *table;

    if (approach == "ub") {
        table = new nicolas::Table(cardinality, n_rows);
    } else if (approach == "ucb") {
        table = new ucb::Table(cardinality, n_rows);
    } else if (approach == "naive") {
        table = new naive::Table(cardinality, n_rows);
    } else {
        cerr << "Unknown approach." << endl;
        exit(-1);
    }

    struct timeval before, after;
    struct timeval time_out_begin, time_out_end;
    vector<double> times;

    double ratio = (double) n_deletes / (double) (n_deletes + n_queries);
    cout << "ratio = " << ratio << endl;

    uniform_real_distribution<double> ratio_distribution(0.0, 1.0);
    string _mode = mode;

#ifdef LINUX
    int eventc = 1;
    int eventv[2] = {PERF_COUNT_HW_CACHE_MISSES, PERF_TYPE_HARDWARE};
    long long eventval[1] = {0};
    int perf_pid;

    if (perf) {
        perf_pid = fork();
        if (perf_pid > 0) {
            // parent
        } else if (perf_pid == 0) {
            // child
            perf_pid = getppid();
            char perf_pid_opt[24];
            snprintf(perf_pid_opt, 24, "%d", perf_pid);
            char *perfargs[10] = {"amplxe-perf", "stat", "-x,", "-e",
                "cache-references,cache-misses,cycles,instructions,branches,branch-misses,page-faults,cpu-migrations", "-p",
                perf_pid_opt, "--log-fd", "2", NULL};
            execvp("perf", perfargs);
            assert(0 && "perf failed to start");
        } else {
            perror("fork did not.");
        }
        usleep(1000);
    }
#endif

    if (show_memory) {
        table->printMemory();
        table->printUncompMemory();
    }

    gettimeofday(&time_out_begin, NULL);
    for (int i = 0; i < n_queries + n_deletes; ++i) {

#ifdef LINUX
        int *fd;
        if (cache_misses)
            fd = start_measuring(eventc, eventv);
#endif

        if (ratio_distribution(generator) < ratio) {
            uint32_t rid = rid_distribution(generator);
            int val = distribution(generator);
            if (_mode == "mix") {
                int test = rid_distribution(generator);
                switch (test % 3) {
                    case 0:
                        mode = "update";
                        break;
                    case 1:
                        mode = "delete";
                        break;
                    case 2:
                        mode = "insert";
                        break;
                }
            }
            if (mode == "update") {
                gettimeofday(&before, NULL);
                table->update(rid, val);
                gettimeofday(&after, NULL);
                times.push_back(time_diff(before, after));
                if (verbose)
                    cout << "U " << time_diff(before, after) << endl;
            }
            else if (mode == "delete") {
                gettimeofday(&before, NULL);
                table->remove(rid);
                gettimeofday(&after, NULL);
                times.push_back(time_diff(before, after));
                if (verbose)
                    cout << "D " << time_diff(before, after) << endl;
            }
            else if (mode == "insert") {
                gettimeofday(&before, NULL);
                table->append(val);
                gettimeofday(&after, NULL);
                times.push_back(time_diff(before, after));
                if (verbose)
                    cout << "I " << time_diff(before, after) << endl;
            }
        } else {
            int r = distribution(generator);
            ibis::bitvector res;
            gettimeofday(&before, NULL);
            table->evaluate(r, res);
            gettimeofday(&after, NULL);
            times.push_back(time_diff(before, after));
            if (verbose)
                cout << "Q " << time_diff(before, after) << endl;
        }

        if (show_memory && i % 1000 == 0) {
            table->printMemory();
            table->printUncompMemory();
        }

        if (times.size() > 1000 && !verbose) {
            double sum = std::accumulate(times.begin(), times.end(), 0.0);
            double mean = sum / times.size();
            double sq_sum = std::inner_product(times.begin(), times.end(), times.begin(), 0.0);
            double stddev = std::sqrt(sq_sum / times.size() - mean * mean);
            printf("time:\t %.0lf us \tstddev: %.0lf us\n", mean, stddev);
            times.clear();
        }

#ifdef LINUX
        if (cache_misses) {
            stop_measuring(fd, eventc, eventval);
            print_event_counts(eventc, eventval);
        }
#endif

        if (time_out > 0 && i % 10 == 0) {
            gettimeofday(&time_out_end, NULL);
            if (time_diff(time_out_begin, time_out_end) > time_out * 1000000) {
                break;
            }
        }
    }

    if (show_memory) {
        table->printMemory();
        table->printUncompMemory();
    }

#ifdef LINUX
    if (perf) {
        kill(perf_pid, SIGINT);
    }
#endif
}

void evaluateGetValue();
void evaluateImpact() {
    auto seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);
    uniform_int_distribution<int> distribution(0, cardinality - 1);
    uniform_int_distribution<uint32_t> rid_distribution(0, n_rows - 1);

    struct timeval before, after;
    struct timeval time_out_begin, time_out_end;

    uniform_real_distribution<double> ratio_distribution(0.0, 1.0);

    int comp[] = {5,10,15,20,50,100,200,300,500,1000,2000,5000,10000};

    gettimeofday(&time_out_begin, NULL);
    for (int i = 0; i < 13; ++i) {
        BaseTable *table = new nicolas::Table(cardinality, n_rows);
        evaluateGetValue();

        int thres = comp[i];

        cout << "Current comp: " << thres << endl;

        for (int j = 0; j < thres; ++j) {
            uint32_t rid = rid_distribution(generator);
            int val = 0;
            gettimeofday(&before, NULL);
            table->update(rid, val);
            gettimeofday(&after, NULL);
            cout << "U " << time_diff(before, after) << endl;
        }

        ibis::bitvector res;
        gettimeofday(&before, NULL);
        table->evaluate(0, res);
        gettimeofday(&after, NULL);
        cout << "Q " << time_diff(before, after) << endl;
    }
}

void evaluateGetValue() {
    nicolas::Table table(cardinality, n_rows);

    auto seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);
    uniform_int_distribution<uint32_t> distribution(0, n_rows - 1);

    struct timeval before, after;
    gettimeofday(&before, NULL);
    for (int i = 0; i < 10000; ++i) {
        int test = distribution(generator);
        table.get_value(test);
    }
    gettimeofday(&after, NULL);
    cout << time_diff(before, after) << endl;
}

void evaluateRange() {
    auto seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);
    uniform_int_distribution<uint32_t> distribution(0, cardinality - 1 - n_range);

    nicolas::Table *table = new nicolas::Table(cardinality, n_rows);

    struct timeval before, after;
    struct timeval time_out_begin, time_out_end;

    cout << "range = " << n_range << endl;

    gettimeofday(&time_out_begin, NULL);
    for (int i = 0; i < n_queries; ++i) {
        auto r = distribution(generator);
        gettimeofday(&before, NULL);
        table->range(r, n_range);
        gettimeofday(&after, NULL);

        if (verbose)
            cout << "Q " << time_diff(before, after) << endl;

        if (time_out > 0 && i % 10 == 0) {
            gettimeofday(&time_out_end, NULL);
            if (time_diff(time_out_begin, time_out_end) > time_out * 1000000) {
                break;
            }
        }
    }
}

int main(const int argc, const char *argv[]) {
    po::variables_map options = get_options(argc, argv);
    init(options);

    if (options.count("mode")) {
        string mode = options["mode"].as<string>();
        if (mode == "build") {
            build_index();
        } else if (mode == "getvalue") {
            evaluateGetValue();
        } else if (mode == "impact") {
            evaluateImpact();
        } else if (mode == "range") {
            evaluateRange();
        } else {
            evaluate(mode);
        }
    }

    return 0;
}
