#include "stack.h"
#include <cassert>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "commands.h" //не трогать

struct processor 
{
    size_t ip;
    byte* byteCode;
    size_t codeLen;
    stackElem_t registers [4];
    stack_t returnAddress;
    stack_t valueStack;
    byte* Vram;
};


enum VerifyErr : size_t
{
    SignErr = 0,
    VersionErr = 1,
    NofCommandsErr = 2
};


typedef void (*Command) (processor* Spu);

#define CMP_TYPE(...) __VA_ARGS__

int SpuCtor (processor* SPU, const char* codeName, FILE* code);
size_t CountFileSize (const char* fileName);
byte* ReadFile (size_t* fileSize, FILE* file);
size_t VerifySign (byte* byteCode, size_t* signSize, size_t* nOfStr);
void EvalCommands (processor* ProcessorStat);
void Push   (processor* Spu);
void Add    (processor* Spu);
void Mult   (processor* Spu);
void Div    (processor* Spu);
void Sqrt   (processor* Spu);
void Out    (processor* Spu);
void Pop    (processor* Spu);
void Pshr   (processor* Spu);
void Jmp    (processor* Spu);
void Branch (processor* Spu);
void Call   (processor* Spu);
void Ret    (processor* Spu);
int CmpForJmp (stack_t* stk, byte jmpType);

void Show ();

int CmpForJmpf (stack_t* stk, CMP_TYPE(char* line));