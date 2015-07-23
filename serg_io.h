// serg_io.h
// (c) Copyright 2015 Sergio Gonzalez
//
// Released under the MIT license. See LICENSE.txt

/**
 *
 * File and stream functions
 *
 */

#pragma once


static const int bytes_in_fd(FILE* fd)
{
    fpos_t fd_pos;
    fgetpos(fd, &fd_pos);
    fseek(fd, 0, SEEK_END);
    int len = ftell(fd);
    fsetpos(fd, &fd_pos);
    return len;
}

static const char* slurp_file(const char* path, int *out_size)
{
    FILE* fd = fopen(path, "r");
    if (!fd)
    {
        fprintf(stderr, "ERROR: couldn't slurp %s\n", path);
        assert(fd);
        return NULL;
    }
    int len = bytes_in_fd(fd);
    char* contents = malloc(len + 1);
    if (contents)
    {
        // TODO: Truncating from size_t to int... Technically a bug but will probably never happen
        const int read = (int)fread((void*)contents, 1, (size_t)len, fd);
        // TODO: Do something about read != len?
        fclose(fd);
        *out_size = read + 1;
        contents[read] = '\0';
    }
    return contents;
}

