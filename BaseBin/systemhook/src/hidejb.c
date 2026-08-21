//
//  hidejb.c
//  systemhook — hide jailbreak paths from processes marked in HideJailbreakApps
//

#include "hidejb.h"
#include "common/common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/attr.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <litehook.h>
#include <libjailbreak/jbroot.h>

extern char *JB_RootPath;
extern char gExecutablePath[PATH_MAX];

static bool gHideEnabled = false;

bool hidejb_is_enabled(void)
{
	return gHideEnabled;
}

static bool path_is_jb_related(const char *path)
{
	if (!path || path[0] == '\0') return false;

	if (!strcmp(path, "/var/jb") || !strcmp(path, "/private/var/jb")) {
		return true;
	}
	if (string_has_prefix(path, "/var/jb/") || string_has_prefix(path, "/private/var/jb/")) {
		return true;
	}

	if (JB_RootPath && JB_RootPath[0] != '\0') {
		size_t rootLen = strlen(JB_RootPath);
		if (!strcmp(path, JB_RootPath)) {
			return true;
		}
		if (string_has_prefix(path, JB_RootPath) && path[rootLen] == '/') {
			return true;
		}
	}

	if (!strcmp(path, "/usr/lib/systemhook.dylib") ||
		string_has_suffix(path, "/systemhook.dylib")) {
		return true;
	}

	return false;
}

static int access_hook(const char *path, int mode)
{
	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}
	return access(path, mode);
}

static int faccessat_hook(int fd, const char *path, int mode, int flag)
{
	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}
	return faccessat(fd, path, mode, flag);
}

static int stat_hook(const char *path, struct stat *buf)
{
	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}
	return stat(path, buf);
}

static int lstat_hook(const char *path, struct stat *buf)
{
	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}
	return lstat(path, buf);
}

static int fstatat_hook(int fd, const char *path, struct stat *buf, int flag)
{
	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}
	return fstatat(fd, path, buf, flag);
}

static int open_hook(const char *path, int flags, ...)
{
	mode_t mode = 0;
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);
	}

	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}

	if (flags & O_CREAT) {
		return open(path, flags, mode);
	}
	return open(path, flags);
}

static int openat_hook(int fd, const char *path, int flags, ...)
{
	mode_t mode = 0;
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		mode = (mode_t)va_arg(ap, int);
		va_end(ap);
	}

	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}

	if (flags & O_CREAT) {
		return openat(fd, path, flags, mode);
	}
	return openat(fd, path, flags);
}

static int getattrlist_hook(const char *path, void *attrList, void *attrBuf, size_t attrBufSize, unsigned long options)
{
	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}
	return getattrlist(path, attrList, attrBuf, attrBufSize, options);
}

static ssize_t readlink_hook(const char *path, char *buf, size_t bufsize)
{
	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return -1;
	}
	return readlink(path, buf, bufsize);
}

static char *realpath_hook(const char *path, char *resolved)
{
	if (path_is_jb_related(path)) {
		errno = ENOENT;
		return NULL;
	}
	return realpath(path, resolved);
}

void hidejb_apply_hooks(void)
{
	const char *hideEnv = getenv("JB_HIDE");
	bool hide = (hideEnv && !strcmp(hideEnv, "1"));

	if (!hide && gExecutablePath[0] != '\0') {
		hide = should_hide_jailbreak_for_executable(gExecutablePath);
	}

	if (!hide) return;

	gHideEnabled = true;

	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)access, (void *)access_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)faccessat, (void *)faccessat_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)stat, (void *)stat_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)lstat, (void *)lstat_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)fstatat, (void *)fstatat_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)open, (void *)open_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)openat, (void *)openat_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)getattrlist, (void *)getattrlist_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)readlink, (void *)readlink_hook, NULL);
	litehook_rebind_symbol(LITEHOOK_REBIND_GLOBAL, (void *)realpath, (void *)realpath_hook, NULL);

	unsetenv("JB_HIDE");
}
