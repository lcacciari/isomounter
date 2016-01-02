#include <glib.h>
#include <fuse/fuse.h>

#include "iso9660fuse.h"

int main(int argc,char **argv) {
  /*
   * for now, our command line is very simple:
   * All switch are for FUSE, we are only interested
   * is the second to last parameter: the path to the
   * image file
   */
  if ((argc < 3) ||
      (argv[argc - 2][0] == '-') ||
      (argv[argc - 1][0] == '-')) {
    g_error("bad command line");
    return -1;
  }
  char * pathtoimage = argv[argc - 2];
  argv[argc - 2] = argv[argc - 1];
  argc = argc - 1;
  g_message("call fuse_main");
  int result = fuse_main(argc,argv,&isofuse_ops,pathtoimage);
  g_message("fuse main returned %d",result);
  return result;
}

