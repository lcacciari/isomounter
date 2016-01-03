#include "if_utils.h"
#include <time.h>

#define DEFAULT_FILE_PERMISSIONS S_IRUSR | S_IRGRP | S_IROTH
#define DEFAULT_DIR_PERMISSIONS DEFAULT_FILE_PERMISSIONS | S_IXUSR | S_IXGRP | S_IXOTH 

if_status * if_status_new(im_config_t * config) {
  if_status * status = g_malloc0(sizeof(if_status));
  if (status != NULL) {
    status->path = g_strdup(config->image_path);
    status->default_file_mode = DEFAULT_FILE_PERMISSIONS | S_IFREG;
    status->default_dir_mode = DEFAULT_DIR_PERMISSIONS | S_IFDIR;
  }
  return status;
}

void if_status_destroy(if_status * status) {
  if (status != NULL) {
    g_free(status->path);
    g_free(status);
  }
}

if_status * get_status() {
  struct fuse_context * ctx = fuse_get_context();
  return ctx != NULL ? (if_status *) ctx->private_data : NULL;
}

int translate_stat(iso9660_stat_t * src,struct stat * dest) {
  struct fuse_context * ctx = fuse_get_context();
  if_status * status = (if_status *) ctx->private_data;
  // dest->st_dev ignored
  // dest->st_ino ignored. Can be useful
  dest->st_mode = IS_DIRECTORY(src) ?
    status->default_dir_mode & ~ ctx->umask :
    status->default_file_mode  & ~ ctx->umask;
  dest->st_nlink = 1; // ???
  dest->st_uid = ctx->uid;
  dest->st_gid = ctx->gid;
  dest->st_rdev = 0; // there are no special files
  dest->st_size = src->size; // TODO Check endianity
  dest->st_blocks = 0; // ????
  time_t ct = mktime(&src->tm);
  // we don't keep track of last access...
  dest->st_atim.tv_sec =
    dest->st_mtim.tv_sec =
    dest->st_ctim.tv_sec = ct;

  return 0;
}

