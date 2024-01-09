#include <stdio.h>

void
do_print(const char* print_level, const char* msg)
{
    const char* program_name = "Pong";

    (void) printf("[%s|%s] %s\n", program_name, print_level, msg);
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
