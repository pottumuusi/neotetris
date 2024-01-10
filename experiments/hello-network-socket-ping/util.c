#include <stdio.h>

extern const char* g_program_name;

void
do_print(const char* print_level, const char* msg)
{
    (void) printf("[%s|%s] %s\n", g_program_name, print_level, msg);
}

void
print_info(const char* msg)
{
    const char* print_level = "I";

    do_print(print_level, msg);
}

void
print_error(const char* msg)
{
    const char* print_level = "E";

    do_print(print_level, msg);
}
