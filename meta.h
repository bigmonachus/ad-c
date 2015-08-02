// meta.h
// (c) Copyright 2015 Sergio Gonzalez
//
// Released under the MIT license. See LICENSE.txt

/**
 *
 * Tools for doing meta-programming
 *
 */

#pragma once

#include <assert.h>
#ifndef _WIN32
#include <dirent.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "win_dirent.h"
#endif

#include "serg_io.h"
#include "memory.h"

void meta_clear_file(const char* fname);

// NOTE: this function dumbly replaces text. It doesn't care about grammar.
// e.g. it will replace tokens inside of strings.
void meta_expand(
        const char* result_path,
        const char* tmpl_path,
        const int num_bindings,
        ...);

// Searches for *.c *.h *.cpp and *.cc files in directory_path, parses them,
// and generates type info at output_path
void meta_type_info(
        const char* output_path,
        const char* directory_path);

// ============================================================

void meta_clear_file(const char* path)
{
    FILE* fd = fopen(path, "w+");
    assert(fd);
    static const char message[] = "//File generated from template by libserg/meta.h.\n\n";
    fwrite(message, sizeof(char), sizeof(message) - 1, fd);
    fclose(fd);
}

typedef struct
{
    char* str;
    size_t len;
} TemplateToken;

typedef struct
{
    TemplateToken name;
    TemplateToken substitution;
} Binding;

void meta_expand(
        const char* result_path,
        const char* tmpl_path,
        const int num_bindings,
        ...)
{
    size_t size = 300 * 1024 * 1024;
    void* big_block_of_memory = malloc(size);
    Arena root_arena = arena_init(big_block_of_memory, size);

    enum
    {
        LEX_NOTHING,
        LEX_DOLLAR,
        LEX_BEGIN,
        LEX_INSIDE,
    };

    // Get bindings

    // TODO: do a flexible stretchy array.
    Binding* bindings = arena_array(&root_arena, Binding, 1000);

    va_list ap;

    va_start(ap, num_bindings);
    for (int i = 0; i < num_bindings; ++i)
    {
        char* name = va_arg(ap, char*);
        char* subst = va_arg(ap, char*);

        TemplateToken tk_name = { name, strlen(name) };
        TemplateToken tk_subst = { subst, strlen(subst) };

        Binding binding = { tk_name, tk_subst };
        array_push(bindings, binding);
    }
    va_end(ap);

    FILE* out_fd = fopen(result_path, "a");
    assert(out_fd);
    int data_size = 0;
    const char* in_data = slurp_file(tmpl_path, &data_size);
    char* out_data = arena_array(&root_arena, char, 10 * 1024 * 1024);

    size_t path_len = strlen(tmpl_path);
    array_push(out_data, '/'); array_push(out_data, '/');
    for (int i = 0; i < path_len; ++i)
    {
        array_push(out_data, tmpl_path[i]);
    }
    array_push(out_data, '\n');
    array_push(out_data, '\n');

    TemplateToken* tokens = arena_array(&root_arena, TemplateToken, 5000);

    int lexer_state = LEX_NOTHING;

    char prev = 0;
    char* name = arena_array(&root_arena, char, 1000);
    int name_len = 0;
    for (int i = 0; i < data_size - 1; ++i)  // Don't count EOF
    {
        char c = in_data[i];
        // Handle transitions
        {
            if ( lexer_state == LEX_NOTHING && c != '$' )
            {
                array_push(out_data, c);
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
                array_push(name, c);
                ++name_len;
            }
            else if (lexer_state == LEX_INSIDE && c == '>')
            {
                // add token
                array_push(name, '\0');
                TemplateToken token;
                char* new_name = arena_array(&root_arena, char, strlen(name)+1);
                strcpy(new_name, name);
                token.str = new_name;
                token.len = name_len;
                array_reset(name);
                array_push(tokens, token);
                name_len = 0;
                lexer_state = LEX_NOTHING;

                // Do stupid search on args to get matching subst.
                for (int i = 0; i < array_count(bindings); ++i)
                {
                    const Binding binding = bindings[i];
                    if (!strcmp(binding.name.str, token.str))
                    {
                        for (int j = 0; j < binding.substitution.len; ++j)
                        {
                            char c = binding.substitution.str[j];
                            array_push(out_data, c);
                        }
                        break;
                    }
                }
            }
        }
        prev = c;
    }

    array_push(out_data, '\n');

    fwrite(out_data, sizeof(char), array_count(out_data), out_fd);
    fclose(out_fd);
    free(big_block_of_memory);
}

