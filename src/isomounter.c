#include "common.h"
#include "if_utils.h"
#include "filechecks.h"
#include <glib/gstdio.h>

int main(int argc,char **argv) {
  GError *error = NULL;
  im_config_t * config = im_config_new();
  GOptionContext * ctx = g_option_context_new("path-to-image [mountpoint]");
  g_option_context_add_group(ctx,build_fuse_options(config));
  g_option_context_set_main_group(ctx,build_main_options(config));
  g_debug("parse options");
  gboolean ok = g_option_context_parse(ctx, &argc, &argv, &error);
  if (!ok) {
    g_error("option parsing failed: %s", error->message);
    exit (1);
  }
  ok = check_image_file(config->image_path);
  if (!ok) {
    g_error("no image path given");
    exit(1);
  }
  if_status * status = if_status_new(config);
  switch (check_mountpoint(config)) {
  case UNAVAILABLE:
    g_error("mount point %s can not be used",config->mountpoint);
    exit(1);
  case MANAGED:
    status->mountpoint_managed = TRUE;
    break;
  case UNMANAGED:
    if (config->manage_mp) {
      g_warning("mountpoint %s already exist. It will *not* be deleted on umount",config->mountpoint);
    }
    status->mountpoint_managed = FALSE;
    break;
  }
  int f_argc = 0;
  char ** f_argv;
  
  im_config_extract_fuse_args(config,argv[0],&f_argc,&f_argv);

  gchar * cl = g_strjoinv(" ",f_argv);
  g_debug("Arguments that will be passed to fuse_main: '%s'\n",cl);
  g_free(cl);
      
  g_debug("call fuse_main");
  g_print("%s\n",config->mountpoint);
  int result = fuse_main(f_argc,f_argv,&isofuse_ops,status);
  g_debug("fuse main returned %d",result);
  // doing cleanup if needed
  if (status->mountpoint_managed) {
      gint rc = g_rmdir(config->mountpoint);
      if (rc != 0) {
	g_warning("removal of mountpoint %s failed",config->mountpoint);
      }
  }    
  exit(result);
}

