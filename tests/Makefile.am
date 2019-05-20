AM_CPPFLAGS = -I$(srcdir)/../src $(CXX17_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

check_PROGRAMS = linked-list

linked_list_SOURCES = linked-list.cc
linked_list_LDADD = ../src/librlu.a -lpthread

TESTS = linked-list