// Note:
//  Parser should output defines of the form
//  #define ADCTYPE_FILE_filename_SCOPE_scope_NAME_name
//  `scope` can be 'global' or the name of a top level function/struct/enum
//      e.g. my_file.c has a global int variable named foo
//      therefore we have:
//      #define ADCTYPE_FILE_my_file_SCOPE_global_NAME_foo int


char** known_types = 0;
char** type_decls = 0;


static int is_whitespace(char c)
{
    if (
            (c == ' ' ) |
            (c == '\v') |
            (c == '\t') |
            (c == '\r')
       )
    {
        return 1;
    }
    return 0;
}

static int is_ident_char(char c)
{
    static char idents[] =
    {
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F',
        'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        '_',
        '.', // We treat accessors as part of the identifier
        ':', // special support for light c++
    };
    for (int i = 0; i < sizeof(idents); ++i)
    {
        if (c == idents[i])
            return 1;
    }
    return 0;
}

enum
{
    LEX_BEGIN_LINE,
    LEX_WAIT_FOR_NEWLINE,
    LEX_ONE_SLASH,
    LEX_RECEIVING,
    LEX_INSIDE_STRING,
    LEX_INSIDE_CHAR,
    LEX_ASSIGN,
};

enum
{
    PARSE_TOP         = (1 << 0),
    PARSE_GOT_STRUCT  = (1 << 1),
    PARSE_IN_STRUCT   = (1 << 2),
    PARSE_GOT_TYPEDEF = (1 << 3),
    PARSE_ADD_TYPE    = (1 << 4),
    PARSE_GOT_ENUM    = (1 << 5),
    PARSE_IN_FUNC     = (1 << 6),
};


#define BUFFERSIZE (10 * 1024)

