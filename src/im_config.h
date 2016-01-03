#ifndef __IM_CONFIG_H__
#define __IM_CONFIG_H__
/*
 * Routines for configuration of isomounter
 */

#include <glib.h>
typedef struct im_config_s {
  gboolean debug;
  gboolean foreground;
  gboolean single_thread;
  gchar * mount_opts;
  gboolean manage_mp;
  gchar * base_dir;
  gchar * image_path;
  gchar * mountpoint;
} im_config_t;




/*
 * Parsing functions for -o and args.
 * Both assumes that a im_config_ pointer has been passed
 * at group creation
 */


gboolean parse_mount_options(const gchar * option,const gchar * value,gpointer data,GError **error);

gboolean parse_arguments(const gchar * option,const gchar * value,gpointer data,GError **error);

im_config_t * im_config_new();

void im_config_print(im_config_t * config);

/* option groups construction*/
GOptionGroup * build_fuse_options(im_config_t * config);

GOptionGroup * build_main_options(im_config_t * config);

void im_config_extract_fuse_args(im_config_t * config,const gchar * argv0,
				 gint * p_argc,gchar *** p_argv);

#endif /*__IM_CONFIG_H__*/
