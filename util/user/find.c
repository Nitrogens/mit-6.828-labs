#include <stddef.h>

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char *path, char *pattern)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if (path == NULL || pattern == NULL) {
        return;
    }

    // open file
    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // get the metadata of the file
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // if the current path does not directs to a directory,
    // return immediately
    if (st.type != T_DIR) {
        close(fd);
        return;
    }

    // check if the total length of the path
    // (including the file name and the '\0' character)
    // exceeds the length of the buffer
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        printf("find: path too long\n");
        close(fd);
        return;
    }

    // copy the path to the buffer
    strcpy(buf, path);
    // p helps to append the name of the file to the path
    p = buf + strlen(buf);
    *p++ = '/';

    // retrieve all the files/directories of the current directory
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        // skip the file which not exists
        if (de.inum == 0) {
            continue;
        }
        // append the filename after the path name
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = '\0';

        // skip the '.' directory and the '..' directory
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
            continue;
        }

        // check if the name of the file
        // matches the given pattern
        if (strcmp(de.name, pattern) == 0) {
            printf("%s\n", buf);
        }

        // find files recursively
        find(buf, pattern);
    }
    close(fd);
}

int
main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(2, "usage: find path file_name\n");
        exit();
    }

    find(argv[1], argv[2]);

    exit();
}
