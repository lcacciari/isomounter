#include "common.h"
#include "im_config.h"
#include <stdlib.h>

/* usefull macos*/
#define FIELD_ADDRESS(c,f) (&((c)->f))

#define REPLACE_STRING_FIELD(p_config,field,value) do {\
    gchar * old = (p_config)->field;                   \
    (p_config)->field = value;                         \
    g_free(old);                                       \
  } while(0)

#define APPEND_TO_STRING_FIELD(p_config,field,value) do {\
    gchar * old = (p_config)->field;                     \
    if (old == NULL || old[0] == '\0') {                 \
      (p_config)->field = g_strdup(value);               \
    } else {                                             \
      (p_config)->field = g_strjoin(",",(p_config)->field,value,NULL);	\
    }                                                    \
    g_free(old);                                         \
  } while(0)

G_GNUC_NORETURN
gboolean parse_version_option(const gchar * option,
			      const gchar * value,
			      gpointer data,
			      GError **error) {
  g_print("%s\n",PACKAGE_STRING);
  exit(0);
}

gboolean parse_mount_options(const gchar * option,
			     const gchar * value,
			     gpointer data,
			     GError **error) {
  im_config_t * config = (im_config_t *) data;
  APPEND_TO_STRING_FIELD(config,mount_opts,value);
  return TRUE;
}

gboolean parse_arguments(const gchar * option,
			 const gchar * value,
			 gpointer data,
			 GError **error) {
  //not very nice...
  static gint idx = 0;
  gboolean result = TRUE;
  im_config_t * config = (im_config_t *) data;
  gchar * path = NULL;
  if (g_path_is_absolute(value)) {
    path = g_strdup(value);
  } else {
    path = g_build_filename(g_get_current_dir(),value,NULL);
  }
  switch(idx) {
  case 0:
    REPLACE_STRING_FIELD(config,image_path,path);
    break;
  case 1:
    REPLACE_STRING_FIELD(config,mountpoint,path);
    break;
  default:
    g_set_error(error,G_OPTION_ERROR,G_OPTION_ERROR_FAILED,"too many arguments");
    result = FALSE;
  }
  idx += 1;
  return result;
}

gboolean parse_basedir(const gchar * option,
		       const gchar * value,
		       gpointer data,
		       GError **error) {
  
  gboolean result = TRUE;
  im_config_t * config = (im_config_t *) data;
  gchar * path = NULL;
  if (g_path_is_absolute(value)) {
    path = g_strdup(value);
  } else {
    path = g_build_filename(g_get_current_dir(),value,NULL);
  }
  REPLACE_STRING_FIELD(config,base_dir,path);
  return result;
}

im_config_t * im_config_new() {
  im_config_t * config = (im_config_t *) g_malloc0(sizeof(im_config_t));
  config->base_dir = g_build_filename(g_get_home_dir(),"isomount",NULL);
  return config;
}

void im_config_print(im_config_t * config) {
  g_print("debug: %s\n",config->debug ? "yes" : "no");
  g_print("foreground: %s\n",config->foreground ? "yes" : "no");
  g_print("single thread: %s\n",config->single_thread ? "yes" : "no");
  g_print("fuse mount options: %s\n",config->mount_opts);
  g_print("manage mount point: %s\n",config->manage_mp ? "yes" : "no");
  g_print("base dir is %s\n",config->base_dir);
  g_print("image path %s\n",config->image_path);
  g_print("mountpoint %s\n",config->mountpoint);
}


GOptionGroup * build_fuse_options(im_config_t * config) {
  GOptionGroup * group = g_option_group_new("fuse",
					    "default fuse options",
					    "The default options to fuse library, with added long options for clarity",
					    config,NULL);
  GOptionEntry entries[] = {
    {"foreground",'f',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(config,foreground),"do not demonize",NULL},
    {"debug",'d',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(config,debug),"do not demonize and print debug messages",NULL},
    {"single-thread",'s',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(config,single_thread),"use single thread imlementation"},
    {"mount_opts",'o',G_OPTION_FLAG_NONE,G_OPTION_ARG_CALLBACK,parse_mount_options,"mount(1) options, included fuse-related ones","mode"},
    {NULL}
  };
  g_option_group_add_entries(group,entries);
  return group;
}

GOptionGroup * build_main_options(im_config_t * config) {
  GOptionGroup * group = g_option_group_new("main",
					    "main options",
					    "Options of isomounter",config,NULL);
  GOptionEntry entries[] = {
    {"version",0,G_OPTION_FLAG_NO_ARG,G_OPTION_ARG_CALLBACK,parse_version_option,"prints the version information and exit",NULL},
    {"manage",'m',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(config,manage_mp),"if the mountpoit doesn't exist create it and remove at exit",NULL},
    {"base-dir",0,G_OPTION_FLAG_FILENAME,G_OPTION_ARG_CALLBACK,parse_basedir,"set the directory under which dynamic mountpoints are created","WRITEABLEDIR"},
    {"",0,G_OPTION_FLAG_HIDDEN | G_OPTION_FLAG_FILENAME,G_OPTION_ARG_CALLBACK,parse_arguments,NULL,NULL},
    {NULL}
  };
  g_option_group_add_entries(group,entries);
  return group;  
}

void im_config_extract_fuse_args(im_config_t * config,const gchar * argv0,
				       gint * p_argc,gchar *** p_argv) {

  int count = 0;
  gchar ** argv = (gchar **) g_malloc0(sizeof(gchar *) * 9); // <- max number of options is 7
  argv[count++] = g_strdup(argv0);
  if (config->debug) {
    argv[count++] = g_strdup("-d");
  }
  if (config->foreground) {
    argv[count++] = g_strdup("-f");
  }
  if (config->single_thread) {
    argv[count++] = g_strdup("-s");
  }
  if ((config->mount_opts != NULL) && (config->mount_opts[0] != '\0')) {
    argv[count++] = g_strdup("-o");
    argv[count++] = g_strdup(config->mount_opts);
  }
  argv[count++] = g_strdup(config->mountpoint);
  *p_argc = count;
  *p_argv = argv;
}

