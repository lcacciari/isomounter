#include "if_utils.h"
#include <glib/gstdio.h>
typedef struct isomounter_config_s {
  gboolean debug;
  gboolean foreground;
  gboolean single_thread;
  gchar ** mount_opts;
  gboolean manage_mp;
  gchar * base_dir;
  gchar ** arguments;
  // these are set up by check_config
  gchar * image_path;
  gchar * mountpoint;
} isomounter_config_t;

static void print_config(isomounter_config_t * config) {
  g_print("debug: %s\n",config->debug ? "yes" : "no");
  g_print("foreground: %s\n",config->foreground ? "yes" : "no");
  g_print("single thread: %s\n",config->single_thread ? "yes" : "no");
  // missing modes
  g_print("manage mount point: %s\n",config->manage_mp ? "yes" : "no");
  g_print("base dir is %s\n",config->base_dir);
  g_print("image path %s\n",config->image_path);
  g_print("mountpoint %s\n",config->mountpoint);
}

static gboolean is_writable_directory(const gchar * path) {
  GStatBuf st;
  gint rc = g_stat(path,&st);
  if (rc != 0) return false;
  if (!S_ISDIR(st.st_mode)) {
    return false;
  }
  rc = g_access(path,W_OK);
  return (rc == 0);
}

  
#define FIELD_ADDRESS(c,f) (&((c)->f))

static gchar * default_base_dir() {
  const gchar * home = g_get_home_dir();
  return g_build_filename(home,"isomount",NULL);
}

static isomounter_config_t * create_default_config() {
  isomounter_config_t * config =
    (isomounter_config_t *) g_malloc0(sizeof(isomounter_config_t));
  return config;
}

static GOptionGroup * build_fuse_options(isomounter_config_t * config) {
  GOptionGroup * group = g_option_group_new("fuse",
					    "default fuse options",
					    "The default options to fuse library, with added long options for clarity",
					    NULL,NULL);
  GOptionEntry entries[] = {
    {"foreground",'f',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(config,foreground),"do not demonize",NULL},
    {"debug",'d',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(config,debug),"do not demonize and print debug messages",NULL},
    {"single-thread",'s',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(config,single_thread),"use single thread imlementation"},
    {"mount_opts",'o',G_OPTION_FLAG_NONE,G_OPTION_ARG_STRING_ARRAY,FIELD_ADDRESS(config,mount_opts),"mount(1) options, included fuse-related ones","MODE"},
    {NULL}
  };
  g_option_group_add_entries(group,entries);
  return group;
}

static void add_main_entries(GOptionContext * ctx,isomounter_config_t * config) {
  GOptionEntry entries[] = {
    {"manage-mp",'p',G_OPTION_FLAG_NONE,G_OPTION_ARG_NONE,FIELD_ADDRESS(config,manage_mp),"if the mountpoit doesn't exist create it and remove at exit",NULL},
    {"base-dir",0,G_OPTION_FLAG_NONE,G_OPTION_ARG_FILENAME,FIELD_ADDRESS(config,base_dir),"set the directory under which dynamic mountpoints are created","WRITEABLEDIR"},
    {"",0,G_OPTION_FLAG_HIDDEN,G_OPTION_ARG_FILENAME_ARRAY,FIELD_ADDRESS(config,arguments),"???","!!!"},
    {NULL}
  };
  g_option_context_add_main_entries(ctx,entries,NULL);
}

/**
 * Setup context for parsing options into a  isomounter_config_t object.
 */
static GOptionContext * build_context(isomounter_config_t * config) {
  GOptionContext * ctx = g_option_context_new(" - mount a ISO9660 filename in userspace");
  g_option_context_add_group(ctx,build_fuse_options(config));
  add_main_entries(ctx,config);
  return ctx;
}

