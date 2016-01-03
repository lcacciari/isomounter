#ifndef __MOUNTPOINT_H__
#define __MOUNTPOINT_H__

#include "im_config.h"
#include <glib.h>

typedef enum mountpoint_status_e {
  UNAVAILABLE,
  MANAGED,
  UNMANAGED
} mountpoint_status;

mountpoint_status check_mountpoint(im_config_t * config);
gboolean check_image_file(gchar * path);


#endif /*  __MOUNTPOINT_H__ */
