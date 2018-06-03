#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>


int main() {
    char *filename = "test.txt";
    char buf[1024];

    int fd = open(filename, O_RDONLY);
    printf("%d\n", (int) read(fd, buf, 1024));
    printf("%d\n", (int) read(fd, buf, 1024));
    printf("%d\n", (int) read(fd, buf, 1024));

    return 0;
}
