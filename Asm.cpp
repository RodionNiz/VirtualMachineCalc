#include "asm.h"
#include <cmath>
#include <cstdio>


//TODO посмотреть совместимость raylib и txlib
int main (int argc, char* argv [])
{
    const char* readFileName =  ((argc - 1) >= 1) ? argv [1] : "bytes.txt";
    const char* writeFileName = ((argc - 1) >= 2) ? argv [2] : "bytes.bin";
    
    FILE* textCodesFile = fopen (readFileName, "r");
    FILE* byteCodesFile = fopen (writeFileName, "w");

    size_t size = CountFileSize (readFileName);
    byte* text = ReadFile (&size, textCodesFile);
    if (text == nullptr)
    {
        return -1;
    }
    size_t nOfStr = CountStrings (text, size);
    free (text);

    if (CompileMain (textCodesFile, byteCodesFile, nOfStr))
    {
        printf ("Compilation err\n");
        return 1;
    }

    fclose (textCodesFile);
    fclose (byteCodesFile);
}


size_t CountFileSize (const char* fileName)
{
    assert (fileName != nullptr);
    struct stat fileStat = {};
    assert (stat (fileName, &fileStat) != -1);

    return  fileStat.st_size;
}


byte* ReadFile (size_t* fileSize, FILE* file)
{
    assert (file != nullptr);

    byte* text = (byte*) calloc (*fileSize + 1, sizeof (text [0]));
    if (text == nullptr)
    {
        printf ("reading err\n");
        return nullptr;
    }

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


int CompileMain (FILE* readFile, FILE* outFile, size_t nOfStr)
{
    const int MAX_ARG_SIZE = sizeof (double);
    byte* byteCode = (byte*) calloc (nOfStr * MAX_ARG_SIZE, sizeof (byte));
    if (byteCode == nullptr)
    {
        printf ("byteCode calloc err\n");
        return 1;
    }

    size_t byteCodeLen = 0;
    int labels [N_OF_LABELS] = {};
    memset (labels, -1, sizeof (labels));
    byteCode = Translate (readFile, byteCode, nOfStr, &byteCodeLen, labels);
    if (byteCode == nullptr) return 1;

    FWriteHeader (outFile);

    byteCode = Translate (readFile, byteCode, nOfStr, &byteCodeLen, labels);
    if (byteCode == nullptr) return 1;

    fwrite (byteCode, sizeof (byte), byteCodeLen, outFile);
    free (byteCode);
    return 0;
}


byte* Translate (FILE* readFile, byte* byteCode, size_t nOfStr, size_t* codelen, int* labels)
{
    fseek (readFile, 0, SEEK_SET);

    char tempCommand [MAX_COMMAND_LEN] = "";
    size_t codeCount = 0;

    for (size_t commandNum = 0; commandNum < nOfStr; commandNum++)
    {
        if (fscanf (readFile, "%s", tempCommand) == EOF) break;
        if (*tempCommand == 'J') tempCommand [3] = 0;
        if (*tempCommand == ':')
        {
            int labelNum = -1;
            sscanf (tempCommand + 1, "%d", &labelNum);
            if (labelNum < 0 || labelNum >= N_OF_LABELS)
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
                CommandCase(PUSH,   HAVE_HRD_ARG);
                CommandCase(ADD,);
                CommandCase(MULT,);
                CommandCase(DIV,);
                CommandCase(SQRT,);
                CommandCase(HLT,);
                CommandCase(OUT,);
                CommandCase(POP,    HAVE_HRD_ARG);
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
                CommandCase(INPT,);


                default:
                {
                    printf ("Syntaxes error! Compilation failed! %d, \'%s\', %zu\n", *(int*) tempCommand, tempCommand, commandNum);
                    return nullptr;
                }
            }
        }
    }

    *codelen = codeCount;
    return byteCode;
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
    char arg [MAX_COMMAND_LEN] = "";
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


void ParseRegArg (FILE* readFile, byte* byteCodes, size_t* ip, size_t* commandNum)
{
    char tempCommand [3] = "";
    fscanf (readFile, "%s", tempCommand);
    byteCodes [*ip] = tempCommand [0] - 'A';

    (*ip)++;
    (*commandNum)++;
}

struct FargsType 
{
    unsigned int opCode  : 5;
    unsigned int digit   : 1;
    unsigned int regName : 1;
    unsigned int label   : 1;
};

void ParseHardArg (FILE* readFile, byte* byteCodes, size_t* ip, size_t* commandNum)
{
    double valueArg = NAN;
    char regName [4] = "";

    getc (readFile);
    char tempChar = getc (readFile);

    if (tempChar == '[')   ((FargsType*) (byteCodes + *ip - 1))->label = 1;
    else                   ungetc (tempChar, readFile);

    if (fscanf (readFile, "%lg", &valueArg))
    {
        ((FargsType*) (byteCodes + *ip - 1))->digit = 1;
        memcpy (byteCodes + *ip, &valueArg, sizeof (valueArg));
        *ip += sizeof (valueArg);
        (*commandNum)++;

        if (fscanf (readFile, " + %[^]]", regName))
        {
            ((FargsType*) (byteCodes + *ip - 1 - sizeof (valueArg)))->regName = 1;
            byteCodes [*ip] = regName [0] - 'A';
            (*ip)++;
            (*commandNum)++;
        }
    }

    else if (fscanf(readFile, "%s", regName))
    {
        ((FargsType*) (byteCodes + *ip - 1))->regName = 1;
        byteCodes [*ip] = regName [0] - 'A';
        (*ip)++;
        (*commandNum)++;
    }

    else 
    {
        printf ("Invalid hard arg sintaxis\n");
        return;
    }

    tempChar = getc (readFile);
    if (tempChar != ']') ungetc (tempChar, readFile);
}


void FWriteHeader (FILE* byteCode)
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