static gboolean check_config(isomounter_config_t * config) {
  // set base_dir value if not set
  if (config->base_dir == NULL) config->base_dir = default_base_dir();
  // verify that base_dir is the absolute path of a writeable directory
  gchar * cwd = g_get_current_dir(); // cwd will leak if this fails, but we ar goint to exit
  if (! g_path_is_absolute(config->base_dir)) {
    gchar * relative = config->base_dir;
    config->base_dir = g_build_filename(cwd,relative,NULL);
    g_free(relative);
  }
  // check the basedir is writable
  // should check is acually a dir!
  if (!is_writable_directory(config->base_dir)) {
    g_warning("base directory '%s' doesn't exist or isn't writeable",config->base_dir);
    return false;
  }
  // TODO check the mode string
  // the imagefile
  if (config->arguments[0] == NULL) {
    g_warning("no imagefile given");
    return false;
  }
  if (g_path_is_absolute(config->arguments[0])) {
    config->image_path = g_strdup(config->arguments[0]);
  } else {
    config->image_path = g_build_filename(cwd,config->arguments[0],NULL);
  }
  if (config->arguments[1] == NULL) {
    gchar * isoname = g_path_get_basename(config->image_path);
    config->mountpoint = g_build_filename(config->base_dir,isoname,NULL);
    g_free(isoname);
  } else if (g_path_is_absolute(config->arguments[1])) {
    config->mountpoint = g_strdup(config->arguments[1]);
  } else {
    config->mountpoint = g_build_filename(cwd,config->arguments[1],NULL);
  }
  if (config->arguments[1] != NULL && config->arguments[2] != NULL) {
    g_warning("extra arguments will be ignored");
  }
  g_free(cwd);
  return true;
}

/**
 * Builds the arguments to be passed to fuse_main from the given config object.
 * The argv vector is dynamically allocated and is guaranteed to be NULL terminated.
 */
static gboolean prepare_fuse_args(isomounter_config_t * config,const gchar * argv0,
				  gchar *** p_argv,gint * p_argc) {
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
  argv[count++] = g_strdup(config->mountpoint);
  // for now do not handle modes
  *p_argc = count;
  *p_argv = argv;
  return true;
}

static gboolean check_mountpoint(gchar * path,gboolean manage) {
  if (is_writable_directory(path)) {
    return true;
  }
  if (! manage) {
    g_warning("mount point %s does not exist and we weren't asked to create it\n",path);
    return false;
  }
  int rc = g_mkdir(path,0777);
  if (rc != 0) {
    g_warning("creation of mount point %s failed\n",path);
    return false;
  }
  return true;
}

int main(int argc,char **argv) {
  /*
   * The new command line:
   * - preparse mountpoint and image paths (so to handle encoding)
   * - also preparse fuse options, so that we are aware of what
   *   is going on
   * - add default options such as --help
   */
  GError *error = NULL;
  g_message("create config");
  isomounter_config_t * config = create_default_config();
  g_message("build context");
  GOptionContext * context = build_context(config);
  g_message("parse options");
  gboolean ok = g_option_context_parse(context, &argc, &argv, &error);
  g_message("option parsed returned %s", ok ? "true" : "false");
  if (!ok) {
    g_error("option parsing failed: %s\n", error->message);
    return (1);
  }
  g_message("gonna check config");
  ok = check_config(config);
  g_message("check config returned %s", ok ? "true" : "false");
  if (!ok) {
    g_error("bad configuration");
    return(1);
  }
  print_config(config);
  g_message("check mountpoint");
  ok = check_mountpoint(config->mountpoint,config->manage_mp);
  if (!ok) return 1;

  int fuse_argc = 0;
  char ** fuse_argv;
  
  ok = prepare_fuse_args(config,argv[0],&fuse_argv,&fuse_argc);
  if (!ok) {
    g_error("failed to prepare fuse args");
    return(1);
  }
  g_print("prepared %d fuse options\n",fuse_argc);
  

  if_status * status = if_status_new(config->image_path);
  g_message("call fuse_main");
  int result = fuse_main(fuse_argc,fuse_argv,&isofuse_ops,status);
  g_message("fuse main returned %d",result);
  // not usefull: we ar going to release all memory anyway. But dangerous
  // if this part of code is executed twice!
  //if_status_destroy(status);
  return result;
}