static void process_file(const char* fname)
{
    size_t size = 300 * 1024 * 1024;
    void* big_block_of_memory = malloc(size);
    Arena root_arena = arena_init(big_block_of_memory, size);

    int file_size;
    const char* file_contents = slurp_file(fname, &file_size);
    int lex_state = LEX_BEGIN_LINE;
    int parse_state = PARSE_TOP;

    char* tokenstack[BUFFERSIZE];
    int tokencount = 0;

    char* current_func = 0;  // Used for code-gen

    if (file_contents)
    {

        int brace_count = 0; // To determine if leaving function scope.
        char** idents = 0;
        char* curtok = arena_array(&root_arena, char, 2000);
        char prev_c = 0;

        int newtoken = 0;  // Set to one when last loop emitted an identifier.

        for (int i = 0; i < file_size; ++i)
        {
            char c = file_contents[i];

            if (tokencount && !(parse_state & PARSE_GOT_STRUCT) &&
                    !(parse_state & PARSE_IN_STRUCT) &&
                    !(parse_state & PARSE_IN_FUNC) &&
                    !strcmp(tokenstack[tokencount - 1], "struct"))
            {
                parse_state |= PARSE_GOT_STRUCT;
            }
            else if (tokencount && !(parse_state & PARSE_GOT_TYPEDEF) && !strcmp(tokenstack[tokencount - 1], "typedef"))
            {
                parse_state |= PARSE_GOT_TYPEDEF;
            }
            else if (tokencount && !(parse_state & PARSE_GOT_ENUM) && !strcmp(tokenstack[tokencount - 1], "enum"))
            {
                parse_state |= PARSE_GOT_ENUM;
            }
            ///// Check struct
            if ((parse_state & PARSE_GOT_STRUCT) && prev_c == ';')
            {
                parse_state ^= PARSE_GOT_STRUCT;
            }
            if ((parse_state & PARSE_GOT_STRUCT) && c == '{')
            {
                parse_state ^= PARSE_GOT_STRUCT;
                parse_state |= PARSE_IN_STRUCT;
            }
            if ((parse_state & PARSE_IN_STRUCT) && c == '}')
            {
                parse_state ^= PARSE_IN_STRUCT;
            }
            ///// check enum
            if ((parse_state & PARSE_GOT_ENUM) && c == '}')
            {
                parse_state ^= PARSE_GOT_ENUM;
            }
            ///// check typedef (to append to known types)
            if ((parse_state & PARSE_GOT_TYPEDEF) && !(parse_state & PARSE_IN_STRUCT) && c == ';')
            {
                // Please, when the token is finalized, append to known types
                parse_state |= PARSE_ADD_TYPE;
                parse_state ^= PARSE_GOT_TYPEDEF;
            }
            ///////////
            //// check for function.
            ///////////
            if ((parse_state == PARSE_TOP) && c == '{')
            {
                char* funcname = tokenstack[tokencount - 1];
                printf("Got function, named: %s\n", funcname);
                current_func = funcname;
                parse_state |= PARSE_IN_FUNC;
                tokenstack[tokencount++] = ";";  // Anchor for doing type chencking
            }
            if ((parse_state & PARSE_IN_FUNC) && c == '{')
            {
                brace_count += 1;
            }
            if ((parse_state & PARSE_IN_FUNC) && c == '}')
            {
                brace_count -= 1;
                if (brace_count == 0)
                {
                    printf("Leaving function %s.\n", current_func);
                    current_func = 0;
                    parse_state ^= PARSE_IN_FUNC;
                } else
                {
                    tokenstack[tokencount++] = ";";  // Anchor for doing type chencking
                }
            }
            ///////////
            // HANDLE STUFF THAT SHOULD BE IGNORED
            // ////////
            if ( lex_state == LEX_WAIT_FOR_NEWLINE && c == '\n' )
            {
                lex_state = LEX_BEGIN_LINE;
            }
            else if ( lex_state == LEX_BEGIN_LINE && c == '#' )
            {
                lex_state = LEX_WAIT_FOR_NEWLINE;
            }
            else if ( lex_state == LEX_BEGIN_LINE && is_whitespace(c) )
            {
                lex_state = LEX_RECEIVING;
            }
            else if ( (lex_state == LEX_RECEIVING || lex_state == LEX_BEGIN_LINE) && c == '/')
            {
                lex_state = LEX_ONE_SLASH;
            }
            else if ( lex_state == LEX_ONE_SLASH && c == '/' )
            {
                lex_state = LEX_WAIT_FOR_NEWLINE;
            }
            else if (lex_state == LEX_BEGIN_LINE && c == '\n')
            {
                lex_state = LEX_BEGIN_LINE;
            }
            ////////
            // Handle strings
            ////////
            else if ( (lex_state == LEX_RECEIVING || lex_state == LEX_BEGIN_LINE) && c == '\"' )
            {
                lex_state = LEX_INSIDE_STRING;
            }
            else if ( lex_state == LEX_INSIDE_STRING && c == '\"' && prev_c != '\\' )
            {
                lex_state = LEX_RECEIVING;
            }
            else if ( (lex_state == LEX_RECEIVING || lex_state == LEX_BEGIN_LINE) && c == '\'' )
            {
                lex_state = LEX_INSIDE_CHAR;
            }
            else if ( lex_state == LEX_INSIDE_CHAR && c == '\''  && prev_c != '\\' )
            {
                lex_state = LEX_RECEIVING;
            }
            ///////
            // Assignment.
            //////
            else if ( (lex_state == LEX_RECEIVING || lex_state == LEX_BEGIN_LINE) && c == '=' )
            {
                lex_state = LEX_ASSIGN;  // Need to differentiate between assignmt and equals operator.
            }
            else if (tokencount &&
                    (
                     ((lex_state == LEX_ASSIGN && c != '='))
                     ||
                     ((parse_state & PARSE_IN_FUNC) && !strcmp(tokenstack[tokencount - 1], ";"))
                    )
                    )
            {
                newtoken = 0;  // reset
                int i = tokencount;
                char* token = tokenstack[i - 1];
                int tokens_consumed = 0;
                while (i)
                {
                    token = tokenstack[--i];
                    int found_anchor = !strcmp(token, ";");
                    if (!found_anchor)
                    {
                        array_push(type_decls, token);
                    }
                    else
                    {
                        break;
                    }
                    ++tokens_consumed;
                }
                if (tokens_consumed)
                {
                    //tokencount -= tokens_consumed;
                    array_push(type_decls, current_func);
                    array_push(type_decls, 0);
                }

                if(lex_state == LEX_ASSIGN) lex_state = LEX_RECEIVING;
            }
            ////////
            // parse token
            ///////
            if ( lex_state == LEX_RECEIVING || lex_state == LEX_BEGIN_LINE )
            {
                lex_state = LEX_RECEIVING;
                if (is_ident_char(c))
                {
                    array_push(curtok, c);
                }
                else  // Finish token
                {
                    // Handle empty line.
                    if (array_count(curtok))
                    {
                        static const char* control_flow[] =
                        {
                            "do", "while", "for",
                            "if", "else",
                        };
                        // Filter control flow
                        int is_control = 0;
                        {
                            for (int i = 0; i < sizeof(control_flow) / sizeof(char*); ++i)
                            {
                                if (!strcmp(curtok, control_flow[i]))
                                {
                                    is_control = 1;
                                    break;
                                }
                            }
                        }
                        if (!is_control)
                        {
                            array_push(curtok, '\0');
                            newtoken = 1;
                            char* token = arena_array(&root_arena, char, array_count(curtok));
                            strcpy(token, curtok);
                            if (parse_state & PARSE_ADD_TYPE)
                            {
                                parse_state ^= PARSE_ADD_TYPE;
                                array_push(known_types, token);
                                printf("Typeinfo: added type %s\n", token);
                            }
                            if (parse_state & PARSE_GOT_STRUCT)
                            {
                                array_push(known_types, token);
                                printf("Typeinfo: added struct name %s\n", token);
                            }

                            tokenstack[tokencount++] = token;
                        }
                    }
                    // push semicolons
                    if (c == ';')
                    {
                        tokenstack[tokencount++] = ";";
                    }
                    if (c == '*')
                    {
                        tokenstack[tokencount++] = "*";
                    }
                    // Reset token
                    array_reset(curtok);
                }
            }
            prev_c = c;
        }
    }
    else
    {
        fprintf(stderr, "Could not open file for processing, %s\n", fname);
    }

}

