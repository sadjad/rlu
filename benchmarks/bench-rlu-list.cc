#include <getopt.h>
#include <iostream>
#include <stdexcept>

#include "benchmark.hh"

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
       << "  -i, --initial-size <S=512>" << endl
       << "  -d, --duration <D=2s>" << endl
       << endl;

  exit(exit_code);
}

int main(int argc, char *argv[])
{
  try {
    if (argc <= 0) {
      abort();
    }

    if (argc < 1) {
      usage(argv[0], EXIT_FAILURE);
    }

    Benchmark::Config config;

    struct option long_options[] = {
        {"threads", required_argument, nullptr, 'n'},
        {"update-ratio", required_argument, nullptr, 'r'},
        {"min-value", required_argument, nullptr, 'm'},
        {"max-value", required_argument, nullptr, 'M'},
        {"initial-size", required_argument, nullptr, 'i'},
        {"duration", required_argument, nullptr, 'd'}};

    while (true) {
      const int opt = getopt_long(argc, argv, "n:r:m:M:i:d:h", long_options, 0);

      if (opt == -1) break;

      // clang-format off
      switch (opt) {
      case 'n': config.n_threads = stoul(optarg); break;
      case 'r': config.update_ratio = stof(optarg); break;
      case 'm': config.min_value = stol(optarg); break;
      case 'M': config.max_value = stol(optarg); break;
      case 'i': config.initial_size = stoul(optarg); break;
      case 'd': config.duration = chrono::seconds{stoul(optarg)}; break;
      case 'h': usage(argv[0], EXIT_SUCCESS); break;
      default: usage(argv[0], EXIT_FAILURE);
      }
      // clang-format on
    }

    if (config.update_ratio < 0 || config.update_ratio > 1 ||
        config.min_value > config.max_value) {
      usage(argv[0], EXIT_FAILURE);
    }

    Benchmark benchmark{config};
    benchmark.run();
  }
  catch (exception &ex) {
    print_exception(argv[0], ex);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
