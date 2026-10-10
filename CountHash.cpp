#include <stdio.h>

int CountHash (const char* command);

int main ()
{
    char command [5] = "";

    scanf ("%s", command);

    printf ("%d", CountHash (command));
}

int CountHash (const char* command)
{
    return *(int*) command;
}