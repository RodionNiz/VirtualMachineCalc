#include "calc.h"
#include "stack.h"
#include "raylib/raylib.h"
#include "stack.cpp"

const int screenWidth = 1500;
const int screenHeight = 900;
size_t GLOBAL_VRAM_SIZE = 1500;

int main (int argc, char* argv [])
{
    if (argc < 2)  
    {
        printf ("No file name");
        return 0;
    }

    InitWindow(screenWidth, screenHeight, "raylib [core] example - delta time");

    Show();

    const char* fileName = nullptr;
    if (argv [1] != nullptr) fileName = argv [1];
    else                     fileName = "bytes.bin";

    FILE* code = fopen (fileName, "r");

    processor Spu = {};
    SpuCtor (&Spu, fileName, code);

    size_t signSize = 0;
    size_t nOfStr = 0;

    if (VerifySign (Spu.byteCode, &signSize, &nOfStr))
    {
        printf ("invalid sign\n");
        return 1;
    }

    Spu.byteCode += signSize;
    EvalCommands (&Spu);

    while (!WindowShouldClose())
    {  
    }
    CloseWindow();

    printf ("program finished correctly\n");
    fclose (code);
    free (Spu.byteCode -= signSize);
}

//TODO - filename load program
int SpuCtor (processor* SPU, const char* codeName, FILE* code)
{
    StackCtorCall(SPU->returnAddress, 4)
    StackCtorCall(SPU->valueStack, 4)

    SPU->codeLen = CountFileSize (codeName);
    SPU->byteCode = ReadFile (&SPU->codeLen, code);

    byte* tempVram = (byte*) calloc (GLOBAL_VRAM_SIZE, sizeof (tempVram [0]));
    {
        printf ("vram allocation err\n");
        return 1;
    }

    SPU->Vram = tempVram;
    return 0;
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

    byte* tempText = (byte*) calloc (*fileSize + 1, sizeof (tempText [0]));
    if (tempText == nullptr)
    {
        printf ("reading err\n");
        return nullptr;
    }

    byte* text = tempText;

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
    Command ComList [] = {&Push, &Pshr, &Pop, &Add, &Mult, &Div, &Sqrt, &Out, &Jmp, &Branch, &Call, &Ret};

    while (Spu->ip < Spu->codeLen)
    {
        byte cmd = Spu->byteCode [Spu->ip];

        if      (cmd <= JMP)                 ComList [cmd - 1]         (Spu);
        else if (cmd >  JMP  && cmd <  CALL) ComList [JMP]             (Spu); //под номером JMP лежит Branch
        else if (cmd >= CALL && cmd != HLT)  ComList [JMP + cmd - JNE] (Spu);
        else if (cmd == HLT)                 return;

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
    Spu->ip += sizeof (stackElem_t);
}


void Add (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    assert (Spu->valueStack.size > 1);

    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(Spu->valueStack, first);
    StackPopCall(Spu->valueStack, second);

    StackPushCall(Spu->valueStack, first + second);
}


void Mult (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    assert (Spu->valueStack.size > 1);
    
    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(Spu->valueStack, first);
    StackPopCall(Spu->valueStack, second);

    StackPushCall(Spu->valueStack, first * second);
}


void Div (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    assert (Spu->valueStack.size > 1);
    
    stackElem_t first = GLOBAL_STACK_POISON;
    stackElem_t second = GLOBAL_STACK_POISON;

    StackPopCall(Spu->valueStack, first);
    StackPopCall(Spu->valueStack, second);

    StackPushCall(Spu->valueStack, second / first);
}


void Sqrt (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    assert (Spu->valueStack.size > 0);
    
    stackElem_t first = GLOBAL_STACK_POISON;

    StackPopCall(Spu->valueStack, first);

    StackPushCall(Spu->valueStack, sqrt (first));
}


void Out (processor* Spu)
{
    ASSERT_STK_OK (&(Spu->valueStack), __LINE__)
    assert (Spu->valueStack.size > 0);

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
    assert (Spu->valueStack.size > 0);

    stackElem_t popped = 0;
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
    StackPushCall(Spu->returnAddress, Spu->ip + sizeof (int) + 1);
    Jmp (Spu);
}


void Ret (processor* Spu)
{
    stackElem_t backAddres = 0;
    StackPopCall(Spu->returnAddress, backAddres)
    Spu->ip = (size_t) backAddres - 1;
}


//TODO - spu destroy


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
        case JA:  return (last > previous);
        case JAE: return (last > previous || fabs (last - previous) < ACURACY);
        case JB:  return (last < previous);
        case JBE: return (last < previous || fabs (last - previous) < ACURACY);
        case JNE: return (fabs (last - previous) > ACURACY);
        default: 
            printf ("unsupported jmp type, %d\n", __LINE__);
            return 0;
    }

    return 0;
}


void Show (byte* Vram)
{
    int pixInSquare = 10;

    BeginDrawing();
    ClearBackground(RAYWHITE);

    int maxX = screenWidth / pixInSquare;
    int maxY = screenHeight / pixInSquare;

    Color squareColor = {.r = 255, .g = 255, .b = 255, .a = 255};

    for (int y = 0; y < maxY; y++)
    {
        for (int x = 0; x < maxX; x++)
        {
            unsigned char tempColor = *(Vram + y * maxX + x);
            squareColor = {.r = tempColor, .g = tempColor, .b = tempColor};

            DrawRectangle(x, y, pixInSquare, pixInSquare, squareColor);
        }
    }

    EndDrawing();
}