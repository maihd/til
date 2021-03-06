// msvc_test.cpp : Defines the entry point for the console application.
//

#define TIL_IMPL
#include "../til.h"

#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>

static jmp_buf jmpenv;

static void _sighandler(int sig)
{
    if (sig == SIGINT)
    {
        printf("\n");
        printf("Type '.exit' to exit\n");
        longjmp(jmpenv, 1);
    }
}

static char* strtrim_fast(char* str)
{
    while (isspace(*str))
    {
        str++;
    }

    char* ptr = str;
    while (*ptr > 0)      { ptr++; }; ptr--;
    while (isspace(*ptr)) { ptr--; }; ptr[1] = 0;

    return str;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, _sighandler);

    printf("TIL (Table In Lua) file format prompt - v1.0\n");
    printf("Type '.exit' to exit\n");

    char input[1024];
    while (1)
    {
        if (setjmp(jmpenv) == 0)
        {
            printf("> ");
            fgets(input, sizeof(input), stdin);

            const char* code = strtrim_fast(input);
            if (strcmp(code, ".exit") == 0)
            {
                break;
            }
            else
            {
                til_state_t* state;
                til_value_t* value = til_parse(code, &state);
                if (value)
                {
                    til_print(value, stdout); fprintf(stdout, "\n");
                }
                else
                {
                    value = NULL;
                }
            }
        }
    }

    return 0;
}

