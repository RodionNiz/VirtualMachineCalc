#include "calc.h"
#include "stack.h"
#include "raylib/raylib.h"
#include "stack.cpp"
#include <cstdio>

const int GLOBAL_VRAM_SIZE = 6000;

int main (int argc, char* argv [])
{
    const char* fileName = ((argc - 1) >= 1) ? argv [1] : "bytes.bin";
    processor Spu = {};
    SpuCtor (&Spu, fileName);

    InitFuncAddress ();
    EvalCommands (&Spu);

    SpuDestroy (&Spu);
    printf ("program finished correctly\n");

    free (GLOBAL_COM_LIST);
}


int SpuCtor (processor* Spu, const char* codeName)
{
    StackCtorCall(Spu->returnAddress, 4)
    StackCtorCall(Spu->valueStack, 4)

    FILE* code = fopen (codeName, "r");
    Spu->codeLen = CountFileSize (codeName);
    Spu->RawData = ReadFile (&Spu->codeLen, code);
    fclose (code);

    size_t signSize = 0;
    size_t nOfStr = 0;
    if (VerifySign (Spu->RawData, &signSize, &nOfStr))
    {
        printf ("invalid sign\n");
        free (Spu->RawData);
        StackDestroyCall(Spu->valueStack)
        StackDestroyCall(Spu->returnAddress)
        return 1;
    }
    Spu->byteCode = Spu->RawData + signSize;
    Spu->codeLen -= signSize;

    byte* tempVram = (byte*) calloc (GLOBAL_VRAM_SIZE, sizeof (tempVram [0]));
    if (tempVram == nullptr)
    {
        printf ("vram allocation err\n");
        free (Spu->RawData);
        StackDestroyCall(Spu->valueStack)
        StackDestroyCall(Spu->returnAddress)
        return 1;
    }
    Spu->Vram = tempVram;

    return 0;
}


void SpuDestroy (processor* Spu)
{
    free (Spu->Vram);
    free (Spu->RawData);

    StackDestroyCall(Spu->valueStack);
    StackDestroyCall(Spu->returnAddress);
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


size_t VerifySign (byte* byteCode, size_t* signSize, size_t* nOfStr)
{
    int sign = 'ANDR';
    int versInf = 'SREV';
    int offset = 0;

    if (sign != *(int*) byteCode) return SignErr;
    offset += sizeof (sign);
    
    *signSize = *(size_t*) (byteCode + offset);
    offset += sizeof (*signSize);

    if (versInf != *(int*) (byteCode + offset)) return VersionErr;
    offset += sizeof (versInf);

    if (GLOBAL_CODES_VERSION != * (long long int*) ( byteCode + offset)) return VersionErr;
    offset += sizeof (GLOBAL_CODES_VERSION);

    *nOfStr = *(size_t*) (byteCode + offset);
    
    return 0;
}


void InitFuncAddress ()
{
    GLOBAL_COM_LIST = (Command*) calloc (HLT + 1, sizeof (GLOBAL_COM_LIST[0]));
    if (GLOBAL_COM_LIST == nullptr)
    {
        printf ("Func arr calloc err\n");
        return;
    }

    StdFuncAddr(PUSH);
    StdFuncAddr(POP);
    StdFuncAddr(ADD);
    StdFuncAddr(MULT);
    StdFuncAddr(DIV);
    StdFuncAddr(SQRT);
    StdFuncAddr(OUT);
    StdFuncAddr(JMP);
    StdFuncAddr(CALL);
    StdFuncAddr(RET);
    StdFuncAddr(INPT);
    StdFuncAddr(SHOW);
    
    CustomFuncAddr(JA,  &Branch);
    CustomFuncAddr(JAE, &Branch);
    CustomFuncAddr(JB,  &Branch);
    CustomFuncAddr(JBE, &Branch);
    CustomFuncAddr(JE,  &Branch);
    CustomFuncAddr(JNE, &Branch);
}

void EvalCommands (processor* Spu)
{
    Spu->ip = 0;
    while (Spu->ip < Spu->codeLen)
    {
        byte cmd = Spu->byteCode [Spu->ip];
        if (cmd == HLT) return;
        else if (GLOBAL_COM_LIST [cmd & 0x1F] != nullptr) GLOBAL_COM_LIST [cmd & 0x1F] (Spu);
        else
        {
            printf ("Invalid byte code, ip = %zu, code = %d\n", Spu->ip, Spu->byteCode [Spu->ip]);
            return;
        }

        Spu->ip++;
    }
}

struct FargsType 
{
    unsigned int fCode  : 5;
    unsigned int digit   : 1;
    unsigned int regName : 1;
    unsigned int label   : 1;
};

void PUSH_f (processor* Spu)
{
    stackElem_t arg = 0;
    FargsType* opcode = (FargsType*) (Spu->byteCode + Spu->ip);

    if (opcode->digit)
    {
        arg += *(stackElem_t*) (Spu->byteCode + Spu->ip + 1);
        Spu->ip += sizeof (arg);
    }

    if (opcode->regName)
    {
        byte regNum = *(Spu->byteCode + Spu->ip + 1);
        arg += Spu->registers [regNum];
        Spu->ip++;
    }

    if (opcode->label)  StackPushCall(Spu->valueStack, Spu->Vram [(size_t) arg])
    else                StackPushCall(Spu->valueStack, arg)
}


void ADD_f (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    if (Spu->valueStack.size <= 1)
    {
        printf ("Add is impossible\n");
        return;
    }

    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(Spu->valueStack, first);
    StackPopCall(Spu->valueStack, second);

    StackPushCall(Spu->valueStack, first + second);
}


void MULT_f (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    if (Spu->valueStack.size <= 1)
    {
        printf ("Mult is impossible\n");
        return;
    }
    
    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(Spu->valueStack, first);
    StackPopCall(Spu->valueStack, second);

    StackPushCall(Spu->valueStack, first * second);
}


void DIV_f (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    if (Spu->valueStack.size <= 1)
    {
        printf ("Div is impossible\n");
        return;
    }
    
    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(Spu->valueStack, first);
    StackPopCall(Spu->valueStack, second);

    StackPushCall(Spu->valueStack, second / first);
}


void SQRT_f (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    if (Spu->valueStack.size == 0)
    {
        printf ("Sqrt is impossible\n");
        return;
    }
    
    stackElem_t first = GLOBAL_STACK_POISON;
    StackPopCall(Spu->valueStack, first);
    StackPushCall(Spu->valueStack, sqrt (first));
}


void OUT_f (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    if (Spu->valueStack.size == 0)
    {
        printf ("Out is impossible\n");
        return;
    }

    stackElem_t answer = GLOBAL_STACK_POISON;
    StackPopCall(Spu->valueStack, answer);
    printf (PRINT_TYPE "\n", answer);
}


void JMP_f (processor* Spu)
{
    int newIp = *(int*) (Spu->byteCode + Spu->ip + 1);
    if (newIp >= Spu->codeLen)
    {
        printf ("Invalid jmp address\n");
        return;
    }
    Spu->ip = newIp;
    Spu->ip--;
}


void POP_f (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    stackElem_t poppedValue = GLOBAL_STACK_POISON;
    FargsType* opcode = (FargsType*) (Spu->byteCode + Spu->ip);

    if (opcode->label)
    {
        size_t index = 0;
        if (opcode->digit)
        {
            index += (size_t) *(stackElem_t*) (Spu->byteCode + Spu->ip + 1);
            Spu->ip += sizeof (stackElem_t);
        }

        if (opcode->regName)
        {
            byte regNum = *(Spu->byteCode + Spu->ip + 1);
            index += (size_t) Spu->registers [regNum];
            Spu->ip++;
        }

        StackPopCall(Spu->valueStack, poppedValue);
        Spu->Vram [index] = poppedValue;
    }

    else if (opcode->regName)
    {
        byte regNum = *(Spu->byteCode + Spu->ip + 1);
        StackPopCall(Spu->valueStack, poppedValue);
        Spu->registers [regNum] = poppedValue;
        Spu->ip++;
    }

    else 
    {
        printf ("invalid pop opcode\n");
    }
    
    ASSERT_STK_OK(&(Spu->valueStack), __LINE__);
}


void Branch (processor* Spu)
{
    if (CmpForJmp (&Spu->valueStack, Spu->byteCode [Spu->ip]))
        JMP_f (Spu);
    else
        Spu->ip += sizeof (int);
}


void CALL_f (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)

    StackPushCall(Spu->returnAddress, Spu->ip + sizeof (int) + 1);
    JMP_f (Spu);
}


