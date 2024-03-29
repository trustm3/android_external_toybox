/* getprop.c - Get an Android system property
 *
 * Copyright 2015 The Android Open Source Project

USE_GETPROP(NEWTOY(getprop, ">2Z", TOYFLAG_USR|TOYFLAG_SBIN))

config GETPROP
  bool "getprop"
  default y
  depends on TOYBOX_ON_ANDROID
  help
    usage: getprop [NAME [DEFAULT]]

    Gets an Android system property, or lists them all.
*/

#define FOR_getprop
#include "toys.h"

#if defined(__ANDROID__)

#include <cutils/properties.h>

#include <selinux/android.h>
#include <selinux/label.h>
#include <selinux/selinux.h>

GLOBALS(
  size_t size;
  char **nv; // name/value pairs: even=name, odd=value
  struct selabel_handle *handle;
)

static char *get_property_context(char *property)
{
  char *context = NULL;

  if (selabel_lookup(TT.handle, &context, property, 1)) {
    perror_exit("unable to lookup label for \"%s\"", property);
  }
  return context;
}

static void add_property(char *name, char *value, void *unused)
{
  if (!(TT.size&31)) TT.nv = xrealloc(TT.nv, (TT.size+32)*2*sizeof(char *));

  TT.nv[2*TT.size] = xstrdup(name);
  if (toys.optflags & FLAG_Z) {
    TT.nv[1+2*TT.size++] = get_property_context(name);
  } else {
    TT.nv[1+2*TT.size++] = xstrdup(value);
  }
}

static int toybox_selinux_log_callback(int type, const char *fmt, ...) {
  va_list ap;

  if (type == SELINUX_INFO) return 0;
  va_start(ap, fmt);
  verror_msg(fmt, 0, ap);
  va_end(ap);
  return 0;
}

void getprop_main(void)
{
  if (toys.optflags & FLAG_Z) {
    union selinux_callback cb;

    cb.func_log = toybox_selinux_log_callback;
    selinux_set_callback(SELINUX_CB_LOG, cb);
    TT.handle = selinux_android_prop_context_handle();
    if (!TT.handle) error_exit("unable to get selinux property context handle");
  }

  if (*toys.optargs) {
    if (toys.optflags & FLAG_Z) {
      char *context = get_property_context(*toys.optargs);

      puts(context);
      if (CFG_TOYBOX_FREE) free(context);
    } else {
      property_get(*toys.optargs, toybuf, toys.optargs[1] ? toys.optargs[1] : "");
      puts(toybuf);
    }
  } else {
    size_t i;

    if (property_list((void *)add_property, 0)) error_exit("property_list");
    qsort(TT.nv, TT.size, 2*sizeof(char *), qstrcmp);
    for (i = 0; i<TT.size; i++) printf("[%s]: [%s]\n", TT.nv[i*2],TT.nv[1+i*2]);
    if (CFG_TOYBOX_FREE) {
      for (i = 0; i<TT.size; i++) {
        free(TT.nv[i*2]);
        free(TT.nv[1+i*2]);
      }
      free(TT.nv);
    }
  }
  if (CFG_TOYBOX_FREE && (toys.optflags & FLAG_Z)) selabel_close(TT.handle);
}

#else

void getprop_main(void)
{
}

#endif
