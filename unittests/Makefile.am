
EXTRA_DIST      = suppressions.valgrind

noinst_PROGRAMS = test bfc_sample recovery

AM_CPPFLAGS     = -I$(top_builddir)/include

test_SOURCES    = log.cpp \
                  journal.cpp \
                  approx.cpp \
                  api110.cpp \
                  btree_erase.cpp \
                  btree_insert.cpp \
                  check.cpp \
                  error.cpp \
                  filter.cpp \
                  cppapi.cpp \
                  recno.cpp \
                  duplicates.cpp \
                  env.cpp \
                  hamsterdb.cpp \
                  blob.cpp \
                  extkeys.cpp \
                  btree_key.cpp \
                  partial.cpp \
                  btree_cursor.cpp \
                  misc.cpp \
                  device.cpp \
                  os.cpp \
                  cache.cpp \
                  freelist.cpp \
                  db.cpp \
                  main.cpp \
                  endian.cpp \
                  page.cpp \
                  changeset.cpp \
                  txn.cpp \
                  txn_cursor.cpp \
                  cursor.cpp \
                  empty_sample.cpp \
                  bfc-testsuite.cpp \
                  bfc-testsuite.hpp \
                  os.hpp \
                  hamster_fixture.hpp \
                  bfc-signal.h \
                  bfc-signal.c

test_LDADD      = $(top_builddir)/src/libhamsterdb.la -lpthread -ldl

if ENABLE_REMOTE
test_SOURCES   += remote.cpp
CFLAGS         += -DHAM_ENABLE_REMOTE
test_LDADD     += $(top_builddir)/src/server/libhamserver.la -lprotobuf
endif

bfc_sample_SOURCES = bfc-sample.cpp \
                  empty_sample.cpp \
                  bfc-testsuite.cpp \
                  bfc-testsuite.hpp \
                  os.hpp \
                  bfc-signal.h \
                  bfc-signal.c

valgrind:
	valgrind --suppressions=suppressions.valgrind --leak-check=full \
			--num-callers=20 --show-reachable=yes .libs/lt-test 

bfc_sample_LDADD = -ldl

recovery_SOURCES = recovery.cpp

recovery_LDADD   = $(top_builddir)/src/libhamsterdb.la -ldl

noinst_BIN       = test bfc_sample recovery

#
# make sure that our modified CFLAGS are also respected by g++
#
AM_CXXFLAGS     = $(CFLAGS)
CXXFLAGS        = $(CFLAGS)

test_recovery:
	perl recovery.pl
