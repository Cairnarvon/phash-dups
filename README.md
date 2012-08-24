This is two things:

1. an C99 implementation of a perceptual hash function, which you can install as a shared library and use, and

2. a system that keeps track of duplicate (as far as the perceptual hashes are concerned) images on a filesystem.

The library can be built and used on any POSIX system, but the `phash-dups` system is Linux-only.

# Library

The library exports three functions:

* `uint64_t phash_dct(char*)`, the usual 64-bit DCT-based perceptual hash,
* `unsigned hamming_uint64_t(uint64_t, uint64_t)`, which calculates the Hamming distance between two hashes, and
* `unsigned ep_uint64_t(uint64_t, uint64_t)`, which calculates the Equality Percentage, and is just a macro using `hamming_uint64_t`.

If you're using a system that puts things in predictable places (like Debian), you can install it as follows (as root):

> `make install-lib`

If not, you'll have to build the shared object first:

> `make phash.so`

And then move `phash.so`, `phash.h`, and the man pages (`man/*.3`) to the right places yourself.

You will need the [ImageMagick MagickWand](http://www.imagemagick.org/script/install-source.php#unix) library. On Debian, this is the package `libmagickwand-dev`.

It should be noted that this library offers few to no advantages over [pHash](http://phash.org/).

# phash-dups

`phash-dups` is a system for monitoring and retrieving duplicate images in a folder or folder tree. It consists of the following programs:

* `phash-index`, which adds image files to the database,
* `phashd`, the daemon which monitors directories for new files, and
* `phash-dups`, which displays duplicates in some sort of file browser.

Additionally, there are two Bash helper scripts:

* `phash-init`, which indexes all files in a folder and then starts the daemon, and
* `phash-clean`, which deletes the database and kills the daemon.

The database is actually just a folder with subfolders for each hash and hard links to images mapping to that hash in each hash folder. Hard links are used both to keep space usage down and to avoid having to track file deletions, but come with the caveat that the monitored folder tree can't span multiple filesystems and that the database has to be on the same filesystem as the monitored folders.

The daemon uses `inotify` to watch directories, making it Linux-only.

## Installation

You will need [ImageMagick MagickWand](http://www.imagemagick.org/script/install-source.php#unix), as well as [inotify-tools](http://inotify-tools.sourceforge.net/). On Debian, these are the `libmagickwand-dev` and `libinotify-tools-dev` packages.

Once you have those, just do the following to build and install these programs:

> `make install`

If you aren't root, the programs will be installed to `~/bin`. If you're root, they'll be put in `/usr/bin`, and the man pages will be installed as well.

## Suggested usage

In principle you'd run `phash-init` once, and then add `phashd` to your crontab using the `@reboot` directive if you want it to be started automatically from then on.

> `$ phash-init -r ~/porns`

> `$ crontab -l >crontab.txt`

> `$ echo "@reboot phashd -r ~/porns" >>crontab.txt`

> `$ crontab crontab.txt`

`phashd` uses inotify to watch directories for new files, which it will add to the database using `phash-index`. Do not start `phashd` manually after running `phash-init`; `phash-init` will do so itself.

If you want to add an image to the database manually, you can use `phash-index` on it yourself.

Once you have this database, you can easily find matches for images using `phash-dups`:

> `$ phash-dups image.jpg`

If matches are found, a folder containing all of them will be opened using `xdg-open` (see the `xdg-mime` man page for configuration details). Deleting files in this folder will remove them from the database.

If for some reason you no longer want your database, you can delete it using `phash-clean`. This will also kill every running instance of `phashd` you have the rights to kill.

Read the man pages, or use the `-h` flag, for more information.
