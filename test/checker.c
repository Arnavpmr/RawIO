#include "stdiox.h"
#include <time.h>
#include <stdio.h>
#include <errno.h>

#define EXEC(x) \
        if (x == -1) fprintfx("", 's', strerror(errno));

#define FILESHORT "text"
#define FILELONGG "longtext"

int main(int argc, char const *argv[]) {

    printf("\e[1;1H\e[2J");

    /* For generating random string
        Needs to include <time.h>
     */
    srand(time(NULL));

    int array[5] = {1,2,-9,12,-3};
    char* string = "Hello!";

    /*****************************
     * The following is to check
     * your fprintfx() function.
     ***************************** /


    /* Error: NULL destination */
    EXEC(fprintfx("", 'd', NULL))
    EXEC(fprintfx("", 's', NULL))
    /* Error: unrecognized format */
    EXEC(fprintfx("", 'i', string))
    EXEC(fprintfx("", 'X', string))
    EXEC(fprintfx("", 'S', string))
    EXEC(fprintfx("", 'D', string))
    /* Error: both NULL destination AND unrecognized format */
    EXEC(fprintfx("", 'S', NULL))
    EXEC(fprintfx("", 'D', NULL))


    /* Print integers to stdout */
    for (size_t i = 0; i < 5; i++) {
        EXEC(fprintfx("", 'd', &array[i]))
    }

    /* Print string to stdout */
    EXEC(fprintfx("", 's', string))

    /* Write integers to a short text file */
    for (size_t i = 0; i < 5; i ++) {
        EXEC(fprintfx(FILESHORT, 'd', &array[i]))
    }

    /* Generate a long random string
        and write to a file
     */
    long int longsize = 1 << 8;
    char* newfile = (char*)malloc(longsize);
    for (size_t i = 0; i < longsize; i++) {
        newfile[i] = '0' + rand()%72;
        if (newfile[i] == 'a') {
            newfile[i+1] = '\n';
            i ++;
        }
    }
    EXEC(fprintfx(FILELONGG, 's', newfile))
    free(newfile);

    EXEC(clean())


    /************************
     *
     * The following is to
     * check your fscanfx()
     * function.
     *
     * **********************/

    char newstr[1024] = {0};
    int  num;


    /* Error: File doesn't exist */
    EXEC(fscanfx("nofile.txt", 's', newstr))
    EXEC(fscanfx("nofile.txt", 'd', &num))
    /* Error: Unrecognized format */
    EXEC(fscanfx("", 'S', newstr))
    EXEC(fscanfx("", 'X', newstr))
    EXEC(fscanfx("", 'D', &num))
    EXEC(fscanfx("", 'i', &num))
    /* Error: File doesn't exist AND unrecognized format */
    EXEC(fscanfx("nofile.txt", 'S', newstr))
    EXEC(fscanfx("nofile.txt", 'X', newstr))
    EXEC(fscanfx("nofile.txt", 'D', &num))
    EXEC(fscanfx("nofile.txt", 'i', &num))


    /* Receive a string from stdin */
    EXEC(fscanfx("", 's', newstr))
    /* Print out the string to stdout */
    EXEC(fprintfx("", 's', newstr))
    /* Receive an integer from stdin */
    EXEC(fscanfx("", 'd', &num))
    EXEC(fprintfx("", 'd', &num))

    /* Read a file of strings */
    while (!fscanfx(FILESHORT, 's', newstr)) {
        fprintfx("", 's', "Line: ");
        fprintfx("", 's', newstr);
    }

    /* Read a file of strings */
    while (!fscanfx(FILELONGG, 's', newstr)) {
        fprintfx("", 's', "Line: ");
        fprintfx("", 's', newstr);
    }

    /* Read a file of integers */
    while (!fscanfx(FILESHORT, 'd', &num)) {
        fprintfx("", 's', "Number: ");
        fprintfx("", 'd', &num);
    }

    /* Nothing should happen */
    EXEC(fscanfx(FILESHORT, 's', NULL))
    EXEC(fscanfx(FILESHORT, 'd', NULL))


    EXEC(clean())

    return 0;
}
