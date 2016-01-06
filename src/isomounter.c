#include "common.h"
#include "if_utils.h"
#include "im_config.h"
#include <glib/gstdio.h>

int main(int argc,char **argv) {
  GError *error = NULL;
#ifndef NDEBUG
  g_print("start with pid %d\n",getpid());
  g_print("initializing config\n");
#endif
  if (!im_init_config(&error)) {
    // memory erro, just exit
    exit(ENOMEM);
  }
#ifndef NDEBUG
  g_print("processing options\n");
#endif
  gboolean ok = process_options(&argc,&argv,&error);
#ifndef NDEBUG
  g_print("processing options done\n");
#endif
  if (!ok) {
    g_error("option parsing failed: %s", error->message);
    exit(1);
  }
#ifndef NDEBUG
  g_print("checking image file\n");
#endif
  ok = check_image_file(&error);
#ifndef NDEBUG
  g_print("checking image file done\n");
#endif
  if (!ok) {
    g_error("image file: %s",error->message);
    exit(1);
  }
#ifndef NDEBUG
  g_print("checking mountpoint\n");
#endif
  ok = check_mountpoint(&error);
#ifndef NDEBUG
  g_print("checking mountpoint done\n");
#endif
  if (!ok) {
    g_error("mountpoint: %s",error->message);
  }
#ifndef NDEBUG
  g_print("building fuse options\n");
#endif
  gchar ** f_argv = im_config_extract_fuse_args(argv[0],&error);
#ifndef NDEBUG
  g_print("fuse options built\n");
#endif
  if (f_argv == NULL) {
    g_error("failed to extract fuse arguments: %s",error->message);
    exit(1);
  }
  if_status * status = if_status_new();
  
  gint result = 0;
  
  if (! im_get_config()->dry_run) {
    result = fuse_main(g_strv_length(f_argv),f_argv,&isofuse_ops,status);
  } else {
    gchar * cline = g_strjoinv(" ",f_argv);
    g_print("invoking fuse_main with arguments:\n");
    g_print("\t%s\n",cline);
    g_free(cline);
  }
  if (result == 0 && status->mountpoint_managed) {
    if (! im_get_config()->dry_run) {
      result = g_rmdir(im_get_config()->mountpoint);
      if (result != 0) {
	g_warning("failed to remove managed mountpoint %s",im_get_config()->mountpoint);
      }
    } else {
      g_print("will remove managed mountpoint %s\n",im_get_config()->mountpoint);
    }
  }
  exit(result);
}

