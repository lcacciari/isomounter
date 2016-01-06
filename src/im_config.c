#include "common.h"
#include "im_config.h"
#include <glib/gstdio.h>

static im_config_t * _config = NULL;

gboolean im_init_config(GError ** error) {
  _config = g_try_new0(im_config_t,1);
  if (_config != NULL) {
    _config->base_dir = g_build_filename(g_get_home_dir(),DEFAULT_MOUNTPOINT,NULL);
  }
  return (_config != NULL);
}

const im_config_t * im_get_config() {
  return _config;
}

void im_config_print() {
  gchar * options = g_strjoinv(",",_config->options);
  g_print("dry_run: %s\n",_config->dry_run ? "yes" : "no");
  g_print("debug: %s\n",_config->debug ? "yes" : "no");
  g_print("foreground: %s\n",_config->foreground ? "yes" : "no");
  g_print("single thread: %s\n",_config->single_thread ? "yes" : "no");
  g_print("fuse mount options: %s\n",options);
  g_print("manage mount point: %s\n",_config->manage ? "yes" : "no");
  g_print("base dir is %s\n",_config->base_dir);
  g_print("image path %s\n",_config->image_path);
  g_print("mountpoint %s\n",_config->mountpoint);
  g_free(options);
}

G_GNUC_NORETURN
gboolean parse_version_option(const gchar * option,
			      const gchar * value,
			      gpointer data,
			      GError **error) {
  g_print("%s\n",PACKAGE_STRING);
  exit(0);
}


gboolean parse_arguments(const gchar * option,
			 const gchar * value,
			 gpointer data,
			 GError **error) {
  static gint idx = 0;
  gchar * path = NULL;
  if (g_path_is_absolute(value)) {
    path = g_build_filename(value,NULL);
  } else {
    path = g_build_filename(g_get_current_dir(),value,NULL);
  }
  gchar ** target = NULL;
  switch(idx) {
  case 0:
    target = &(_config->image_path);
    break;
  case 1:
    target = &(_config->mountpoint);
    break;
  default:
    g_set_error(error,G_OPTION_ERROR,G_OPTION_ERROR_FAILED,"too many arguments");
    return FALSE;
  }
  // for now there is no default, so it's guaranteed to be NULL
  *target = path;
  idx += 1;
  return TRUE;
}


/*
 * Split mops on comas, ensuring that each element of
 * get_config()->options[] contains just one option
 * This will make easier to check if some incompatible
 * option is missing, or if a needed one is missing
 */
gboolean setup_fuse_options(gchar ** mops,GError ** error) {
  if (mops == NULL) {
    _config->options = g_malloc0(sizeof(gchar *));
    return TRUE;
  }
  gchar * joined = g_strjoinv(",",mops);
  _config->options = g_strsplit(joined,",",-1);
  g_free(joined);
  return TRUE;
}

#define FIELD_ADDRESS(c,f) (&((c)->f))
gboolean process_options(gint * p_argc,gchar *** p_argv, GError ** error) {
  gchar ** mops = NULL;
  GOptionEntry entries[] = {
    {"base-dir",0,G_OPTION_FLAG_NONE,G_OPTION_ARG_FILENAME,FIELD_ADDRESS(_config,base_dir),"set the directory under which dynamic mountpoints are created","dir"},
    {"debug",'d',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(_config,debug),"do not demonize and print debug messages",NULL},
    {"dry-run",'n',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(_config,dry_run),"just print out what the program would do and exit",NULL},
    {"foreground",'f',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(_config,foreground),"do not demonize",NULL},
    {"manage",'m',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(_config,manage),"if the mountpoit doesn't exist create it and remove at exit",NULL},
    {"options",'o',G_OPTION_FLAG_NONE,G_OPTION_ARG_STRING_ARRAY,&mops,"mount(1) options, included fuse-related ones","mode"},
    {"single-thread",'s',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(_config,single_thread),"use single thread imlementation"},
    {"version",0,G_OPTION_FLAG_NO_ARG,G_OPTION_ARG_CALLBACK,parse_version_option,"prints the version information and exit",NULL},
    {"",0,G_OPTION_FLAG_FILENAME,G_OPTION_ARG_CALLBACK,parse_arguments,"???","!!!"},
    {NULL}
  };
  
  GOptionContext * ctx = g_option_context_new("path-to-image [mountpoint]");
  g_option_context_add_main_entries (ctx,entries,NULL);
  gboolean result = g_option_context_parse(ctx, p_argc, p_argv, error);
  if (result) result = setup_fuse_options(mops,error); 
  g_option_context_free(ctx);
  g_free(mops);
  return result;
}

