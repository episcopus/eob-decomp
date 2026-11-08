/* files.c: platform_file_* over directories of the host (platform.h), for both programs: the game's
   files, and the player's own (saved games). A name is matched without case, so a game directory
   copied from a case-sensitive file system works. */
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "platform.h"

static char game_dir[1024] = ".";
static char save_dir[1024];

void platform_set_game_dir(const char *dir)
{
    snprintf(game_dir, sizeof game_dir, "%s", dir);
}

void platform_set_save_dir(const char *dir)
{
    snprintf(save_dir, sizeof save_dir, "%s", dir ? dir : "");
}

/* the host path of `name` in the directory `where`, matched without case; 0 or -1 */
static int find_in(const char *where, const char *name, char *out, size_t n)
{
    DIR *d = opendir(where);
    struct dirent *e;
    int found = -1;
    if (d == NULL) {
        return -1;
    }
    while ((e = readdir(d)) != NULL) {
        const char *a = e->d_name, *b = name;
        while (*a && *b && toupper((unsigned char)*a) == toupper((unsigned char)*b)) {
            a++;
            b++;
        }
        if (*a == 0 && *b == 0) {
            snprintf(out, n, "%s/%s", where, e->d_name);
            found = 0;
            break;
        }
    }
    closedir(d);
    return found;
}

/* the player's copy of a file, else the game's */
static int find(const char *name, char *out, size_t n)
{
    if (save_dir[0] && find_in(save_dir, name, out, n) == 0) {
        return 0;
    }
    return find_in(game_dir, name, out, n);
}

int32_t platform_file_size(const char *name)
{
    char path[1300];
    FILE *f;
    long size;
    if (find(name, path, sizeof path) != 0 || (f = fopen(path, "rb")) == NULL) {
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fclose(f);
    return size < 0 ? -1 : (int32_t)size;
}

int platform_file_read(const char *name, uint8_t *buf, uint32_t size)
{
    char path[1300];
    FILE *f;
    size_t got;
    if (find(name, path, sizeof path) != 0 || (f = fopen(path, "rb")) == NULL) {
        return -1;
    }
    got = fread(buf, 1, size, f);
    fclose(f);
    return got == size ? 0 : -1;
}

/* a whole file, by way of a temporary one and a rename: a run that dies leaves the old save whole */
int platform_file_write(const char *name, const uint8_t *buf, uint32_t size)
{
    char path[1300], tmp[1320];
    FILE *f;
    int ok;
    if (!save_dir[0]) {
        return 0;
    }
    if (find_in(save_dir, name, path, sizeof path) != 0) {
        snprintf(path, sizeof path, "%s/%s", save_dir, name);
    }
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    if ((f = fopen(tmp, "wb")) == NULL) {
        fprintf(stderr, "eob-port: cannot write %s: %s\n", tmp, strerror(errno));
        return -1;
    }
    ok = (size == 0 || fwrite(buf, 1, size, f) == size);
    ok = (fclose(f) == 0) && ok;
    if (!ok || rename(tmp, path) != 0) {
        fprintf(stderr, "eob-port: cannot write %s: %s\n", path, strerror(errno));
        remove(tmp);
        return -1;
    }
    return 0;
}

static int mkdirs(char *path)
{
    char *p;
    for (p = path + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(path, 0755) != 0 && errno != EEXIST) {
                *p = '/';
                return -1;
            }
            *p = '/';
        }
    }
    return mkdir(path, 0755) != 0 && errno != EEXIST ? -1 : 0;
}

static int is_game_dir(const char *dir)
{
    char path[1300];
    struct stat st;
    snprintf(path, sizeof path, "%s/EOBDATA1.PAK", dir);
    return stat(path, &st) == 0;
}

int platform_find_install(char *game, size_t game_n, char *saves, size_t save_n)
{
    static const char *const places[] = {"../Resources/game", "Eye of the Beholder.app/Contents/Resources/game", "game"};
    char exe[1024] = "", *slash;
    const char *home = getenv("HOME");
    unsigned i;
#ifdef __APPLE__
    uint32_t n = sizeof exe;
    if (_NSGetExecutablePath(exe, &n) != 0) {
        return -1;
    }
#else
    ssize_t n = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (n <= 0) {
        return -1;
    }
    exe[n] = 0;
#endif
    if ((slash = strrchr(exe, '/')) == NULL) {
        return -1;
    }
    *slash = 0;
    for (i = 0; i < sizeof places / sizeof places[0]; i++) {
        snprintf(game, game_n, "%s/%s", exe, places[i]);
        if (is_game_dir(game)) {
            break;
        }
    }
    if (i == sizeof places / sizeof places[0]) {
        return -1;
    }
#ifdef __APPLE__
    snprintf(saves, save_n, "%s/Library/Application Support/Eye of the Beholder", home ? home : ".");
#else
    snprintf(saves, save_n, "%s/.local/share/eye-of-the-beholder", home ? home : ".");
#endif
    return mkdirs(saves);
}
