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
    byte* RawData;
    size_t codeLen;
    stackElem_t registers [6];
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
Command* GLOBAL_COM_LIST = nullptr;

int SpuCtor     (processor* Spu, const char* codeName);
void SpuDestroy (processor* Spu);

size_t CountFileSize (const char* fileName);
byte* ReadFile       (size_t* fileSize, FILE* file);
size_t VerifySign    (byte* byteCode, size_t* signSize, size_t* nOfStr);

void InitFuncAdrArr  ();
void EvalCommands    (processor* ProcessorStat);

void PUSH_f   (processor* Spu);
void ADD_f    (processor* Spu);
void MULT_f   (processor* Spu);
void DIV_f    (processor* Spu);
void SQRT_f   (processor* Spu);
void OUT_f    (processor* Spu);
void POP_f    (processor* Spu);
void JMP_f    (processor* Spu);
void Branch   (processor* Spu);
void CALL_f   (processor* Spu);
void RET_f    (processor* Spu);
void INPT_f   (processor* Spu);
void SHOW_f   (processor* Spu);

int CmpForJmp (stack_t* stk, byte jmpType);

#define StdFuncAddr(cmd)\
    if ((cmd) >= HLT)\
    {\
        printf ("%s code too big, max code is %d", #cmd, HLT);\
        return;\
    }\
    GLOBAL_COM_LIST [(cmd)] = & cmd ## _f;

#define CustomFuncAddr(cmd, ...)\
    if ((cmd) >= HLT)\
    {\
        printf ("%s code too big, max code is %d", #cmd, HLT);\
        return;\
    }\
    GLOBAL_COM_LIST [(cmd)] = __VA_ARGS__;


    
