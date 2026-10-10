#include "stack.h"
#include "stack.cpp"

#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef unsigned char byte;


enum funcCodes : byte
{
    PUSH = 1,
    ADD = 2,
    OUT = 3,
    HLT = 0xF4
};


size_t CountFileSize (char* const fileName);
byte* ReadFile (size_t fileSize, FILE* file);
void ParseCommands (byte* const byteCode, size_t codeLen);
void Push (stack_t* stk, stackElem_t elem);
void Add (stack_t* stk);
void Out (stack_t* stk);


int main (int argc, char* argv [])
{
    if (argv [1] == nullptr)
    {
        char fileName [] = "bytes.txt";
        argv [1] = fileName;
    }    

    FILE* code = fopen (argv [1], "r");

    size_t filesize = CountFileSize (argv [1]);

    byte* byteCode = ReadFile (filesize, code);

    ParseCommands (byteCode, filesize);

    printf ("program finished correctly\n");

    fclose (code);
    free (byteCode);
}


size_t CountFileSize (char* const fileName)
{
    assert (fileName != nullptr);

    struct stat fileStat = {};

    assert (stat (fileName, &fileStat) != -1);

    return  fileStat.st_size;
}


byte* ReadFile (size_t fileSize, FILE* file)
{
    assert (file != nullptr);

    byte* tempText = (byte*) calloc (fileSize + 1, sizeof (tempText [0]));

    if (tempText == nullptr)
    {
        printf ("reading err");
        abort ();
    }

    byte* text = tempText;

    fread (text, sizeof (text [0]), fileSize, file);

    assert (text != nullptr);

    text [fileSize] = 0;

    return text;
}


void ParseCommands (byte* const byteCode, size_t codeLen)
{
    assert (byteCode != nullptr);

    size_t count = 0;

    stack_t valueStack = {};
    
    StackCtorCall(valueStack, 1);

    while (count < codeLen)
    {
        switch (byteCode [count])
        {
            case PUSH:
            {
                stackElem_t arg = *(stackElem_t*) (byteCode + count + 1);
                Push (&valueStack, arg);
                count += sizeof (stackElem_t);
                break;
            }

            case ADD:
            {
                Add (&valueStack);
                break;
            }

            case OUT:
            {
                Out (&valueStack);
                break;
            }

            case HLT:
            {
                StackDestroyCall(valueStack);
                return;
            }

            default:
                printf ("Invalid byte code\n");

                StackDestroyCall(valueStack);

                free (byteCode);
                abort ();
        }

        count++;
    }
}


void Push (stack_t* stk, stackElem_t elem)
{
    assert(stk != nullptr);

    StackPushCall(*stk, elem)
}


void Add (stack_t* stk)
{
    assert (stk->size > 1);

    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(*stk, first);
    StackPopCall(*stk, second);

    Push (stk, first + second);
}

void Out (stack_t* stk)
{
    assert (stk->size > 0);

    stackElem_t answer = GLOBAL_STACK_POISON;

    StackPopCall(*stk, answer);

    printf ("Your answer is: " PRINT_TYPE "\n", answer);
}