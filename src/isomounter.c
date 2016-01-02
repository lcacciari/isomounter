#include "if_utils.h"


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
  if_status * status = if_status_new(argv[argc - 2]);
  argv[argc - 2] = argv[argc - 1];
  argc = argc - 1;
  g_message("call fuse_main");
  int result = fuse_main(argc,argv,&isofuse_ops,status);
  g_message("fuse main returned %d",result);
  if_status_destroy(status);
  return result;
}

