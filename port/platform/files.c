/* files.c: platform_file_* over a directory of the host (platform.h), for both programs. A name is
   matched without case, so a game directory copied from a case-sensitive file system works. */
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "platform.h"

static char game_dir[1024] = ".";

void platform_set_game_dir(const char *dir)
{
    snprintf(game_dir, sizeof game_dir, "%s", dir);
}

/* the host path of `name`, matched without case; 0 or -1 */
static int find(const char *name, char *out, size_t n)
{
    DIR *d = opendir(game_dir);
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
            snprintf(out, n, "%s/%s", game_dir, e->d_name);
            found = 0;
            break;
        }
    }
    closedir(d);
    return found;
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
