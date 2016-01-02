#ifndef __IF_UTILS_H__
#define  __IF_UTILS_H__

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <fuse.h>
#include <glib.h>


extern struct fuse_operations isofuse_ops;


#define IS_DIRECTORY(stats) ((stats)->type == _STAT_DIR)

/*
 * data structures
 */

typedef struct if_dir_s {
  gchar * path;
  iso9660_stat_t * info;
} if_dir;

/**
 * Used to store the status of this fuse instance.
 */
typedef struct isofuse_status_s {
  gchar     * path;
  // all files will be appear as belonging to the user belo
  uid_t owner_uid;
  gid_t owner_gid;
  mode_t default_file_mode;
  mode_t default_dir_mode;
  iso9660_t * fh;
} if_status;

if_status * if_status_new(const char * path);
void if_status_destroy(if_status * status);
if_status * get_status();


/**
 * Extract data 
 */
int translate_stat(iso9660_stat_t * src,struct stat * dest);
int translate_fsstat(iso9660_pvd_t * src,struct statvfs * fsstat);

#endif /*  __IF_UTILS_H__ */
