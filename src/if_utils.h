/* if_utils.h - utilities
 * 
 * Copyright (C) 2016 Leo Cacciari <leo.cacciari@gmail.com>
 *
 * This file belongs to the isomounter project.
 * isomounter is free software and is distributed under the terms of the 
 * GNU GPL. See the file COPYING for details.
 */
#ifndef __IF_UTILS_H__
#define  __IF_UTILS_H__

#include "common.h"
#include "im_config.h"
#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <fuse.h>

#define IS_DIRECTORY(stats) ((stats)->type == _STAT_DIR)

extern struct fuse_operations isofuse_ops;

typedef struct if_dir_s {
  gchar * path;
  iso9660_stat_t * info;
} if_dir;

/**
 * Used to store the status of this fuse instance.
 */
typedef struct isofuse_status_s {
  enum {
    AT_START = 0,
    AFTER_MOUNT,
    AFTER_UMOUNT,
    IN_ERROR
  } phase;
  gchar     * path;
  gboolean  mountpoint_managed;
  // all files will be appear as belonging to the user below
  uid_t owner_uid;
  gid_t owner_gid;
  mode_t default_file_mode;
  mode_t default_dir_mode;
  iso9660_t * fh;
} if_status;

if_status * if_status_new();
void if_status_destroy(if_status * status);
if_status * get_status();


/**
 * Extract data 
 */
int translate_stat(iso9660_stat_t * src,struct stat * dest);


#endif /*  __IF_UTILS_H__ */
