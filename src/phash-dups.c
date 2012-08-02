#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "phash.h"

void usage(char *argv0)
{
    printf("Usage: %s [-c OPENER] [-d DB_DIR] [-h] IMAGE\n\n"
           "  -c OPENER\n"
           "    Specify program with which to open results folder."
           " (default: xdg-open)\n\n"
           "  -d DB_DIR\n"
           "    Specify directory to use for the database."
           " (default: $PHASHDB or $HOME/.phashdb)\n\n"
           "  -h\n    Display this message and exit.\n\n"
           "  IMAGE\n"
           "    Image of which you'd like to find duplicates.\n\n",
           argv0);
}

int main(int argc, char **argv)
{
    char *db_dir = NULL, *opener = "xdg-open";
    unsigned sz;

    /* Parse options. */
    int c;
    extern int optind;
    extern char *optarg;
    while ((c = getopt(argc, argv, "+d:c:h")) != -1) {
        switch (c) {
        case 'd':
            db_dir = strdup(optarg);
            break;
        case 'c':
            opener = strdup(optarg);
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        }
    }

    if (optind == argc) {
        fprintf(stderr, "Need arg.\n");
        return 1;
    }

    /* Default db_dir. */
    if (db_dir == NULL && (db_dir = getenv("PHASHDB")) == NULL) {
        char *home = getenv("HOME");

        if (home == NULL)
            home = "/tmp";

        sz = strlen(home) + 10;
        db_dir = malloc(sz);
        snprintf(db_dir, sz, "%s/.phashdb", home);
    }

    /* Calculate hash. */
    uint64_t h;
    extern int errno;

    errno = 0;
    h = phash_dct(argv[optind]);

    if (h == (uint64_t)0 && errno == EINVAL) {
        fprintf(stderr, "Not an image: %s\n", argv[optind]);
        return 1;
    }

    /* Check if directory exists. */
    char *hdir;
    struct stat s;
    
    sz = strlen(db_dir) + 20;
    hdir = malloc(sz);
    snprintf(hdir, sz, "%s/%016" PRIx64, db_dir, h);

    errno = 0;
    if (stat(hdir, &s) != 0) {
        if (errno == ENOENT) {
            printf("No matches found.\n");
            return 0;
        } else if (errno) {
            perror("stat");
            return 1;
        }
    }

    /* It does, so open it. */
    char *cmd;

    sz += strlen(opener) + 1;
    cmd = malloc(sz);
    snprintf(cmd, sz, "%s %s", opener, hdir);

    return system(cmd);
}
