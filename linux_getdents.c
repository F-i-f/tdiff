#include <unistd.h>
#include <syscall.h>
#include <dirent.h>

struct linux_dent {
  unsigned long         d_ino;
  unsigned long         d_off;
  unsigned short	d_reclen;
  char			d_name[NAME_MAX+1];
};

int
getdents(int fd, char* buf, unsigned count)
{
  return syscall(SYS_getdents, fd, buf, count);
}
