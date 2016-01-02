
bin_PROGRAMS=isomounter
isomounter_SOURCES=isomounter.c if_impl.c if_utils.c

isomounter_CFLAGS = $(DEPS_CFLAGS)
isomounter_LDADD = $(DEPS_LIBS)
if DEBUG
isomounter_CFLAGS += -g
else
isomounter_CFLAGS += -O2
endif
