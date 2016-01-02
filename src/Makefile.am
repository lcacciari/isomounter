CFLAGS = $(DEPS_CFLAGS)
LDADD = $(DEPS_LIBS)
if DEBUG
CFLAGS += -g
else
CFLAGS += -O2
endif

bin_PROGRAMS=isomount
isomount_SOURCES=isomount.c iso9660fuse.c
