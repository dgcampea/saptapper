/**
 * Inline file/directory path routines for C.
 */

#ifndef CPATH_H_INCLUDED
#define CPATH_H_INCLUDED

#include <string.h>
#include <stdlib.h>
#include <limits.h>

#ifdef _WIN32
#pragma comment(lib, "shlwapi")
#include <windows.h>
#include <shlwapi.h>
#include <sys/stat.h>
#include <direct.h>
#ifndef PATH_MAX
#define PATH_MAX	_MAX_PATH
#endif
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#endif

#ifndef __cplusplus
#ifdef HAVE_STDBOOL
#include <stdbool.h>
#else
#ifndef bool
typedef int bool;
#define true    1
#define false   0
#endif /* bool */
#endif /* HAVE_STDBOOL */
#endif /* C++ */

#ifndef INLINE
#ifdef inline
#define INLINE  inline
#elsif defined(__inline)
#define INLINE  __inline
#else
#define INLINE
#endif
#endif /* !INLINE */

#ifdef _WIN32
#define PATH_SEPARATOR_CHAR	'\\'
#define PATH_SEPARATOR_STR	"\\"
#else
#define PATH_SEPARATOR_CHAR	'/'
#define PATH_SEPARATOR_STR	"/"
#endif

static INLINE const char *path_findbase(const char *path)
{
#ifdef _WIN32
	return PathFindFileNameA(path);
#else
	char *pslash;

	if (path == NULL)
	{
		return NULL;
	}

	pslash = strrchr(path, PATH_SEPARATOR_CHAR);
	if (pslash != NULL)
	{
		return pslash + 1;
	}
	else
	{
		return path;
	}
#endif
}

static INLINE const char *path_findext(const char *path)
{
#ifdef _WIN32
	return PathFindExtensionA(path);
#else
	char *pdot;
	char *pslash;

	if (path == NULL)
	{
		return NULL;
	}

	pdot = strrchr(path, '.');
	pslash = strrchr(path, PATH_SEPARATOR_CHAR);
	if (pdot != NULL && (pslash == NULL || pdot > pslash))
	{
		return pdot;
	}
	else
	{
		return NULL;
	}
#endif
}

static INLINE void path_basename(char *path)
{
#ifdef _WIN32
	PathStripPathA(path);
#else
	basename(path);
#endif
}

static INLINE void path_dirname(char *path)
{
#ifdef _WIN32
	PathRemoveFileSpecA(path);
#else
	dirname(path);
#endif
}

static INLINE void path_stripext(char *path)
{
#ifdef _WIN32
	PathRemoveExtensionA(path);
#else
	char *pdot = (char*) path_findext(path);
	if (pdot != NULL)
	{
		*pdot = '\0';
	}
#endif
}

static INLINE bool path_isdir(const char *path)
{
	struct stat st;
	if (stat(path, &st) == 0)
	{
		if ((st.st_mode & S_IFDIR) != 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

static char *path_getabspath(const char *path, char *absolute_path)
{
#ifdef _WIN32
	char *szFilePart;
	return GetFullPathNameA(path, PATH_MAX, absolute_path, &szFilePart) != 0 ? absolute_path : NULL;
#else
	// realpath() can resolve path only if the path exists. For example,
	// "/tmp/non-existing-directory-will-fail/../foo.bar" will return wrong result.
	// Therefore, we return just a simple absolute path here. What a pity.
	if (path == NULL || absolute_path == NULL)
	{
		errno = EINVAL;
		return NULL;
	}
	if (path[0] == '/')
	{
		strcpy(absolute_path, path);
	}
	else
	{
		size_t len;
		getcwd(absolute_path, PATH_MAX);
		len = strlen(absolute_path);
		strcpy(&absolute_path[len], PATH_SEPARATOR_STR);
		len += strlen(PATH_SEPARATOR_STR);
		strcat(&absolute_path[len], path);
	}
	return absolute_path;
#endif
}

#endif /* !CPATH_H_INCLUDED */
