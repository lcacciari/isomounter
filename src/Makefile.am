
bin_PROGRAMS=isomount
isomount_SOURCES=isomount.c iso9660fuse.c

isomount_CFLAGS = $(DEPS_CFLAGS)
isomount_LDADD = $(DEPS_LIBS)
if DEBUG
isomount_CFLAGS += -g
else
isomount_CFLAGS += -O2
endif
