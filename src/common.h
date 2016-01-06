#ifndef __COMMON_H__
#define __COMMON_H__
#include "config.h"

/* other common definitions and includes */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <glib.h>


#define DEFAULT_MOUNTPOINT "isomount"

/* for using in errors */
typedef enum {
  IM_ERROR_UNKNOWN,
  IM_ERROR_MOUNTPOINT_EXISTS,
  IM_ERROR_MOUNTPOINT_ACCESS,
  IM_ERROR_IMAGE,
} im_error;

GQuark im_error_quark();

#define IM_ERROR_DOMAIN (im_error_quark())



#endif /*__COMMON_H__*/
