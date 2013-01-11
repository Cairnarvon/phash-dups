#include <errno.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "phash.h"

void usage(char *argv0)
{
    printf("Usage: %s [-d DB_DIR] [-h] IMAGE\n\n"
           "  -d DB_DIR\n"
           "    Specify directory to use for the database."
           " (default: $PHASHDB or $HOME/.phashdb)\n\n"
           "  -s\n    Index using symlinks instead of hard links.\n\n"
           "  -h\n    Display this message and exit.\n\n"
           "  IMAGE\n"
           "    Image to hash and add to the database.\n\n",
           argv0);
}

int main(int argc, char **argv)
{
    char *db_dir = NULL;
    extern int errno;
    int (*mklink)(const char*, const char*) = link;

    /* Parse options. */
    int c;
    extern int optind;
    extern char *optarg;

    while ((c = getopt(argc, argv, "+d:sh")) != -1) {
        switch (c) {
        case 'd':
            db_dir = strdup(optarg);
            break;
        case 's':
            mklink = symlink;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        }
    }

    /* Default db_dir. */
    if (db_dir == NULL && (db_dir = getenv("PHASHDB")) == NULL) {
        char *home = getenv("HOME");

        if (home == NULL)
            home = "/tmp";

        db_dir = malloc(strlen(home) + 10);
        snprintf(db_dir, strlen(home) + 10, "%s/.phashdb", home);
    }

    /* Calculate hash. */
    errno = 0;
    uint64_t h = phash_dct(argv[optind]);
    if (h == (uint64_t)0 && errno == EINVAL)
        /* Not an image; ignore. */
        return 0;

    /* Create db_dir if it doesn't exist yet. */
    errno = 0;
    if (mkdir(db_dir, 0755) != 0 && errno != EEXIST) {
        perror("Can't create DB dir");
        return 1;
    }

    /* Create the hash folder if it doesn't exist yet. */
    unsigned sz = strlen(db_dir) + 16 + 2;
    char *hdir = malloc(sz);

    snprintf(hdir, sz, "%s/%016" PRIx64, db_dir, h);

    errno = 0;
    if (mkdir(hdir, 0755) != 0 && errno != EEXIST) {
        perror("Can't create folder in DB dir");
        return 1;
    }

    /* Determine a valid filename. */
    char *arg = strdup(argv[optind]),
         *bn = basename(arg),
         *fn = malloc((sz = strlen(hdir) + strlen(bn) + 2));
    FILE *f;

    snprintf(fn, sz, "%s/%s", hdir, bn);

    while ((f = fopen(fn, "r")) != NULL) {
        fclose(f);

        char *ext = strdup(strrchr(fn, '.'));
        if (ext == NULL) ext = "";
        int el = strlen(ext);

        fn = realloc(fn, ++sz);
        strcpy(fn + sz - el - 1, ext);
        free(ext);
        fn[sz - el - 2] = '_';
    }

    /* Create link. */
    if (mklink(argv[optind], fn) != 0) {
        perror("Can't link");
        return 1;
    }

    return 0;
}
