#include "stack.h"
#include "stack.cpp"

#include <cassert>
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
};


enum VerifyErr : size_t
{
    SignErr = 0,
    VersionErr = 1,
    NofCommandsErr = 2
};


size_t CountFileSize (char* const fileName);
byte* ReadFile (size_t* fileSize, FILE* file);
size_t VerifySign (byte* byteCode);
void ParseCommands (processor ProcessorStat);
void Push (stack_t* stk, stackElem_t elem);
void Add (stack_t* stk);
void Mult (stack_t* stk);
void Out (stack_t* stk);
void Pop (stack_t* stk, processor* ProcessorStat);
void Pshr (stack_t* stk, processor* ProcessorStat);
void Jmp (processor* spu);
int CmpForJmp (stack_t* stk, byte jmpType);


//TODO abort убрать
int main (int argc, char* argv [])
{
    if (argv [1] == nullptr)
    {
        char fileName [] = "bytes.txt";
        argv [1] = fileName;
    }

    FILE* code = fopen (argv [1], "r");

    processor ProcessorStat = {};

    StackCtorCall(ProcessorStat.returnAddress, 4)

    ProcessorStat.codeLen = CountFileSize (argv [1]);
    ProcessorStat.byteCode = ReadFile (&ProcessorStat.codeLen, code);

    if (!VerifySign (ProcessorStat.byteCode))
    {
        printf ("invalid sign\n");
        return 1;
    }

    size_t signSize = 24;

    ProcessorStat.byteCode += signSize;

    ParseCommands (ProcessorStat);

    printf ("program finished correctly\n");

    fclose (code);
    free (ProcessorStat.byteCode -= signSize);
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


size_t VerifySign (byte* byteCode)
{
    int sign = 'ANDR';
    int versInf = 'ANDR';

    if (sign != *(int*) byteCode)
    {
        return SignErr;
    }

    if (versInf != *((int*) byteCode + 1))
    {
        return VersionErr;
    }

    if (GLOBAL_CODES_VERSION != *((long long int*) byteCode + 1))
    {
        return VersionErr;
    }

    return 0;
}


void ParseCommands (processor ProcessorStat)
{
    ProcessorStat.ip = 0;

    stack_t valueStack = {};
    
    StackCtorCall(valueStack, 1);

    while (ProcessorStat.ip < ProcessorStat.codeLen)
    {
        switch (ProcessorStat.byteCode [ProcessorStat.ip])
        {
            case PUSH:
            {
                stackElem_t arg = *(stackElem_t*) (ProcessorStat.byteCode + ProcessorStat.ip + 1);
                Push (&valueStack, arg);
                ProcessorStat.ip += sizeof (stackElem_t);
                break;
            }

            case ADD:
            {
                Add (&valueStack);
                break;
            }

            case MULT:
            {
                Mult (&valueStack);
                break;
            }

            case OUT:
            {
                Out (&valueStack);
                break;
            }

            case HLT:
            {
                StackDestroyCall(valueStack);
                return;
            }

            case POP:
            {
                Pop (&valueStack, &ProcessorStat);
                break;
            }

            case PSHR:
            {
                Pshr (&valueStack, &ProcessorStat);
                break;
            }

            case JMP:
            {
                Jmp (&ProcessorStat);
                break;
            }

            case JA:
            {
                if (CmpForJmp (&valueStack, JA))
                {
                    Jmp (&ProcessorStat);
                }

                break;
            }

            case JB:
            {
                if (CmpForJmp (&valueStack, JB))
                {
                    Jmp (&ProcessorStat);
                }

                break;
            }

            case JAE:
            {
                if (CmpForJmp (&valueStack, JAE))
                {
                    Jmp (&ProcessorStat);
                }

                break;
            }

            case JBE:
            {
                if (CmpForJmp (&valueStack, JBE))
                {
                    Jmp (&ProcessorStat);
                }

                break;
            }

            case JE:
            {
                if (CmpForJmp (&valueStack, JE))
                {
                    Jmp (&ProcessorStat);
                }

                break;
            }

            case JNE:
            {
                if (CmpForJmp (&valueStack, JNE))
                {
                    Jmp (&ProcessorStat);
                }

                break;
            }
            

            default:
            {
                printf ("Invalid byte code, ip = %zu\n", ProcessorStat.ip);

                StackDestroyCall(valueStack);

                free (ProcessorStat.byteCode);
                abort ();
            }
        }

        ProcessorStat.ip++;
    }
}

//TODO - rework to processor struct
void Push (stack_t* stk, stackElem_t elem)
{
    ASSERT_STK_OK (stk, __LINE__)

    StackPushCall(*stk, elem)
}


void Add (stack_t* stk)
{
    ASSERT_STK_OK (stk, __LINE__)
    assert (stk->size > 1);

    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(*stk, first);
    StackPopCall(*stk, second);

    Push (stk, first + second);
}


void Mult (stack_t* stk)
{
    ASSERT_STK_OK (stk, __LINE__)
    assert (stk->size > 1);
    
    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(*stk, first);
    StackPopCall(*stk, second);

    Push (stk, first * second);
}


void Out (stack_t* stk)
{
    ASSERT_STK_OK (stk, __LINE__)
    assert (stk->size > 0);

    stackElem_t answer = GLOBAL_STACK_POISON;

    StackPopCall(*stk, answer);

    printf ("Your answer is: " PRINT_TYPE "\n", answer);
}

//TODO mult, dev
void Jmp (processor* spu)
{
    spu->ip = *(int*) (spu->byteCode + spu->ip + 1);
    spu->ip--;
}


void Pop (stack_t* stk, processor* ProcessorStat)
{
    ASSERT_STK_OK (stk, __LINE__)
    assert (stk->size > 0);

    stackElem_t popped = 0;
    StackPopCall(*stk, popped);

    byte arg = *(ProcessorStat->byteCode + ProcessorStat->ip + 1);

    ProcessorStat->registers [arg] = popped;
    ProcessorStat->ip++;
}


void Pshr (stack_t* stk, processor* ProcessorStat)
{
    ASSERT_STK_OK (stk, __LINE__)

    byte arg = *(ProcessorStat->byteCode + ProcessorStat->ip + 1);

    stackElem_t regValue = ProcessorStat->registers [arg];

    StackPushCall(*stk, regValue);
    ProcessorStat->ip++;
}

int CmpForJmp (stack_t* stk, byte jmpType)
{
    ASSERT_STK_OK (stk, __LINE__)
    assert (stk->size > 1);

    stackElem_t last = GLOBAL_STACK_POISON;
    stackElem_t previous = GLOBAL_STACK_POISON;

    StackPopCall(*stk, last);
    StackPopCall(*stk, previous);

    switch (jmpType)
    {
        case JA:
        {
            if (last > previous) return 0;
            return 1;
        }

        case JAE:
        {
            if (last >= previous) return 0;
            return 1;
        }

        case JB:
        {
            if (last < previous) return 0;
            return 1;
        }

        case JBE:
        {
            if (last <= previous) return 0;
            return 1;
        }

        case JE:
        {
            if (last == previous) return 0;
            return 1;
        }

        case JNE:
        {
            if (last != previous) return 0;
            return 1;
        }
    }
}