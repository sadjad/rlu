#include <getopt.h>
#include <chrono>
#include <iostream>
#include <stdexcept>

#include "list.hh"

using namespace std;

inline void print_exception(const char *argv0, const exception &e)
{
  cerr << argv0 << ": " << e.what() << endl;
}

void usage(const char *argv0, const int exit_code)
{
  cerr << "usage: " << argv0 << " [OPTIONS]" << endl
       << endl
       << "options:" << endl
       << "  -n, --threads <N=8>" << endl
       << "  -r, --update-ratio <R=0.02>" << endl
       << "  -m, --min-value <V=-1024>" << endl
       << "  -M, --max-value <V=1023>" << endl
       << endl;

  exit(exit_code);
}

struct BenchmarkConfig {
  size_t n_threads = 8;
  float update_ratio = 0.02;
  int32_t min_value = -1024;
  int32_t max_value = 1023;
};

int main(int argc, char *argv[])
{
  try {
    if (argc <= 0) {
      abort();
    }

    if (argc < 1) {
      usage(argv[0], EXIT_FAILURE);
    }

    BenchmarkConfig config;

    struct option long_options[] = {
        {"threads", required_argument, nullptr, 'n'},
        {"update-ratio", required_argument, nullptr, 'r'},
        {"min-val", required_argument, nullptr, 'm'},
        {"max-val", required_argument, nullptr, 'M'}};

    while (true) {
      const int opt = getopt_long(argc, argv, "n:r:m:M:h", long_options, 0);

      if (opt == -1) break;

      // clang-format off
      switch (opt) {
      case 'n': config.n_threads = stoul(optarg); break;
      case 'r': config.update_ratio = stof(optarg); break;
      case 'm': config.min_value = stol(optarg); break;
      case 'M': config.max_value = stol(optarg); break;
      case 'h': usage(argv[0], EXIT_SUCCESS); break;
      default: usage(argv[0], EXIT_FAILURE);
      }
      // clang-format on
    }

    if (config.update_ratio < 0 || config.update_ratio > 1 ||
        config.min_value > config.max_value) {
      usage(argv[0], EXIT_FAILURE);
    }
  }
  catch (exception &ex) {
    print_exception(argv[0], ex);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