gboolean check_mountpoint(GError ** error) {
  gboolean result = TRUE;
  if (_config->mountpoint == NULL) {
    // set to the defaul value before going on
    gchar * name = g_path_get_basename(_config->image_path);
    _config->mountpoint = g_build_filename(_config->base_dir,name,NULL);
    g_free(name);
  } 
  const gchar * path = _config->mountpoint;
  if (g_file_test(path,G_FILE_TEST_IS_DIR) && (g_access(path,W_OK) == 0)) {
    if (_config->manage) {
      // do we truly want an error?
      result = FALSE;
      g_set_error(error,IM_ERROR_DOMAIN,IM_ERROR_MOUNTPOINT_EXISTS,"unable to manage an existing mountpoint");
    }
  } else if (! _config->manage) {
    result = FALSE;
    g_set_error(error,IM_ERROR_DOMAIN,IM_ERROR_MOUNTPOINT_ACCESS,"mountpoint is not accessible");
  } else if (_config->dry_run) {
    g_print("will create mountpoint %s\n",path);
  } else {
    gint rc = g_mkdir(path,0777);
    if (rc != 0) {
      result = FALSE;
      g_set_error(error,IM_ERROR_DOMAIN,IM_ERROR_MOUNTPOINT_ACCESS,"failed to create managed mountpoint");
    }
  }
  return result;
}

// check only that exists and is writable. should check is a iso file?
gboolean check_image_file(GError ** error) {
  const gchar * path = _config->image_path; 
  gboolean result = TRUE;
  if (path == NULL) {
    result = FALSE;
    g_set_error(error,IM_ERROR_DOMAIN,IM_ERROR_IMAGE,"no image specified");
  } else if (! g_file_test(path,G_FILE_TEST_IS_REGULAR) || (g_access(path,R_OK))) {
    result = FALSE;
    g_set_error(error,IM_ERROR_DOMAIN,IM_ERROR_IMAGE,"image file isn't accessible");
  }
  return result;
}

gchar ** im_config_extract_fuse_args(const gchar * argv0,GError ** error) {
  GSList *arg_list = NULL;
  gint n = 1;
  arg_list = g_slist_prepend(arg_list,g_strdup(argv0));
  if (_config->debug) {
    arg_list = g_slist_prepend(arg_list,g_strdup("-d"));
    n++;
  }
  if (_config->foreground) {
    arg_list = g_slist_prepend(arg_list,g_strdup("-f"));
    n++;
  }
  if (_config->single_thread) {
    arg_list = g_slist_prepend(arg_list,g_strdup("-s"));
    n++;
  }
  for (gint idx = 0; _config->options[idx] != NULL; idx++) {
    arg_list = g_slist_prepend(arg_list,g_strconcat("-o",_config->options[idx],NULL));
    n++;
  }
  arg_list = g_slist_prepend(arg_list,g_strdup(_config->mountpoint));
  n++;

  gchar ** opts = g_new(gchar *,n + 1);
  opts[n--] = NULL;
  for (GSList * slist = arg_list;slist;slist = slist->next) {
    opts[n--] = slist->data;
  }
  g_slist_free(arg_list);
  return opts;
}

    

