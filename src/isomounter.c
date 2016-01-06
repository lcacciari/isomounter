#include "common.h"
#include "if_utils.h"
#include "im_config.h"
#include <glib/gstdio.h>

G_DEFINE_QUARK(isomounter-error-quark,im_error);

int main(int argc,char **argv) {
  GError *error = NULL;
  g_print("initializing config\n");
  if (!im_init_config(&error)) {
    // memory erro, just exit
    exit(ENOMEM);
  }
  g_print("processing options\n");
  gboolean ok = process_options(&argc,&argv,&error);
  g_print("processing options done\n");
  if (!ok) {
    g_error("option parsing failed: %s", error->message);
    exit(1);
  }
  g_print("checking image file\n");
  ok = check_image_file(&error);
  g_print("checking image file done\n");
  if (!ok) {
    g_error("image file: %s",error->message);
    exit(1);
  }
  g_print("checking mountpoint\n");
  ok = check_mountpoint(&error);
  g_print("checking mountpoint done\n");
  if (!ok) {
    g_error("mountpoint: %s",error->message);
  }
  g_print("building fuse options\n");
  gchar ** f_argv = im_config_extract_fuse_args(argv[0],&error);
  g_print("fuse options built\n");
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
  exit(result);
}

