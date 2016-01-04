#include "common.h"
#include "filechecks.h"
#include <glib/gstdio.h>
#include <unistd.h>




mountpoint_status check_mountpoint(im_config_t * config) {
  if (config->mountpoint == NULL) {
    // set to the defaul value before going on
    gchar * name = g_path_get_basename(config->image_path);
    config->mountpoint = g_build_filename(config->base_dir,name,NULL);
    g_free(name);
  } 
  const gchar * path = config->mountpoint;
  if (g_file_test(path,G_FILE_TEST_IS_DIR) && (g_access(path,W_OK) == 0)) {
    return UNMANAGED;
  } else if (config->manage_mp && (g_mkdir(path,0777) == 0)) {
    return MANAGED;
  }
  return UNAVAILABLE;
}

// check only that exists and is writable. should check is a iso file?
gboolean check_image_file(gchar * path) {
  if (path == NULL) {
    return FALSE;
  } else if (! g_file_test(path,G_FILE_TEST_IS_REGULAR)) {
    return FALSE;
  } else if (g_access(path,R_OK) != 0) {
    return FALSE;
  }
  return TRUE;
}

