#include "if_utils.h"


int main(int argc,char **argv) {
  /*
   * The new command line:
   * - preparse mountpoint and image paths (so to handle encoding)
   * - also preparse fuse options, so that we are aware of what
   *   is going on
   * - add default options such as --help
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
  // not usefull: we ar going to release all memory anyway. But dangerous
  // if this part of code is executed twice!
  //if_status_destroy(status);
  return result;
}

