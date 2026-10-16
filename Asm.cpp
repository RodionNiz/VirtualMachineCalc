#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>

#include "commands.h" //не трогать

size_t CountFileSize (char* const fileName);
byte* ReadFile (size_t* fileSize, FILE* file);

size_t CountStrings (byte* text, size_t textSize);

byte* ReCode (FILE* readFile, size_t nOfStr, size_t* codelen, int* labels);

void ParseJmp (FILE* readFile, byte* byteCodes, size_t* ip, int* labels);

void ParsePop (FILE* readFile, byte* byteCodes, size_t* ip);
void ParsePshr (FILE* readFile, byte* byteCodes, size_t* ip);

void FWriteHeader (FILE* byteCode, size_t nOfStr);


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

    int labels [20] = {};

    memset (labels, -1, sizeof (labels));

    byte* byteCode = ReCode (textCodes, nOfStr, &byteCodeLen, labels);

    if (byteCode == nullptr)
    {
        return -1;
    }

    FWriteHeader (byteCodesFile, nOfStr);

    byteCode = ReCode (textCodes, nOfStr, &byteCodeLen, labels);

    if (byteCode == nullptr)
    {
        return -1;
    }
    
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
            if (text [count + 1] != ':')
            {
                strCount++;
            }
        }

        count++;
    }

    if (text [textSize - 1] != '\n')
    {
        strCount++;
    }

    return strCount;
}


byte* ReCode (FILE* readFile, size_t nOfStr, size_t* codelen, int* labels)
{
    fseek (readFile, 0, SEEK_SET);

    byte* tempByteCodes = (byte*) calloc (nOfStr * 8, sizeof (byte));

    if (tempByteCodes == nullptr)
    {
        printf ("byteCode calloc err\n");
        abort ();
    }

    byte* byteCodes = tempByteCodes;
    
    char tempCommand [5] = "";

    //NOTE мб аргумент из тайпдефа стека
    double tempArg = 0;

    size_t codeCount = 0;

    for (size_t commandNum = 0; commandNum < nOfStr; commandNum++)
    {
        if (fscanf (readFile, "%s", tempCommand) == 0)
        {
            break;
        }

        if (*tempCommand == ':')
        {
            int labelNum = -1;

            sscanf (tempCommand + 1, "%d", &labelNum);

            assert (labelNum >= 0);
            assert (labelNum < 20);

            labels [labelNum] = codeCount;
            printf ("%d", labels [labelNum]);

            commandNum--;
        }

        else
        {
            switch (*(int*) tempCommand)
            {
                case PUSH_HASH:
                {
                    byteCodes [codeCount++] = PUSH;
                    fscanf (readFile, "%lg", &tempArg);
                    //TODO - memcpy
                    *(double*) (byteCodes + codeCount) = tempArg;
                    codeCount += sizeof (double);

                    commandNum++;
                }

                case ADD_HASH:
                {
                    byteCodes [codeCount++] = ADD;
                    break;
                }

                case MULT_HASH:
                {
                    byteCodes [codeCount++] = MULT;
                    break;
                }

                case DIV_HASH:
                {
                    byteCodes [codeCount++] = DIV;
                    break;
                }

                case OUT_HASH:
                {
                    byteCodes [codeCount++] = OUT;
                    break;
                }

                case HLT_HASH:
                {
                    byteCodes [codeCount++] = HLT;
                    break;
                }

                case PSHR_HASH:
                {
                    ParsePshr (readFile, byteCodes, &codeCount);
                    commandNum++;
                    break;
                }

                case POP_HASH:
                {
                    ParsePop (readFile, byteCodes, &codeCount);
                    commandNum++;
                    break;
                }

                case JA_HASH: 
                {
                    byteCodes [codeCount++] = JA;
                    ParseJmp (readFile, byteCodes, &codeCount, labels);
                    commandNum++;
                    break;
                }

                case JAE_HASH: 
                {
                    byteCodes [codeCount++] = JAE;
                    ParseJmp (readFile, byteCodes, &codeCount, labels);
                    commandNum++;
                    break;
                }

                case JB_HASH: 
                {
                    byteCodes [codeCount++] = JB;
                    ParseJmp (readFile, byteCodes, &codeCount, labels);
                    commandNum++;
                    break;
                }

                case JBE_HASH: 
                {
                    byteCodes [codeCount++] = JBE;
                    ParseJmp (readFile, byteCodes, &codeCount, labels);
                    commandNum++;
                    break;
                }

                case JE_HASH: 
                {
                    byteCodes [codeCount++] = JE;
                    ParseJmp (readFile, byteCodes, &codeCount, labels);
                    commandNum++;
                    break;
                }

                case JNE_HASH: 
                {
                    byteCodes [codeCount++] = JNE;
                    ParseJmp (readFile, byteCodes, &codeCount, labels);
                    commandNum++;
                    break;
                }

                case JMP_HASH: 
                {
                    byteCodes [codeCount++] = JMP;
                    ParseJmp (readFile, byteCodes, &codeCount, labels);
                    commandNum++;
                    break;
                }

                default:
                {
                    printf ("Syntaxes error! Compilation failed! %d\n", *(int*) tempCommand);
                    return nullptr;
                }
            }
        }
    }

    *codelen = codeCount;

    return byteCodes;
}


void ParseJmp (FILE* readFile, byte* byteCodes, size_t* ip, int* labels)
{
    int labelNum = -1;

    fscanf (readFile, "%d", &labelNum);

    assert (labelNum >= 0);
    assert (labelNum < 20);
    
    memcpy(byteCodes + *ip, labels + labelNum, sizeof (labels [0]));

    *ip += sizeof (labels [0]);
}


void ParsePop (FILE* readFile, byte* byteCodes, size_t* ip)
{
    byteCodes [*ip] = POP;
    ++*ip;

    char tempCommand [5] = "";

    fscanf (readFile, "%s", tempCommand);

    if (strcmp (tempCommand, "AX") == 0)
    {
        byteCodes [*ip] = 0x0;
    }

    else if (strcmp (tempCommand, "BX") == 0)
    {
        byteCodes [*ip] = 0x1;
    }

    else if (strcmp (tempCommand, "CX") == 0)
    {
        byteCodes [*ip] = 0x2;
    }

    else if (strcmp (tempCommand, "DX") == 0)
    {
        byteCodes [*ip] = 0x3;
    }

    ++*ip;
}


void ParsePshr (FILE* readFile, byte* byteCodes, size_t* ip)
{
    byteCodes [*ip] = PSHR;
    ++*ip;

    char tempCommand [5] = "";

    fscanf (readFile, "%s", tempCommand);

    if (strcmp (tempCommand, "AX") == 0)
    {
        byteCodes [*ip] = 0x0;
    }

    else if (strcmp (tempCommand, "BX") == 0)
    {
        byteCodes [*ip] = 0x1;
    }

    else if (strcmp (tempCommand, "CX") == 0)
    {
        byteCodes [*ip] = 0x2;
    }

    else if (strcmp (tempCommand, "DX") == 0)
    {
        byteCodes [*ip] = 0x3;
    }

    ++*ip;
}


void FWriteHeader (FILE* byteCode, size_t nOfStr)
{
    byte* headerBuffer = (byte*) calloc (20, sizeof (headerBuffer [0]));

    int sign = 'ANDR';
    long long int version = 'SREV';

    fwrite ((char*) &sign, 1, 4, byteCode);
    
    fwrite ((char*) &version, 1, 4, byteCode);

    fwrite ((char*) &GLOBAL_CODES_VERSION, 1, 8, byteCode);

    fwrite ((char*) &nOfStr, 1, 8, byteCode);
}