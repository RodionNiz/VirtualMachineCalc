#include "asm.h"
#include <cmath>
#include <cstdio>
#include <cstring>

//TODO битовые сруктуры
//TODO пофиксить сигнатуру
int main (int argc, char* argv [])
{
    const char* readFileName = nullptr;
    if (argv [1] != nullptr) readFileName = argv [1];
    else                     readFileName = "bytes.txt";

    const char* writeFileName = nullptr;
    if (argv [2] != nullptr) writeFileName = argv [2];
    else                     writeFileName = "bytes.bin";
    
    FILE* textCodesFile = fopen (readFileName, "r");
    FILE* byteCodesFile = fopen (writeFileName, "w");

    size_t size = CountFileSize (readFileName);
    byte* text = ReadFile (&size, textCodesFile);
    if (text == nullptr)
    {
        return -1;
    }
    size_t nOfStr = CountStrings (text, size);

    size_t byteCodeLen = 0;
    int labels [NOFLABELS] = {};
    memset (labels, -1, sizeof (labels));

    byte* byteCode = CompileMain (textCodesFile, byteCodesFile, nOfStr, &byteCodeLen, labels);

    fwrite (byteCode, sizeof (byte), byteCodeLen, byteCodesFile);

    free (byteCode);
    free (text);
    fclose (textCodesFile);
    fclose (byteCodesFile);
}


size_t CountFileSize (const char* const fileName)
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
        printf ("reading err\n");
        return nullptr;
    }

    byte* text = tempText;

    *fileSize = fread (text, sizeof (text [0]), *fileSize, file);

    if (text == nullptr)
    {
        printf ("reading err\n");
        return nullptr;
    }

    if (*fileSize == 0)
    {
        printf ("size err\n");
        return nullptr;
    }

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
            if (text [count + 1] != ':') strCount++;
        }

        count++;
    }

    if (text [textSize - 1] != '\n') strCount++;

    return strCount;
}


byte* CompileMain (FILE* readFile, FILE* outFile, size_t nOfStr, size_t* codelen, int* labels)
{
    byte* byteCode = Translate (readFile, nOfStr, codelen, labels);
    if (byteCode == nullptr) return nullptr;

    FWriteHeader (outFile, nOfStr);

    byteCode = Translate (readFile, nOfStr, codelen, labels);
    if (byteCode == nullptr) return nullptr;

    return byteCode;
}


byte* Translate (FILE* readFile, size_t nOfStr, size_t* codelen, int* labels)
{
    fseek (readFile, 0, SEEK_SET);

    byte* tempByteCodes = (byte*) calloc (nOfStr * 8, sizeof (byte));
    if (tempByteCodes == nullptr)
    {
        printf ("byteCode calloc err\n");
        return nullptr;
    }

    byte* byteCodes = tempByteCodes;
    char tempCommand [MAXCOMMANDLEN] = "";

    size_t codeCount = 0;
    for (size_t commandNum = 0; commandNum < nOfStr; commandNum++)
    {
        if (fscanf (readFile, "%s", tempCommand) == 0) break;
        if (*tempCommand == 'J') tempCommand [3] = 0;

        if (*tempCommand == ':')
        {
            int labelNum = -1;
            sscanf (tempCommand + 1, "%d", &labelNum);

            if (labelNum < 0 || labelNum >= NOFLABELS)
            {
                printf ("invalid label num\n");
                break;
            }

            labels [labelNum] = codeCount;

            commandNum--;
        }

        else
        {
            switch (*(int*) tempCommand)
            {
                CommandCase(PUSH,   HAVE_DBL_ARG);
                CommandCase(ADD,);
                CommandCase(MULT,);
                CommandCase(DIV,);
                CommandCase(SQRT,);
                CommandCase(HLT,);
                CommandCase(OUT,);
                CommandCase(PSHR,   HAVE_REG_ARG);
                CommandCase(POP,    HAVE_REG_ARG);
                CommandCase(JA,     HAVE_LBL_ARG);
                CommandCase(JAE,    HAVE_LBL_ARG);
                CommandCase(JB,     HAVE_LBL_ARG);
                CommandCase(JBE,    HAVE_LBL_ARG);
                CommandCase(JE,     HAVE_LBL_ARG);
                CommandCase(JNE,    HAVE_LBL_ARG);
                CommandCase(JMP,    HAVE_LBL_ARG);
                CommandCase(CALL,   HAVE_LBL_ARG);
                CommandCase(RET,);
                CommandCase(SHOW,);
                CommandCase(PSHV,   HAVE_REG_ARG);
                CommandCase(POPV,   HAVE_REG_ARG);

                default:
                {
                    printf ("Syntaxes error! Compilation failed! %d, \'%s\', %zu\n", *(int*) tempCommand, tempCommand, commandNum);
                    return nullptr;
                }
            }
        }
    }

    *codelen = codeCount;

    return byteCodes;
}


void ParseDouble (FILE* readFile, byte* byteCodes, size_t* ip, size_t* commandNum)
{
    double arg = NAN;
    fscanf(readFile, "%lg", &arg);
    memcpy (byteCodes + *ip, &arg, sizeof (arg));
    *ip += sizeof (arg);
    (*commandNum)++;
}


void ParseLabel (FILE* readFile, byte* byteCodes, size_t* ip, int* labels, size_t* commandNum)
{
    char arg [MAXCOMMANDLEN] = "";
    fscanf (readFile, "%s", arg);

    int labelNum = -1;

    if (arg [0] == ':')
    {
        sscanf (arg + 1, "%d", &labelNum);
        memcpy (byteCodes + *ip, labels + labelNum, sizeof (labels [0]));
    }

    else 
    {
        sscanf (arg, "%d", &labelNum);
        memcpy (byteCodes + *ip, &labelNum, sizeof (labels [0]));
        (*commandNum)++;
    }

    if (labelNum < 0 || labelNum >= 20)
    {
        printf ("invalid label num\n");
        return;
    }

    *ip += sizeof (labels [0]);
}


void ParseRegArg (FILE* readFile, byte* byteCodes, size_t* ip)
{
    char tempCommand [3] = "";
    fscanf (readFile, "%s", tempCommand);
    byteCodes [*ip] = tempCommand [0] - 'A';

    (*ip)++;
}


void FWriteHeader (FILE* byteCode, size_t nOfStr)
{
    byte* headerBuffer = (byte*) calloc (20, sizeof (headerBuffer [0]));
    if (headerBuffer == nullptr)
    {
        printf ("Calloc err in fwrite\n");
        return;
    }

    int sign = 'ANDR';
    int version = 'SREV';
    size_t signSize = sizeof (sign) + sizeof (signSize) + sizeof (version) + sizeof (GLOBAL_CODES_VERSION);

    fwrite (&sign, sizeof (sign), 1, byteCode);
    fwrite (&signSize, sizeof (signSize), 1, byteCode);
    fwrite (&version, sizeof (version), 1, byteCode);
    fwrite (&GLOBAL_CODES_VERSION, sizeof (GLOBAL_CODES_VERSION), 1, byteCode);
}