#include <unistd.h>
#include <syscall.h>
#include <dirent.h>

struct linux_dent64 {
  unsigned long long    d_ino;
  unsigned long long    d_off;
  unsigned short	d_reclen;
  unsigned char		d_type;
  char			d_name[NAME_MAX+1];
};

int
getdents64(int fd, char* buf, unsigned count)
{
  return syscall(SYS_getdents64, fd, buf, count);
}

#define getdents(fd, buf, count) getdents64(fd, buf, count)
