#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>    // For open() and file access flags (O_RDONLY, etc.)
#include <unistd.h>   // For close(), read(), and write() functions

int main(int argc, char *argv[]) {
    if (argc > 2) {
        return EINVAL;
    }

    if (argc == 2) {
        close(0); // close file descriptor 0
        int fd = open(argv[1], O_RDONLY); // O_RDONLY flag for read only
        if (fd == -1) {
            int err = errno;
            perror("open");
            return err;
        }
    }
    // the above use set the file descriptor 0 to the specified file can also
    // be done without the above by running compileProgram < fileToRead

    // implementation of the cat function in unix, return what is entered
    char buffer[4069]; // 2 to the power of 12
    ssize_t bytes_read;       // read(0, ...), 0 is standard input for file descriptor 0 i.e. convention
    while ((bytes_read = read(0, buffer, sizeof(buffer))) > 0) {
        ssize_t bytes_written = write(1, buffer, bytes_read);

        if (bytes_written == -1) {
            int err = errno;
            perror("write");
            return err;
        }
        assert(bytes_read == bytes_written);
    }
    
    if (bytes_read == -1) {
        int err = errno;
        perror("read");
        return err;
    }
    assert(bytes_read==0); // if no error, we read to the end of the data, i.e. 0 bytes
    return 0;

}