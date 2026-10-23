#include "calc.h"
#include "stack.h"
#include "raylib/raylib.h"
#include "stack.cpp"
#include <cstdio>

const int GLOBAL_VRAM_SIZE = 6000;

int main (int argc, char* argv [])
{
    if (argc < 2)  
    {
        printf ("No file name");
        return 0;
    }
    const char* fileName = nullptr;
    if (argv [1] != nullptr) fileName = argv [1];
    else                     fileName = "bytes.bin";

    processor Spu = {};
    SpuCtor (&Spu, fileName);

    EvalCommands (&Spu);

    SpuDestroy (&Spu);
    printf ("program finished correctly\n");
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
    text [*fileSize] = 0;

    return text;
}

//TODO - длинна сигнатуры, кол-во строк
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


void EvalCommands (processor* Spu)
{
    Spu->ip = 0;
    Command ComList [] = {&Push, &Pshr, &Pop, &Add, &Mult, &Div, &Sqrt, &Out, &Jmp, &Branch, &Call, &Ret, &Show, &Pshv, &Popv};

    while (Spu->ip < Spu->codeLen)
    {
        byte cmd = Spu->byteCode [Spu->ip];
        if      (cmd <= JMP)                 ComList [cmd - 1]         (Spu);
        else if (cmd >  JMP  && cmd <  CALL) ComList [JMP]             (Spu); //под номером JMP лежит Branch
        else if (cmd >= CALL && cmd != HLT)  ComList [JMP + cmd - JNE] (Spu);
        else if (cmd == HLT)
        {
            return;
        }
        else
        {
            printf ("Invalid byte code, ip = %zu, code = %d\n", Spu->ip, Spu->byteCode [Spu->ip]);
            return;
        }

        Spu->ip++;
    }
}


void Push (processor* Spu)
{
    stackElem_t arg = *(stackElem_t*) (Spu->byteCode + Spu->ip + 1);
    StackPushCall(Spu->valueStack, arg)
    Spu->ip += sizeof (arg);
}


void Add (processor* Spu)
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


void Mult (processor* Spu)
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


void Div (processor* Spu)
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


void Sqrt (processor* Spu)
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


void Out (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    if (Spu->valueStack.size == 0)
    {
        printf ("Out is impossible\n");
        return;
    }

    stackElem_t answer = GLOBAL_STACK_POISON;
    StackPopCall(Spu->valueStack, answer);
    printf ("Your answer is: " PRINT_TYPE "\n", answer);
}


void Jmp (processor* Spu)
{
    Spu->ip = *(int*) (Spu->byteCode + Spu->ip + 1);
    Spu->ip--;
}


void Pop (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    if (Spu->valueStack.size == 0)
    {
        printf ("Pop is impossible\n");
        return;
    }

    stackElem_t popped = GLOBAL_STACK_POISON;
    StackPopCall(Spu->valueStack, popped);
    byte arg = *(Spu->byteCode + Spu->ip + 1);
    Spu->registers [arg] = popped;
    Spu->ip++;
}


void Pshr (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)

    byte arg = *(Spu->byteCode + Spu->ip + 1);
    stackElem_t regValue = Spu->registers [arg];
    StackPushCall(Spu->valueStack, regValue);
    Spu->ip++;
}


void Branch (processor* Spu)
{
    if (CmpForJmp (&Spu->valueStack, Spu->byteCode [Spu->ip]))
        Jmp (Spu);
    else
        Spu->ip += sizeof (int);
}


void Call (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)

    StackPushCall(Spu->returnAddress, Spu->ip + sizeof (int) + 1);
    Jmp (Spu);
}


void Ret (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)

    stackElem_t backAddres = GLOBAL_STACK_POISON;
    StackPopCall(Spu->returnAddress, backAddres)
    Spu->ip = (size_t) backAddres - 1;
}


void Pshv (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)

    byte arg = *(Spu->byteCode + Spu->ip + 1);

    int vramIndex = (int)(Spu->registers [arg]);
    if (vramIndex >= GLOBAL_VRAM_SIZE)
    {
        printf ("Invalid vram address\n");
        return;
    }
    stackElem_t memValue = (stackElem_t) *(Spu->Vram + vramIndex);
    StackPushCall(Spu->valueStack, memValue);

    Spu->ip += sizeof (arg);
}


void Popv (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    if (Spu->valueStack.size == 0)
    {
        printf ("Popv is impossible\n");
        return;
    }

    byte arg = *(Spu->byteCode + Spu->ip + 1);
    int vramIndex = (int)(Spu->registers [arg]);
    if (vramIndex >= GLOBAL_VRAM_SIZE)
    {
        printf ("Invalid vram address\n");
        return;
    }
    stackElem_t popped = GLOBAL_STACK_POISON;
    StackPopCall(Spu->valueStack, popped)
    Spu->Vram [vramIndex] = (byte) popped;

    Spu->ip += sizeof (arg);
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


void Show (processor* Spu)
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

        DrawRectangle(10 * pixInSquare, 0 * pixInSquare, pixInSquare, pixInSquare, squareColor);

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