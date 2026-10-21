#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>

#include "commands.h" //не трогать`

#define CommandCase(cmd, ...) \
                case cmd ## _HASH: \
                { \
                    byteCodes [codeCount++] = (cmd); \
                    __VA_ARGS__;\
                    break; \
                } 

#define HAVE_DBL_ARG ParseDouble (readFile, byteCodes, &codeCount, &commandNum);
#define HAVE_REG_ARG ParseRegArg (readFile, byteCodes, &codeCount);
#define HAVE_LBL_ARG ParseLabel  (readFile, byteCodes, &codeCount, labels, &commandNum);

size_t CountFileSize    (const char* const fileName);
byte* ReadFile          (size_t* fileSize, FILE* file);
size_t CountStrings     (byte* text, size_t textSize);
byte* CompileMain       (FILE* readFile, FILE* outFile, size_t nOfStr, size_t* codelen, int* labels);
byte* Translate         (FILE* readFile, size_t nOfStr, size_t* codelen, int* labels);
void ParseDouble        (FILE* readFile, byte* byteCodes, size_t* ip, size_t* commandNum);
void ParseLabel         (FILE* readFile, byte* byteCodes, size_t* ip, int* labels, size_t* commandNum);
void ParsePop           (FILE* readFile, byte* byteCodes, size_t* ip);
void ParsePshr          (FILE* readFile, byte* byteCodes, size_t* ip);
void ParseRegArg        (FILE* readFile, byte* byteCodes, size_t* ip);
void FWriteHeader       (FILE* byteCode, size_t nOfStr);

#define NOFLABELS 20
#define MAXCOMMANDLEN 5