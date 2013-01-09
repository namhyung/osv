#define __FILE_defined
class FILE;

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <algorithm>
#include <dirent.h>
#include "libc.hh"
#include "fs/fs.hh"

class FILE {
public:
    explicit FILE(int fd);
    ~FILE();
private:
    int _fd;
};

class __dirstream {
public:
    explicit __dirstream(fileref file);
    fileref dir();
    int idx = 0;
private:
    fileref _file;
};

FILE::FILE(int fd)
    : _fd(fd)
{
}

__dirstream::__dirstream(fileref file)
    : _file(file)
{
}

fileref __dirstream::dir()
{
	return _file;
}

FILE* fopen(const char* fname, const char* fmode)
{
    static struct conv {
        const char* fmode;
        int mode;
    } modes[] = {
        { "r", O_RDONLY },
        { "r+", O_RDWR },
        { "w", O_WRONLY | O_CREAT | O_TRUNC },
        { "w+", O_RDWR | O_CREAT | O_TRUNC },
        { "a", O_WRONLY | O_APPEND | O_CREAT | O_TRUNC },
        { "a+", O_RDWR | O_APPEND | O_CREAT | O_TRUNC },
    };
    auto p = std::find_if(modes, modes + 6,
                 [=](conv c) { return strcmp(fmode, c.fmode) == 0; } );
    if (p == modes + 6) {
        return nullptr;
    }
    auto fd = ::open(fname, p->mode);
    if (fd == -1) {
        return nullptr;
    }
    return new FILE(fd);
}

DIR* opendir(const char* fname)
{
    auto f = rootfs->open(fname);
    if (!f) {
        return libc_error_ptr<DIR>(ENOENT);
    }
    return new DIR(f);
}

int closedir(DIR* dir)
{
//	rootfs->close(dir->_file);   once we implement close
	delete dir;
	return 0;
}

struct dirent *readdir(DIR* dir)
{
	static struct dirent entry, *result;	// XXX: tls?
	int ret;

	ret = readdir_r(dir, &entry, &result);
	if (ret)
		return libc_error_ptr<struct dirent>(ret);

	errno = 0;
	return result;
}

int readdir_r(DIR* dir, struct dirent* entry, struct dirent** result)
{
	int ret;

	ret = dir->dir()->getdent(entry, dir->idx++);
	if (ret == 0)
		*result = entry;
	else
		*result = NULL;
	return 0;
}
