#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>


#define PATH_MAX 256
#define BUFFER_SIZE 128


/* This function returns a char pointer to a string representation of the integer given */
char* int_to_str(int num) {
    int i = 0;
    int num_digits_signed = 12;

    char* int_str = (char*) malloc(num_digits_signed);

    if (num < 0) {
        int_str[i++] = '-';
        num = -num;
    }

    int div = 1;

    while (num / div >= 10) {
        div *= 10;
    }

    while (div > 0) {
        int digit = num / div;
        int_str[i++] = '0' + digit;
        num %= div;
        div /= 10;
    }

    int_str[i] = '\0'; //Add null terminator at the end for safe measure

    return int_str;
}

/* Returns a char pointer to the path for the specific fd */
char* construct_fd_path(int fd) {
    char path[] = "/proc/self/fd/";
    // Convert integers to strings
    char* str_fd = int_to_str(fd);

    char* fd_path = (char*) malloc(strlen(path) + strlen(str_fd) + 1);
    fd_path[0] = '\0';

    // Construct the file path
    strcat(fd_path, path);
    strcat(fd_path, str_fd);

    free(str_fd); // free pointer

    return fd_path;
}

/* If an fd is found for the file which is already opened, then simply set fd to that, otherwise return -1 */
int get_opened_fd(char* file_path, int* fd) {
    struct stat file_stat;

    if (stat(file_path, &file_stat) < 0) {
        return -1; // file doesn't exist or can't be accessed
    }

    int dir_fd = open("/proc/self/fd", O_RDONLY | __O_DIRECTORY);

    if (dir_fd < 0) {
        return -1; // can't open /proc/self/fd directory
    }

    for (int i = 0; i < dir_fd; i++) {

        char * fd_path = construct_fd_path(i);

        struct stat fd_stat;

        if (stat(fd_path, &fd_stat) < 0) {
            free(fd_path);

            continue; // can't access this fd
        }

        if (fd_stat.st_ino == file_stat.st_ino) { // check if the inodes are the same for both files
            int fd_candidate = openat(dir_fd, fd_path, O_RDONLY);

            if (fd_candidate < 0) {
                free(fd_path);

                continue; // can't open this fd
            }

            if (fcntl(fd_candidate, F_GETFD) < 0) {
                close(fd_candidate);
                free(fd_path);

                continue; // this fd is closed
            }

            *fd = i;

            close(dir_fd);
            close(fd_candidate);

            free(fd_path);

            return 0; // file is opened with this fd
        }

        free(fd_path);
    }

    close(dir_fd);

    return -1; // file is not opened
}

/* This adds a newline char to the end of the current string in a newly allocated buffer */
char* add_newline(char *str) {
    size_t len = strlen(str);
    size_t new_len = len + 2;

    char *new_str = (char*) malloc(new_len);
    strcpy(new_str, str);

    new_str[new_len-2] = '\n';
    new_str[new_len-1] = '\0';

    return new_str;
}


int fprintfx(char* filename, char format, void* data) {
    if (data == NULL) { // error condition for data is checked here
        errno = EIO;
        return -1;
    }

    int fd = 0;

    if (filename == NULL || strcmp(filename, "") == 0) {
        fd = STDOUT_FILENO;
    }
    else if (get_opened_fd(filename, &fd) == -1) { // if there is no fd under the current filename, create a new one
        fd = open(filename, O_WRONLY | O_APPEND | O_CREAT);

        if (fd == -1 || chmod(filename, 0640) == -1) {
            errno = EIO;
            return -1;
        }
    }

    int bytes_written = 0;

    /* Format is checked and typecasts are made appropriately */
    if (format == 's') {
        char* data_str = (char*) data;
        char* new_str = add_newline(data_str);

        bytes_written = write(fd, new_str, strlen(new_str));

        free(new_str);
    }
    else if (format == 'd') {
        int data_int = *((int*) data);
        char * int_str = int_to_str(data_int);
        char * new_int_str = add_newline(int_str);

        bytes_written = write(fd, new_int_str, strlen(new_int_str));

        free(int_str);
        free(new_int_str);
    }
    else { // otherwise error if format isnt recognized
        errno = EIO;
        return -1;
    }

    // check for errors with writing the bytes
    if (bytes_written == -1) {
        errno = EIO;
        return -1;
    }

    return 0;
}


