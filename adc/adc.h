#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_sb.h"

// ============================================================
// ad-C
// ============================================================

void adc_clear_file(const char* fname);

// NOTE: this function dumbly replaces text. It doesn't care about grammar.
// e.g. it will replace tokens inside of strings.
void adc_expand(
        const char* result_path,
        const char* tmpl_path,
        ...);

// ============================================================

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
    const char* contents = malloc(len);
    fread((void*)contents, len, 1, fd);
    fclose(fd);
    *out_size = len;
    return contents;
}

void adc_clear_file(const char* path)
{
    FILE* fd = fopen(path, "w+");
    assert(fd);
    static const char message[] = "//File generated from template by ad-C.\n\n";
    fwrite(message, sizeof(char), sizeof(message) - 1, fd);
    fclose(fd);
}

typedef struct
{
    char* str;
    size_t len;
} Token;

typedef struct
{
    Token name;
    Token substitution;
} Binding;

enum
{
    LEX_NOTHING,
    LEX_DOLLAR,
    LEX_BEGIN,
    LEX_INSIDE,
};

void adc_expand(
        const char* result_path,
        const char* tmpl_path,
        const int num_bindings,
        ...)
{
    // Get bindings

    Binding* bindings = NULL;

    va_list ap;

    va_start(ap, num_bindings);
    for (int i = 0; i < num_bindings; ++i)
    {
        char* name = va_arg(ap, char*);
        char* subst = va_arg(ap, char*);

		Token tk_name = { name, strlen(name) };
		Token tk_subst = { subst, strlen(subst) };

        Binding binding = { tk_name, tk_subst };
        sb_push(bindings, binding);
    }
    va_end(ap);

    FILE* out_fd = fopen(result_path, "a");
    assert(out_fd);
    size_t data_size = 0;
    const char* in_data = slurp_file(tmpl_path, &data_size);
    char* out_data = NULL;

    size_t path_len = strlen(tmpl_path);
    sb_push(out_data, '/'); sb_push(out_data, '/');
    for (int i = 0; i < path_len; ++i)
    {
        sb_push(out_data, tmpl_path[i]);
    }
    sb_push(out_data, '\n');
    sb_push(out_data, '\n');

    Token* tokens = NULL;

    int lexer_state = LEX_NOTHING;

    char prev = 0;
    char* name = NULL;
	int name_len = 0;
    for (int i = 0; i < data_size; ++i)
    {
        char c = in_data[i];
        // Handle transitions
        {
            if ( lexer_state == LEX_NOTHING && c != '$' )
            {
                sb_push(out_data, c);
            }
            if ( lexer_state == LEX_NOTHING && c == '$' )
            {
                lexer_state = LEX_DOLLAR;
            }
            else if ( lexer_state == LEX_DOLLAR && c == '<' )
            {
                lexer_state = LEX_INSIDE;
            }
            else if (lexer_state == LEX_INSIDE && c != '>')
            {
                // add char to name
                sb_push(name, c);
                ++name_len;
            }
            else if (lexer_state == LEX_INSIDE && c == '>')
            {
                // add token
				sb_push(name, '\0');
                Token token;
                token.str = name;
				token.len = name_len;
				name = NULL;
                sb_push(tokens, token);
				name_len = 0;
                lexer_state = LEX_NOTHING;

                // Do stupid search on args to get matching subst.
                for (int i = 0; i < sb_count(bindings); ++i)
                {
                    const Binding binding = bindings[i];
                    if (!strcmp(binding.name.str, token.str))
                    {
                        for (int j = 0; j < binding.substitution.len; ++j)
                        {
                            char c = binding.substitution.str[j];
                            sb_push(out_data, c);
                        }
						break;
                    }
                }
            }
        }
        prev = c;
    }

    sb_push(out_data, '\n');

    fwrite(out_data, sizeof(char), sb_count(out_data), out_fd);
    fclose(out_fd);
}
