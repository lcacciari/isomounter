#include "if_utils.h"
#include "mountpoint.h"
#include <glib/gstdio.h>

int main(int argc,char **argv) {
  /*
   * The new command line:
   * - preparse mountpoint and image paths (so to handle encoding)
   * - also preparse fuse options, so that we are aware of what
   *   is going on
   * - add default options such as --help
   */
  GError *error = NULL;
  g_message("create config");
  im_config_t * config = im_config_new();
  g_message("build context");
  GOptionContext * ctx = g_option_context_new("- mounting ISO images in userspace");
  g_option_context_add_group(ctx,build_fuse_options(config));
  g_option_context_set_main_group(ctx,build_main_options(config));
  g_message("parse options");
  gboolean ok = g_option_context_parse(ctx, &argc, &argv, &error);
  if (!ok) {
    g_error("option parsing failed: %s", error->message);
    return (1);
  }
  ok = check_image_file(config->image_path);
  if (!ok) {
    g_error("no image path given");
    return(1);
  }
  if_status * status = if_status_new(config);
  switch (check_mountpoint(config)) {
  case UNAVAILABLE:
    g_error("mount point %s can not be used",config->mountpoint);
    return 1;
  case MANAGED:
    g_print("mountpoint %s has been created and will be deleted on umount",config->mountpoint);
    status->mountpoint_managed = TRUE;
    break;
  case UNMANAGED:
    if (config->manage_mp) {
      g_warning("mountpoint %s already exist. It will *not* be deleted on umount",config->mountpoint);
    } else {
      g_print("will use mountpoint %s",config->mountpoint);
    }
    status->mountpoint_managed = FALSE;
    break;
  }
  int f_argc = 0;
  char ** f_argv;
  
  im_config_extract_fuse_args(config,argv[0],&f_argc,&f_argv);

  gchar * cl = g_strjoinv(" ",f_argv);
  g_print("Arguments that will be passed to fuse_main: '%s'\n",cl);
  g_free(cl);
      
  g_message("call fuse_main");
  int result = fuse_main(f_argc,f_argv,&isofuse_ops,status);
  g_message("fuse main returned %d",result);
  // doing cleanup if needed
  /* If the mountpoint is managed it need to be removed. This should be done
   * by the forked child (if any).
   * if either -f or -d where specified, there is no child
   * otherwhise, if the mount failed the phase was set to ON_ERROR and no chile was created(?)
   * if a child is created, on termination it will have the phase set to AFTER_UMOUNT
   */
  if ((config->debug || config->foreground) /* we never forked */ ||
      (status->phase != AT_START) /* we are the forked child or no child was forked */) {
    if (status->mountpoint_managed) {
      // try to remove the mountpoint
      g_print("trying to remove mountpoit %s",config->mountpoint);
      gint rc = g_rmdir(config->mountpoint);
      if (rc != 0) {
	g_warning("removal of mountpoint %s failed",config->mountpoint);
      }
    }
  }
  return result;
}

