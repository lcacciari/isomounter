/* if_utils.c - implementation of utilities
 * 
 * Copyright (C) 2016 Leo Cacciari <leo.cacciari@gmail.com>
 *
 * This file belongs to the isomounter project.
 * isomounter is free software and is distributed under the terms of the 
 * GNU GPL. See the file COPYING for details.
 */
#include "common.h"
#include "if_utils.h"
#include "im_config.h"
#include <time.h>

#define DEFAULT_FILE_PERMISSIONS S_IRUSR | S_IRGRP | S_IROTH
#define DEFAULT_DIR_PERMISSIONS DEFAULT_FILE_PERMISSIONS | S_IXUSR | S_IXGRP | S_IXOTH 

if_status * if_status_new() {
  const im_config_t * config = im_get_config();
  if_status * status = g_malloc0(sizeof(if_status));
  if (status != NULL) {
    status->path = g_strdup(config->image_path);
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
  struct fuse_context * ctx = fuse_get_context();
  if_status * status = (if_status *) ctx->private_data;
  // dest->st_dev ignored
  // dest->st_ino ignored. Can be useful
  dest->st_mode = IS_DIRECTORY(src) ?
    status->default_dir_mode :
    status->default_file_mode;
  dest->st_nlink = 1; // ???
  dest->st_uid = status->owner_uid;
  dest->st_gid = status->owner_gid;
  dest->st_size = src->size; // TODO Check endianity
  time_t ct = mktime(&src->tm);
  // we don't keep track of last access...
  dest->st_atim.tv_sec =
    dest->st_mtim.tv_sec =
    dest->st_ctim.tv_sec = ct;

  return 0;
}

