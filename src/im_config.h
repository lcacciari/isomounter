/* im_config.h - configuration 
 * 
 * Copyright (C) 2016 Leo Cacciari <leo.cacciari@gmail.com>
 *
 * This file belongs to the isomounter project.
 * isomounter is free software and is distributed under the terms of the 
 * GNU GPL. See the file COPYING for details.
 */
#ifndef __IM_CONFIG_H__
#define __IM_CONFIG_H__
#include "common.h"

typedef struct im_config_s {
  gboolean debug;
  gboolean foreground;
  gboolean single_thread;
  gchar ** options;
  gboolean manage;
  gboolean dry_run;
  gchar  * base_dir;
  gchar  * image_path;
  gchar  * mountpoint;
} im_config_t;

const im_config_t * im_get_config();
void im_config_print();
gboolean im_init_config(GError **error);
gboolean process_options(gint * p_argc,gchar *** p_argv, GError ** error);

/**
 * Returns a freshly allocated NULL-terminated array of strings.
 * On error, it returns NULL and set error accordingly
 */
gchar * * im_config_extract_fuse_args(const gchar * argv0,GError ** error);


gboolean check_mountpoint(GError ** error);
gboolean check_image_file(GError ** error);

#endif /*__IM_CONFIG_H__*/
