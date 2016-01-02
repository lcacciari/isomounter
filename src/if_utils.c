#include "if_utils.h"
#include <time.h>

#define DEFAULT_FILE_PERMISSIONS S_IRUSR | S_IRGRP | S_IROTH
#define DEFAULT_DIR_PERMISSIONS DEFAULT_FILE_PERMISSIONS | S_IXUSR | S_IXGRP | S_IXOTH 

if_status * if_status_new(const char * path) {
  if_status * status = g_malloc0(sizeof(if_status));
  if (status != NULL) {
    status->path = g_strdup(path);
    status->owner_uid = getuid();
    status->owner_gid = getgid();
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
  if_status * status = get_status();
  // dest->st_dev ignored
  // dest->st_ino ignored. Can be useful
  dest->st_mode = IS_DIRECTORY(src) ?
    status->default_dir_mode : status->default_file_mode;
  dest->st_nlink = 1; // ???
  dest->st_uid = status->owner_uid;
  dest->st_gid = status->owner_gid;
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

int translate_fsstat(iso9660_pvd_t * src,struct statvfs * fsstat) {
  if_status * status = get_status();
  
          /* struct statfs { */
          /*      __SWORD_TYPE f_type;    /\* type of filesystem (see below) *\/ */
          /*      __SWORD_TYPE f_bsize;   /\* optimal transfer block size *\/ */
          /*      fsblkcnt_t   f_blocks;  /\* total data blocks in filesystem *\/ */
          /*      fsblkcnt_t   f_bfree;   /\* free blocks in fs *\/ */
          /*      fsblkcnt_t   f_bavail;  /\* free blocks available to */
          /*                                 unprivileged user *\/ */
          /*      fsfilcnt_t   f_files;   /\* total file nodes in filesystem *\/ */
          /*      fsfilcnt_t   f_ffree;   /\* free file nodes in fs *\/ */
          /*      fsid_t       f_fsid;    /\* filesystem id *\/ */
          /*      __SWORD_TYPE f_namelen; /\* maximum length of filenames *\/ */
          /*      __SWORD_TYPE f_frsize;  /\* fragment size (since Linux 2.6) *\/ */
          /*      __SWORD_TYPE f_spare[5]; */
          /*  }; */
 
}