void meta_type_info(
        const char* output_path,
        const char* directory_path)
{
    assert(output_path);
    assert(directory_path);

    size_t size = 300 * 1024 * 1024;
    void* big_block_of_memory = malloc(size);
    Arena root_arena = arena_init(big_block_of_memory, size);

    known_types = arena_array(&root_arena, char*, 3000);
    type_decls  = arena_array(&root_arena, char*, 3000);
    // Init known C types
    {
        // valid identifiers in type declarations
        char* init_types[] =
        {
            "static", "const",
            "unsigned", "char", "short", "int", "long", "float", "double",
            "uint8_t", "uint16_t", "uint32_t", "uint64_t",
            "int8_t", "int16_t", "int32_t", "int64_t",
            "struct",
            "*",
        };
        int count_types = (sizeof(init_types) / sizeof(char*));
        for (int i = 0; i < count_types; ++i)
        {
            array_push(known_types, init_types[i]);
        }
    }

    FILE* fd = fopen(output_path, "w+");
    if (!fd)
    {
        fprintf(stderr, "Could not open file %s for writing.\n", output_path);
        exit(-1);
    }
    // Traverse directory, fill filename array
    DIR* dirstack[256] = { 0 };
    int dircount = 0;

    DIR* dir = opendir(directory_path);
    dirstack[dircount++] = dir;
    while (dircount)
    {
        DIR* dir = dirstack[--dircount];
        struct dirent* ent = readdir(dir);
        char fname[1024];
        while(ent)
        {
            fname[0] = '\0';
            // TODO: this does not compile on OSX. FIXME
            //strcat(fname, dir->dname);
            fname[strlen(fname) - 1] = '\0'; // Remove * character
            strcat(fname, ent->d_name);

            const size_t fname_len = strlen(fname);
            if (fname[fname_len - 1] != '.')
            {
                DIR* sub_dir = opendir(fname);
                if (sub_dir)
                {
                    dirstack[dircount++] = sub_dir;
                }
                else  // Is a file.
                {
                    static char* acceptable_exts[] = {"cc", "cpp", "c", "h", "hh", "hpp"};

                    int accepted = 0;
                    { // Check that file extension is OK
                        char* fname_ext = 0;
                        for(size_t i = fname_len; i > 0; --i)
                        {
                            int c = fname[i];
                            if (c == '.')
                            {
                                fname_ext = fname + i + 1;
                                break;
                            }
                        }
                        for (int i = 0; i < sizeof(acceptable_exts) / sizeof(char*); ++i)
                        {
                            if (!strcmp(fname_ext, acceptable_exts[i]))
                            {
                                accepted = 1;
                                break;
                            }
                        }
                    }
                    // Process all C files.
                    if (accepted)
                    {
                        printf("[...] Processing %s\n", fname);
                        process_file(fname);
                    }
                }
            }
            ent = readdir(dir);
        }

        closedir(dir);
    }
    // Do output!
    {
        int debug_limit = 40;
        int begin = 0;
        int end = 0;
        while (end < array_count(type_decls))
        {
            // Move end to next 0
            while (type_decls[++end] != 0);
            char* var_name = type_decls[begin];
            char* func_name = type_decls[end - 1];
            int is_valid = 1;
            char** type = 0;
            for (int i = begin + 1; i < end - 1; ++i)
            {
                char* type = type_decls[i];
                int is_type = 0;
                for (int j = 0; j < array_count(known_types); ++j)
                {
                    if (!strcmp(type, known_types[j]))
                    {
                        is_type = 1;
                        break;
                    }
                }
                if (!is_type)
                {
                    is_valid = 0;
                    break;
                }
            }
            // We had to go through the loop
            if (end - begin < 3)
            {
                is_valid = 0;
            }
            if (is_valid)
            {
                puts("==== Valid type");
                puts(func_name);
                char identifier[1024] = { 0 };
                char type_str[1024] = { 0 };
                sprintf((char*)identifier, "ADC_TYPE__FUNC__%s__NAME__%s",
                                func_name, var_name);
                for (int i = end - 2; i >= begin + 1; --i)
                {
                    char* type = type_decls[i];
                    int is_qualifier = 0;
                    // Filter type qualifiers (const static volatile)
                    static const char* qualifiers[] =
                    {
                        "static", "const", "volatile", "auto",
                    };
                    {
                        for (int j = 0; j < sizeof(qualifiers) / sizeof(char*); ++j)
                        {
                            if (!strcmp(type, qualifiers[j]))
                            {
                                is_qualifier = 1;
                            }
                        }
                    }
                    if (!is_qualifier)
                    {
                        sprintf((char*)type_str, "%s %s ", type_str, type);
                        puts(type);
                    }
                }
                puts(var_name);
                char* header_line[1024];
                sprintf((char*)header_line, "#ifndef %s\n#define %s %s\n#endif\n",
                        identifier, identifier, type_str);
                fwrite((const char*)header_line, sizeof(char), strlen((char*)header_line), fd);

            }
            begin = end + 1;
        }
    }
}
