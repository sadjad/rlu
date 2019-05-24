AM_CPPFLAGS = -I$(srcdir)/../src $(CXX17_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = bench-rlu-list

bench_rlu_list_SOURCES = bechmark.hh benchmark.cc bench-rlu-list.cc
bench_rlu_list_LDADD = ../src/librlu.a -lpthread
