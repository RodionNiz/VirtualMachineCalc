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
                    byteCode [codeCount++] = (cmd); \
                    __VA_ARGS__;\
                    break; \
                } 

#define HAVE_DBL_ARG ParseDouble    (readFile, byteCode, &codeCount, &commandNum);
#define HAVE_REG_ARG ParseRegArg    (readFile, byteCode, &codeCount, &commandNum);
#define HAVE_LBL_ARG ParseLabel     (readFile, byteCode, &codeCount, labels, &commandNum);
#define HAVE_HRD_ARG ParseHardArg   (readFile, byteCode, &codeCount, &commandNum);

size_t CountFileSize    (const char* fileName);
byte* ReadFile          (size_t* fileSize, FILE* file);
size_t CountStrings     (byte* text, size_t textSize);
int CompileMain         (FILE* readFile, FILE* outFile, size_t nOfStr);
byte* Translate         (FILE* readFile, byte* byteCode, size_t nOfStr, size_t* codelen, int* labels);
void ParseDouble        (FILE* readFile, byte* byteCodes, size_t* ip, size_t* commandNum);
void ParseLabel         (FILE* readFile, byte* byteCodes, size_t* ip, int* labels, size_t* commandNum);
void ParsePop           (FILE* readFile, byte* byteCodes, size_t* ip);
void ParsePshr          (FILE* readFile, byte* byteCodes, size_t* ip);
void ParseRegArg        (FILE* readFile, byte* byteCodes, size_t* ip, size_t* commandNum);
void ParseHardArg       (FILE* readFile, byte* byteCodes, size_t* ip, size_t* commandNum);
void FWriteHeader       (FILE* byteCode);

const int N_OF_LABELS = 20;
const int MAX_COMMAND_LEN = 5;