#ifndef __ISO9660FUSE_H__
#define  __ISO9660FUSE_H__

#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <fuse/fuse.h>
#include <glib.h>


extern fuse_operations isofuse_ops;
/**
 * Used to store the status of this fuse instance.
 */
typedef struct isofuse_status_s {
  gchar     * path;
  iso9660_t * fh;
} if_status;



#endif /*  __ISO9660FUSE_H__ */
