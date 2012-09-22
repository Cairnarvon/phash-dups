#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAXDIRS         1024
#define EVENT_BUF_LEN   1024 * (sizeof(struct inotify_event) + 16)

#ifndef IN_ONLYDIR
#define IN_ONLYDIR 0
#endif

char *watches[MAXDIRS];

void daemonize(void)
{
    pid_t pid;

    if (getppid() == 1)
        return;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid > 0)
        exit(0);

    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }

    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
}

void r_add_watch(int fd, char *dir, uint32_t mask)
{
    DIR *dirp = opendir(dir);
    struct dirent *entry;

    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char path[PATH_MAX];

            if (snprintf(path, PATH_MAX,
                         "%s/%s",
                         dir, entry->d_name) > PATH_MAX) {
                fprintf(stderr,
                        "Path too long! Skipping %s and subdirs.\n",
                        path);
            } else {
                int wd;

                wd = inotify_add_watch(fd, path, mask);
                if (wd < 0) {
                    perror("inotify_add_watch");
                } else if (wd >= MAXDIRS) {
                    fprintf(stderr, "Too many watched directories!\n");
                    inotify_rm_watch(fd, wd);
                } else {
                    fprintf(stderr, "Watching: %s (%d)\n", path, wd);
                    watches[wd] = strdup(path);
                    r_add_watch(fd, path, mask);
                }
            }
        }
    }
}

int mkdb(char *db_dir) {
    if (mkdir(db_dir, 0755) != 0) {
        extern int errno;

        if (errno == EEXIST) {
            struct stat s;

            stat(db_dir, &s);

            if (!S_ISDIR(s.st_mode)) {
                fprintf(stderr,
                        "%s exists and is not a directory.\n",
                        db_dir);
                return 1;
            }

        } else {
            perror("Invalid DB dir");
            return 1;
        }
    }
    return 0;
}

void usage(char *argv0)
{
    printf("Usage: %s [-d DB_DIR] [-f] [-h] [-r] [WATCH_DIR]\n\n"
           "  -d DB_DIR\n"
           "    Specify directory to use for the database. "
           "(default: $PHASHDB or $HOME/.phashdb)\n\n"
           "  -f\n    Run in the foreground.\n\n"
           "  -h\n    Display this message and exit.\n\n"
           "  -r\n    Watch all subdirectories too.\n\n"
           "  WATCH_DIR\n    The directory to watch. (default: .)\n\n",
           argv0);
}


int main(int argc, char **argv)
{
    int fd, recurse = 0, foreground = 0, c;
    char *db_dir = NULL;
    uint32_t mask = IN_ONLYDIR | IN_CREATE | IN_MOVED_TO;

    /* Option parsing */
    extern int optind;
    extern char *optarg;

    while ((c = getopt(argc, argv, "+rfd:h")) != -1) {
        switch (c) {
        case 'r':
            recurse = 1;
            break;
        case 'f':
            foreground = 1;
            break;
        case 'd':
            db_dir = optarg;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        }
    }

    if (optind < argc && chdir(argv[optind]) != 0) {
        printf("%d %d\n", argc, optind);
        perror(argv[optind]);
        return 1;
    }

    /* Default db_dir. */
    if (db_dir == NULL && (db_dir = getenv("PHASHDIR")) == NULL) {
        char *home = getenv("HOME");

        if (home == NULL)
            home = "/tmp";

        db_dir = malloc(strlen(home) + 10);
        snprintf(db_dir, strlen(home) + 10, "%s/.phashdb", home);
    }
    if (mkdb(db_dir) != 0)
        return 1;

    /* Daemonise if we should daemonise. */
    if (!foreground)
        daemonize();

    /* Install inotify watcher(s). */
    if ((fd = inotify_init()) == -1) {
        perror("inotify_init");
        return 1;
    }
    watches[inotify_add_watch(fd, ".", mask)] = ".";
    if (recurse)
        r_add_watch(fd, ".", mask);

    /* Lewp. */
    for (;;) {
        char buf[EVENT_BUF_LEN];
        int length, i = 0;

        if ((length = read(fd, buf, EVENT_BUF_LEN)) < 0) {
            perror("read");
            return 1;
        }

        while (i < length) {
            struct inotify_event *event = (struct inotify_event*) &buf[i];

            i += sizeof(struct inotify_even*) + event->len;

            if (event->wd == -1)
                continue;

            char path[PATH_MAX];
            snprintf(path, PATH_MAX, "%s/%s", watches[event->wd], event->name);

            struct stat s;
            if (stat(path, &s) != 0) {
                fprintf(stderr, "Can't stat %s!", path);
                continue;
            }

            if (S_ISDIR(s.st_mode) && recurse) {
                /* Add watcher to new directory. */
                int wd = inotify_add_watch(fd, path, mask);

                if (wd < 0)
                    perror("inotify_add_watch");
                else if (wd >= MAXDIRS)
                    fprintf(stderr, "Too many directories!\n");
                else {
                    watches[wd] = strdup(path);
                    fprintf(stderr, "Watching %s.\n", path);
                }

            } else {
                /* Index file. */
                int l = strlen(path) + strlen(db_dir) + 20;
                char *cmd = malloc(l);

                snprintf(cmd, l, "phash-index -d %s %s", db_dir, path);

                if (system(cmd) != 0)
                    fprintf(stderr, "Couldn't index %s.\n", path);
                else
                    fprintf(stderr, "Indexed %s.\n", path);

                free(cmd);
            }
        }
    }

    /* Never get here. */
    return 0;
}
