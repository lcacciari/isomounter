#include "im_config.h"


int main(int argc,char **argv) {
  GError * error = NULL;
  im_config_t * config = im_config_new();
  g_print("Initial configuration is:\n");
  im_config_print(config);
  GOptionContext * ctx = g_option_context_new("testing configuration");
  g_option_context_add_group(ctx,build_fuse_options(config));
  g_option_context_set_main_group(ctx,build_main_options(config));
  gboolean ok = g_option_context_parse(ctx, &argc, &argv, &error);
  if (ok) {
    g_print("parsing succesfull. Config is now:\n");
    im_config_print(config);
  } else {
    g_print("parsing failed with error %d %s\n",error->code,error->message);
    return 1;
  }
  gint f_argc = 0;
  gchar ** f_argv = NULL;

  ok = im_config_extract_fuse_args(config,argv[0],&f_argc,&f_argv);
  if (ok) {
    gchar * cl = g_strjoinv(" ",f_argv);
    g_print("Arguments that will be passed to fuse_main: '%s'\n",cl);
    g_free(cl);
  } else {
    g_print("building fuse command line failed\n");
  }
	    
}
