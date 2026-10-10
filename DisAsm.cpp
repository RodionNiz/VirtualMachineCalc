#include <cassert>
#include <cstddef>
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
void DisAsm (byte* byteCode, size_t codeLen, FILE* textCodeFile);

int main (int argc, char* argv [])
{
    if (argv [1] == nullptr)
    {
        char readFileName [] = "bytes.txt";
        argv [1] = readFileName;
    } 

    if (argv [2] == nullptr)
    {
        char writeFileName [] = "codes.txt";
        argv [2] = writeFileName;
    }

    FILE* byteCodeFile = fopen (argv [1], "r");
    FILE* textCodeFile = fopen (argv [2], "w");

    size_t filesize = CountFileSize (argv [1]);

    byte* byteCode = ReadFile (filesize, byteCodeFile);

    if (byteCode == nullptr)
    {
        return -1;
    }

    DisAsm (byteCode, filesize, textCodeFile);

    fclose (byteCodeFile);
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
        return nullptr;
    }

    byte* text = tempText;

    fread (text, sizeof (text [0]), fileSize, file);

    assert (text != nullptr);

    text [fileSize] = 0;

    return text;
}


void DisAsm (byte* byteCode, size_t codeLen, FILE* textCodeFile)
{
    assert (byteCode != nullptr);
    assert (textCodeFile != nullptr);

    size_t count = 0;

    while (count < codeLen)
    {
        switch (byteCode [count])
        {
            case PUSH:
            {
                double arg = *(double*) (byteCode + count + 1);
                count += sizeof (double);

                fprintf (textCodeFile, "%s %lg\n", "PUSH", arg);

                break;
            }

            case ADD:
            {
                fprintf (textCodeFile, "%s\n", "ADD");
                break;
            }

            case OUT:
            {
                fprintf (textCodeFile, "%s\n", "OUT");
                break;
            }

            case HLT:
            {
                fprintf (textCodeFile, "%s\n", "HLT");
                break;
            }

            default:
                printf ("Invalid byte code\n");
                free (byteCode);
                abort ();
        }

        count++;
    }

    fclose (textCodeFile);
}