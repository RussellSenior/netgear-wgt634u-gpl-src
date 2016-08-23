/*
 * Automatically generated C config: don't edit
 */
#if !defined __FEATURES_H && !defined __need_uClibc_config_h
#error Never include <bits/uClibc_config.h> directly; use <features.h> instead.
#endif
#define AUTOCONF_INCLUDED

/*
 * Version Number
 */
#define __UCLIBC_MAJOR__ 0
#define __UCLIBC_MINOR__ 9
#define __UCLIBC_SUBLEVEL__ 19

/*
 * Target Architecture Features and Options
 */
#define __HAVE_ELF__ 1
#define __UCLIBC_HAS_MMU__ 1
#define __UCLIBC_HAS_FLOATS__ 1
#define __HAS_FPU__ 1
#define __DO_C99_MATH__ 1
#define __WARNINGS__ "-Wall"
#define __KERNEL_SOURCE__ "/home/changcs/usb-router-buildroot/build/linux-2.4.x"
#define __C_SYMBOL_PREFIX__ ""
#define __HAVE_DOT_CONFIG__ 1

/*
 * General Library Settings
 */
#define __DOPIC__ 1
#define __HAVE_SHARED__ 1
#undef __ADD_LIBGCC_FUNCTIONS__
#define __BUILD_UCLIBC_LDSO__ 1
#define __LDSO_LDD_SUPPORT__ 1
#define __UCLIBC_CTOR_DTOR__ 1
#define __UCLIBC_HAS_THREADS__ 1
#define __UCLIBC_HAS_LFS__ 1
#undef __MALLOC__
#define __MALLOC_930716__ 1
#define __UCLIBC_DYNAMIC_ATEXIT__ 1
#define __HAS_SHADOW__ 1
#define __UCLIBC_HAS_REGEX__ 1
#undef __UNIX98PTY_ONLY__
#undef __ASSUME_DEVPTS__

/*
 * Networking Support
 */
#undef __UCLIBC_HAS_IPV6__
#undef __UCLIBC_HAS_RPC__

/*
 * String and Stdio Support
 */
#define __UCLIBC_HAS_WCHAR__ 1
#undef __UCLIBC_HAS_LOCALE__
#undef __USE_OLD_VFPRINTF__

/*
 * Library Installation Options
 */
#define __SHARED_LIB_LOADER_PATH__ "/lib"
#define __DEVEL_PREFIX__ "/home/changcs/usb-router-buildroot/build/staging_dir"
#define __SYSTEM_DEVEL_PREFIX__ "/home/changcs/usb-router-buildroot/build/staging_dir"
#define __DEVEL_TOOL_PREFIX__ "/home/changcs/usb-router-buildroot/build/staging_dir/usr"

/*
 * uClibc hacking options
 */
#undef __DODEBUG__
#undef __DOASSERTS__
#undef __SUPPORT_LD_DEBUG__
#undef __SUPPORT_LD_DEBUG_EARLY__
