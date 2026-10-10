#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>

#include "commands.h" //не трогать

size_t CountFileSize (char* const fileName);
byte* ReadFile (size_t* fileSize, FILE* file);
size_t CountStrings (byte* text, size_t textSize);
byte* ReCode (FILE* readFile, size_t nOfStr, size_t* codelen);
void FWriteHeader (FILE* byteCode);


//TODO битовые сруктуры
int main (int argc, char* argv [])
{
    if (argv [1] == nullptr)
    {
        char readFileName [] = "codes.txt";
        argv [1] = readFileName;
    } 

    if (argv [2] == nullptr)
    {
        char writeFileName [] = "bytes.txt";
        argv [2] = writeFileName;
    }

    FILE* textCodes = fopen (argv [1], "r");
    FILE* byteCodesFile = fopen (argv [2], "w");

    size_t size = CountFileSize ((char*) "codes.txt");
    
    byte* text = ReadFile (&size, textCodes);

    if (text == nullptr)
    {
        return -1;
    }

    size_t nOfStr = CountStrings (text, size);

    size_t byteCodeLen = 0;

    fseek (textCodes, 0, SEEK_SET); 
    byte* byteCode = ReCode (textCodes, nOfStr, &byteCodeLen);

    if (byteCode == nullptr)
    {
        return -1;
    }

    FWriteHeader (byteCodesFile);
    
    fwrite (byteCode, sizeof (byte), byteCodeLen, byteCodesFile);

    free (byteCode);
    free (text);
    fclose (textCodes);
    fclose (byteCodesFile);
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
        abort ();
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
        if (text [count] == '\n' || text [count] == ' ')
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

//TODO в начале файла писать структуру: 4 байта RDNA многосимвольная константа в '' = R * 256 в степени позиции, версии кодов проверять, кол-во команд, 
//NOTE - заголовок 
byte* ReCode (FILE* readFile, size_t nOfStr, size_t* codelen)
{
    byte* tempByteCodes = (byte*) calloc (nOfStr * 8, sizeof (byte));

    if (tempByteCodes == nullptr)
    {
        printf ("byteCode calloc err\n");
        abort ();
    }

    byte* byteCodes = tempByteCodes;
    
    char tempCommand [6] = "";

    //NOTE мб аргумент из тайпдефа стека
    double tempArg = 0;

    size_t codeCount = 0;

    for (size_t i = 0; i < nOfStr; i++)
    {
        if (fscanf (readFile, "%s", tempCommand) == 0)
        {
            break;
        }

        if (strcmp (tempCommand, "PUSH") == 0)
        {
            byteCodes [codeCount++] = PUSH;

            fscanf (readFile, "%lg", &tempArg);

            //TODO - memcpy
            
            *(double*) (byteCodes + codeCount) = tempArg;

            codeCount += sizeof (double);

            i++;
        }

        else if (strcmp (tempCommand, "ADD") == 0)
        {
            byteCodes [codeCount++] = ADD;
        }

        else if (strcmp (tempCommand, "OUT") == 0)
        {
            byteCodes [codeCount++] = OUT;
        }

        else if (strcmp (tempCommand, "HLT") == 0)
        {
            byteCodes [codeCount++] = HLT;
        }

        else
        {
            printf ("Syntaxes error! Compilation failed!\n");
            return nullptr;
        }
    }

    *codelen = codeCount;

    return byteCodes;
}


void FWriteHeader (FILE* byteCode)
{
    byte* headerBuffer = (byte*) calloc (20, sizeof (headerBuffer [0]));

    int sign = 'ANDR';
    long long int version = 'EDOC';

    fwrite ((char*) &sign, 1, 4, byteCode);
    
    fwrite ((char*) &version, 1, 4, byteCode);
}