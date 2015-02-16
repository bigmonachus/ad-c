/**
 * serg_io.h
 *  Sergio Gonzalez
 *
 * File and stream functions
 *
 */

#pragma once

static const size_t bytes_in_fd(FILE* fd)
{
    fpos_t fd_pos;
    fgetpos(fd, &fd_pos);
    fseek(fd, 0, SEEK_END);
    size_t len = (size_t)ftell(fd);
    fsetpos(fd, &fd_pos);
    return len;
}

static const char* slurp_file(const char* path, size_t *out_size)
{
    FILE* fd = fopen(path, "r");
    assert(fd);
    if (!fd)
    {
        fprintf(stderr, "ERROR: couldn't slurp %s\n", path);
        return NULL;
    }
    const size_t len = bytes_in_fd(fd);
    const char* contents = malloc(len + 1);
    const size_t read = fread((void*)contents, 1, len, fd);
    // TODO: Do something about read != len?
    fclose(fd);
    *out_size = read;
    return contents;
}

