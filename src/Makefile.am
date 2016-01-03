AM_CFLAGS = $(GLIB_CFLAGS) $(FUSE_CFLAGS) $(ISO9660_CFLAGS)
if DEBUG
AM_CFLAGS += -g
else
AM_CFLAGS += -O2
endif

LIBS = $(GLIB_LIBS) $(FUSE_LIBS) $(ISO9660_LIBS)

bin_PROGRAMS=isomounter config_test
isomounter_SOURCES=isomounter.c if_impl.c if_utils.c im_config.c mountpoint.c
config_test_SOURCES=config_test.c im_config.c