void RET_f (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)

    stackElem_t backAddres = GLOBAL_STACK_POISON;
    StackPopCall(Spu->returnAddress, backAddres)
    Spu->ip = (size_t) backAddres - 1;
}


void INPT_f (processor* Spu)
{
    stackElem_t arg = GLOBAL_STACK_POISON;
    if (!scanf ("%lg", &arg))
    {
        printf ("Invalid input for \"IN\"\n");
        return;
    }

    StackPushCall(Spu->valueStack, arg);
}


int CmpForJmp (stack_t* stk, byte jmpType)
{
    ASSERT_STK_OK (stk, __LINE__)
    assert (stk->size > 1);

    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(*stk, first);
    StackPopCall(*stk, second);

    switch (jmpType)
    {
        case JA:  return (first > second);
        case JAE: return (first > second || fabs (first - second) < ACURACY);
        case JB:  return (first < second);
        case JBE: return (first < second || fabs (first - second) < ACURACY);
        case JE:  return (fabs (first - second) < ACURACY);
        case JNE: return (fabs (first - second) >= ACURACY);
        default: 
            printf ("unsupported jmp type, %d\n", __LINE__);
            return 0;
    }

    return 0;
}


void SHOW_f (processor* Spu)
{
    const int screenWidth = 1000;
    const int screenHeight = 600;

    int pixInSquare = 10;

    InitWindow(screenWidth, screenHeight, "circle");

    while (!WindowShouldClose())
    {  
        BeginDrawing();
        ClearBackground(RAYWHITE);

        Color squareColor = {.r = 255, .g = 255, .b = 255, .a = 255};

        int maxX = screenWidth / pixInSquare;
        int maxY = screenHeight / pixInSquare;
        for (int y = 0; y < maxY; y++)
        {
            for (int x = 0; x < maxX; x++)
            {
                unsigned char tempColor = 255 - *(Spu->Vram + y * maxX + x);
                squareColor = {.r = tempColor, .g = tempColor, .b = tempColor, .a = 255};
                DrawRectangle(x * pixInSquare, y * pixInSquare, pixInSquare, pixInSquare, squareColor);
            }
        }
        EndDrawing();
    }
    CloseWindow();
}
