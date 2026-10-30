#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>

typedef unsigned char byte;

int CountHash (const char* command);
size_t CountFileSize (char* const fileName);
byte* ReadFile (size_t* fileSize, FILE* file);
size_t CountStrings (byte* text, size_t textSize);
void GenHeader (FILE* header, size_t nOfStr, FILE* readFile);

int main (int argc, char* argv [])
{
    if (argv [1] == nullptr)
    {
        char readFileName [] = "commandsList.txt";
        argv [1] = readFileName;
    }

    FILE* textCodes = fopen (argv [1], "r");
    FILE* commandsHeader = fopen ("commands.h", "w");

    size_t size = CountFileSize (argv [1]);
    
    byte* text = ReadFile (&size, textCodes);

    if (text == nullptr)
    {
        return -1;
    }

    size_t nOfStr = CountStrings (text, size);

    GenHeader (commandsHeader, nOfStr, textCodes);

    free (text);
    fclose (textCodes);
    fclose (commandsHeader);
}


size_t CountFileSize (char* const fileName)
{
    assert (fileName != nullptr);

    struct stat fileStat = {};

    assert (stat (fileName, &fileStat) != -1);

    return  fileStat.st_size;
}


byte* ReadFile (size_t* fileSize, FILE* file)
{
    assert (file != nullptr);

    byte* tempText = (byte*) calloc (*fileSize + 1, sizeof (tempText [0]));

    if (tempText == nullptr)
    {
        printf ("reading err");
        return nullptr;
    }

    byte* text = tempText;

    *fileSize = fread (text, sizeof (text [0]), *fileSize, file);

    assert (text != nullptr);

    text [*fileSize] = 0;

    return text;
}


size_t CountStrings (byte* text, size_t textSize)
{
    assert (text != nullptr);
    assert (textSize != 0);

    size_t count = 0;
    size_t strCount = 0;

    while (count < textSize)
    {
        if (text [count] == '\n')
        {
            strCount++;
        }

        count++;
    }

    if (text [textSize - 1] != '\n')
    {
        strCount++;
    }

    return strCount;
}


void GenHeader (FILE* header, size_t nOfStr, FILE* readFile)
{
    fseek (readFile, 0, SEEK_SET); 

    long long int version = 1014;

    fprintf (header, "typedef unsigned char byte;\n\n");

    fprintf (header, "long long int GLOBAL_CODES_VERSION = %lld;\n\n", version);

    fprintf (header, "enum funcCodes : byte\n{\n");

    char tempCommand [5] = "";

    for (size_t i = 1; i <= nOfStr; i++)
    {
        fscanf (readFile, "%s", tempCommand);

        if (strcmp (tempCommand, "HLT") == 0)
        {
            fprintf (header, "\tHLT = 0xF4,\n");
        }

        else
        {
            fprintf (header, "\t%s = %zu,\n", tempCommand, i);
        }
    }

    fprintf (header, "};\n\n");
    fseek (readFile, 0, SEEK_SET); 
    fprintf (header, "enum funcHash\n{\n");

    for (size_t i = 1; i <= nOfStr; i++)
    {
        fscanf (readFile, "%s", tempCommand);

        fprintf (header, "\t%s_HASH = %d,\n", tempCommand, *(int*) tempCommand);
    }

    fprintf (header, "};\n\n");
}


int CountHash (const char* command)
{
    return *(int*) command;
}