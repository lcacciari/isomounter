AM_CFLAGS = $(GLIB_CFLAGS) $(FUSE_CFLAGS) $(ISO9660_CFLAGS)
AM_LDFLAGS = $(GLIB_LDFLAGS) $(FUSE_LDFLAGS) $(ISO9660_LDFLAGS)
if DEBUG
AM_CFLAGS += -g
else
AM_CFLAGS += -O2
endif

LIBS = $(GLIB_LIBS) $(FUSE_LIBS) $(ISO9660_LIBS)

bin_PROGRAMS=isomounter
isomounter_SOURCES=isomounter.c if_impl.c if_utils.c im_config.c \
                   common.h if_utils.h im_config.h


