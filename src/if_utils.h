#ifndef __IF_UTILS_H__
#define  __IF_UTILS_H__

#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <fuse/fuse.h>

#define IS_DIRECTORY(stats) ((stats)->type == _DIRECTORY)

/*
 * data structures
 */

typedef struct if_dir_s {
  gchar * path;
  iso9660_stat_t * info;
} if_dir;

/**
 * Extract data 
 */
int translate_stat(iso9660_stat_t * src,stat * dest);
int translate_fsstat(iso9660_pvd_t * src,struct statvfs * fsstat);

#endif /*  __IF_UTILS_H__ */
