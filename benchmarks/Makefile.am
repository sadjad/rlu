AM_CPPFLAGS = -I$(srcdir)/../src $(CXX17_FLAGS) $(URCU_CFLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = bench-list

bench_list_SOURCES = bechmark.hh benchmark.cc bench-list.cc rcu-list.hh \
                     rcu-list.cc

bench_list_LDADD = ../src/librlu.a $(URCU_LIBS) -lpthread
