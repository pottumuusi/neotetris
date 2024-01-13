#include <stdio.h>

extern const char* g_program_name;

void
do_print(char const * const print_level, char const * const msg)
{
    (void) printf("[%s|%s] %s\n", g_program_name, print_level, msg);
}

void
print_info(char const * const msg)
{
    const char* print_level = "I";

    do_print(print_level, msg);
}

void
print_error(char const * const msg)
{
    const char* print_level = "E";

    do_print(print_level, msg);
}