int fscanfx(char* filename, char format, void* dst) {
    int fd;

    char* buffer;
    size_t buffer_size;

    if (dst == NULL) { // silently return if dst is a null pointer
        return 0;
    }

    if (filename == NULL || strcmp(filename, "") == 0) {
        fd = STDIN_FILENO;
    }
    else if (get_opened_fd(filename, &fd) == -1) { // if fd exists for filename, continue, otherwise make a new fd
        fd = open(filename, O_RDONLY);

        if (fd == -1) {
            if (errno != ENOENT) {
                errno = EIO;
            }

            return -1;
        }
    }

    buffer_size = BUFFER_SIZE; // allocate appriopriate space for buffer
    buffer = (char*) calloc(buffer_size+1, sizeof(char));
    buffer[buffer_size] = '\0';

    if (buffer == NULL) {
        errno = EIO;
        return -1;
    }


    char* line = NULL;

    ssize_t nread, total_read = 0;

    while ((nread = read(fd, buffer + total_read, buffer_size - total_read)) > 0) {
        total_read += nread;
        
        char* newline_chr = strchr(buffer, 10) ? strchr(buffer, 10) : strchr(buffer, 13); // look for newline character in read buffer

        if (newline_chr != NULL) { // if it exists then copy the line to line var and exit loop
            size_t line_len = newline_chr - buffer;
            line = (char*) malloc(line_len + 1);

            if (line == NULL) {
                errno = EIO;
                return -1;
            }

            strncpy(line, buffer, line_len);
            line[line_len] = '\0';

            break;
        }

        if (total_read == buffer_size) { // realloc buffer if total read is an increment of 128 bytes
            buffer_size += BUFFER_SIZE;
            buffer = (char*) realloc(buffer, buffer_size+1);
            buffer[buffer_size] = '\0';

            if (buffer == NULL) {
                errno = EIO;
                return -1;
            }
        }
    }


    if (nread < 0) { // check for error in reading bytes
        free(buffer);
        free(line);

        errno = EIO;

        return -1;
    }
    else if (nread == 0 && total_read == 0) { // check for EOF
        free(buffer);
        free(line);

        return -1;
    }

    char* result = NULL;
    size_t result_size = 0;

    if (line != NULL) { // if line exists, then set it to result then lseek back in the file to the position after the line
        struct stat file_stat;

        size_t line_size = strlen(line);

        if (fstat(fd, &file_stat) == -1) return -1;

        if (S_ISREG(file_stat.st_mode)) { //lseek only if its a regular file
            off_t new_offset = lseek(fd, line_size-total_read+1, SEEK_CUR);

            if (new_offset == -1) {
                errno = EIO;
                return -1;
            }
        }

        result = line;
        result_size = line_size;
    }
    else if (buffer != NULL) { // if line is null and buffer isnt, copy result to buffer
        result = buffer;
        result_size = buffer_size;
    }

    /* Check format and cast to appropriate types */
    if (format == 's') {
        memcpy(dst, result, result_size);

        *((char*)dst + result_size) = '\0';
    }
    else if (format == 'd') {
        *(int *) dst = atoi(result);
    }
    else { // error out if format is unrecognized
        free(buffer);
        free(line);

        errno = EIO;

        return -1;
    }

    free(line); // free malloced blocks
    free(buffer);

    return 0;
}


int clean() {
    int fd, i = 0;
    int dir_fd = open("/proc/self/fd", O_RDONLY | __O_DIRECTORY);

    // can't open /proc/self/fd directory
    if (dir_fd < 0) {
        errno = EIO;
        return -1;
    }

    // iterate over all file descriptors
    for (fd = 3; fd < dir_fd; fd++) {
        // check if the file descriptor is open
        if (fcntl(fd, F_GETFD) == -1) continue;

        // check if it is not stdin, stdout, or stderr
        if (fd == STDIN_FILENO || fd == STDOUT_FILENO || fd == STDERR_FILENO) continue;

        // close the file descriptor and error appropriately
        if (close(fd) == -1) {
            errno = EIO;
            return -1;
        }
    }

    close(dir_fd); // close initial dir

    return 0;
}
