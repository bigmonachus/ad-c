#include "dummy_include.h"

struct MyStruct
{
    int x;
};

typedef struct
{
    void* something;
} OhGodNo;

int main()
{
    int foo = 1;
    struct MyStruct bar;
    OhGodNo a;

    a.something = 0;
    bar.x = 2;

    if (bar.x == 2)
    {
        printf("function call %s %d\n", "with an argument", 2);
    }
    else
    {
        bar.x = 42;
    }

    while (0)
    {

    }

    const int not_useful = 0;
    do {
        // Nothing!
        /* Seriously, nothing // */
        int inside_do = 1729;
    } while (not_useful);
    ++not_useful;
    --not_useful;


    printf("Hi there!");
}
