#define PRINT_TYPE "%lg"

#include "stack.h"

//TODO - проверка что указатель на дин память

ErrorCodes StackCtor (stack_t* stk, size_t capacity DEBUG(, const char* file, const char* name, const char* func, size_t line))
{
    assert (stk != nullptr);
    assert (capacity > 0);
    assert (stk->data == nullptr);

    stk->data = (stackElem_t*) calloc (capacity + 2, sizeof (stackElem_t*)) + 1;

    stk->size = 0;
    stk->capacity = capacity;

    for (size_t i = 0; i < stk->capacity; i++)
    {
        stk->data [i] = GLOBAL_STACK_POISON;
    }

    #ifdef ON_CANARY
    stk->data [-1] = GLOBAL_LEFT_DATA_FOPF;
    stk->data [stk->capacity] = GLOBAL_RIGHT_DATA_FOPF;

    stk->leftCanary  = GLOBAL_LEFT_STACK_FOPF;
    stk->rightCanary = GLOBAL_RIGHT_STACK_FOPF;
    #endif

    #ifdef ON_DBG
    stk->file = file;
    stk->line = line;
    stk->name = name;
    stk->func = func;
    #endif

    #ifdef ON_HASH
    stk->dataHash = DJB2_HashCount (stk);
    stk->struckHash = DJB2_HashStruct (stk);
    #endif

    ASSERT_STK_OK(stk, line);

    return StackIsOk;
}


int StackDestroy (stack_t* stk DEBUG(, size_t callLine))
{
    ASSERT_STK_OK(stk, callLine);

    for (size_t i = 0; i < stk->size;)
    {
        stk->data [--stk->size] = GLOBAL_STACK_POISON;
    }

    stk->data [0] = GLOBAL_STACK_POISON;

    #ifdef ON_CANARY
    stk->data [-1] = GLOBAL_STACK_POISON;
    stk->data [stk->capacity] = GLOBAL_STACK_POISON;
    #endif

    free (stk->data - 1);

    stk->data = nullptr;

    return 0;
}


ErrorCodes StackPush (stack_t* stk, stackElem_t pushingElem DEBUG(, size_t callLine))
{
    ASSERT_STK_OK(stk, callLine);

    if (stk->size == stk->capacity)
    {
        StkCpcUp (stk DEBUG(, callLine));
    }

    stk->data [stk->size++] = pushingElem;

    #ifdef ON_HASH
    stk->dataHash = DJB2_HashCount (stk);
    stk->struckHash = DJB2_HashStruct (stk);
    #endif

    ASSERT_STK_OK(stk, callLine);

    return StackIsOk;
}


ErrorCodes StackPop (stack_t* stk, stackElem_t* poppedElem DEBUG(, size_t callLine))
{
    ASSERT_STK_OK(stk, callLine);

    if (stk->size == 0)
    {
        *poppedElem = NAN;
        return StackUnderFlow;
    }

    if (stk->size <= stk->capacity / 2)
    {
        StkCpcDown (stk DEBUG(, callLine));
    }
    
    stackElem_t popped = stk->data [--stk->size];

    stk->data [stk->size] = GLOBAL_STACK_POISON;

    #ifdef ON_HASH
    stk->dataHash = DJB2_HashCount (stk);
    stk->struckHash = DJB2_HashStruct (stk);
    #endif

    ASSERT_STK_OK(stk, callLine);

    *poppedElem = popped;
    return StackIsOk;
}


ErrorCodes StkCpcUp (stack_t* stk DEBUG(, size_t callLine))
{
    ASSERT_STK_OK(stk, callLine);

    size_t oldCap = stk->capacity;

    #ifdef ON_CANARY
    stackElem_t* tempPtr = (stackElem_t*) realloc (--stk->data, ((stk->capacity * 1.5 + 1) + 2) * sizeof (stackElem_t));
    #else 
    stackElem_t* tempPtr = (stackElem_t*) realloc (--stk->data, (stk->capacity * 1.5 + 1) * sizeof (stackElem_t));
    #endif

    if (tempPtr == nullptr)
    {
        #ifdef ON_DBG
        StackDump (stk, ReallocationErr, __LINE__, callLine);
        #endif

        return ReallocationErr;
    }

    stk->capacity *= 1.5;
    stk->capacity++;

    #ifdef ON_CANARY
    stk->data = tempPtr + 1;
    stk->data [stk->capacity] = GLOBAL_RIGHT_DATA_FOPF;
    #else 
    stk->data = tempPtr;
    #endif

    for (size_t i = oldCap; i < stk->capacity; i++)
    {
        stk->data [i] = GLOBAL_STACK_POISON;
    }

    #ifdef ON_HASH
    stk->dataHash = DJB2_HashCount (stk);
    stk->struckHash = DJB2_HashStruct (stk);
    #endif

    ASSERT_STK_OK(stk, callLine);

    return StackIsOk; 
}


ErrorCodes StkCpcDown (stack_t* stk DEBUG(, size_t callLine))
{
    ASSERT_STK_OK(stk, callLine);

    #ifdef ON_CANARY
    stackElem_t* tempPtr = (stackElem_t*) realloc (--stk->data, (stk->capacity / 1.5 + 2) * sizeof (stackElem_t));
    #else 
    stackElem_t* tempPtr = (stackElem_t*) realloc (--stk->data, (stk->capacity / 1.5) * sizeof (stackElem_t));
    #endif

    if (tempPtr == nullptr)
    {
        #ifdef ON_DBG
        StackDump (stk, ReallocationErr, __LINE__, callLine);
        #endif

        return ReallocationErr;
    }

    stk->capacity /= 1.5;

    #ifdef ON_CANARY
    stk->data = tempPtr + 1;
    stk->data [stk->capacity] = GLOBAL_RIGHT_DATA_FOPF;
    #else 
    stk->data = tempPtr;
    #endif

    #ifdef ON_HASH
    stk->dataHash = DJB2_HashCount (stk);
    stk->struckHash = DJB2_HashStruct (stk);
    #endif

    ASSERT_STK_OK(stk, callLine);

    return StackIsOk;
}


ErrorCodes PrintStack (stack_t* const stk DEBUG(, size_t callLine))
{
    ASSERT_STK_OK(stk, callLine);

    printf ("Stack capacity = %zu, size = %zu, data is:\n", stk->capacity, stk->size);

    #ifdef ON_CANARY

    printf (" [%d]" PRINT_TYPE "\n", -1,  stk->data [-1]);

    for (size_t i = 0; i < stk->capacity + 1; i++)
    {
        if (i < stk->size)
        {
            printf ("*[%zu]" PRINT_TYPE "\n", i,  stk->data [i]);
        }

        else 
        {
            printf (" [%zu]" PRINT_TYPE "\n", i, stk->data [i]);
        }
    }

    #else

    for (size_t i = 0; i < stk->capacity; i++)
    {
        if (i < stk->size)
        {
            printf ("*[%zu]" PRINT_TYPE "\n", i, stk->data [i]);
        }

        else 
        {
            printf (" [%zu]" PRINT_TYPE "\n", i, stk->data [i]);
        }
    }

    #endif

    return StackIsOk;
}


#ifdef DEBUG


size_t StackOk (stack_t* stk, size_t assertLine, size_t checkingFuncCallLine)
{
    size_t errorsCode = 0;

    if (stk == nullptr)
    {
        errorsCode += StackPtrIsNullptr;
        StackDump (stk, errorsCode, assertLine, checkingFuncCallLine);
        return errorsCode;
    }

    if (stk->data == nullptr) 
    {
        errorsCode += DataInStackPtrIsNullptr;
        StackDump (stk, errorsCode, assertLine, checkingFuncCallLine);
        return errorsCode;
    }

    if (stk->capacity == 0 || stk->capacity > SIZE_MAX / 2) errorsCode += InvalidCapacityValue;

    if (stk->size > SIZE_MAX / 2) 
    {
        errorsCode += InvalidSizeValue;
        StackDump (stk, errorsCode, assertLine, checkingFuncCallLine);
        return errorsCode;
    }

    if (stk->size > stk->capacity) 
    {
        errorsCode += SizeBiggerCapacity;
        StackDump (stk, errorsCode, assertLine, checkingFuncCallLine);
        return errorsCode;
    }

    for (size_t i = 0; i < stk->size; i++)
    {
        if (CmpDouble (stk->data [i], GLOBAL_STACK_POISON))
        {
            errorsCode += InvalidElemValue;
            break;
        }
    }

    if (!CmpDouble (stk->data [stk->size], GLOBAL_STACK_POISON) && stk->size != stk->capacity) errorsCode += InvalidPoisonValue;

    #ifdef ON_CANARY
    if (!CheckCanary (stk->leftCanary, GLOBAL_LEFT_STACK_FOPF)) errorsCode += LeftStkCanErr;

    if (!CheckCanary (stk->rightCanary, GLOBAL_RIGHT_STACK_FOPF)) errorsCode += RightStkCanErr;

    if (!CheckCanary (stk->data [-1], GLOBAL_LEFT_DATA_FOPF)) errorsCode += LeftDataCanErr;

    if (!CheckCanary (stk->data [stk->capacity], GLOBAL_RIGHT_DATA_FOPF)) errorsCode += RightDataCanErr;
    #endif

    #ifdef ON_HASH
    if (stk->dataHash != DJB2_HashCount (stk)) errorsCode += InvalidDataHash;
    if (stk->struckHash != DJB2_HashStruct (stk)) errorsCode += InvalidStructHash;
    #endif

    if (errorsCode != 0)
    {
        StackDump (stk, errorsCode, assertLine, checkingFuncCallLine);
        return errorsCode;
    }

    return errorsCode;
}


#ifdef ON_CANARY

int CheckCanary (stackElem_t canary, stackElem_t trueValue)
{
    if (CmpDouble (canary, trueValue))
    {
        return 1;
    }

    return 0;
}

#endif


void StackDump (stack_t* stk, size_t errorsCode, size_t errLine, size_t checkingFuncCallLine)
{
    FILE* log = fopen ("logsFile.log", "w");

    assert (log != nullptr);

    if (errorsCode == StackPtrIsNullptr)
    {
        fprintf (log, "Ptr to stack checked in line %zu, function called in line %zu is nullptr\n", errLine, checkingFuncCallLine);
        fclose (log);
        return;
    }

    if (errorsCode == DataInStackPtrIsNullptr)
    {
        fprintf (log, "Ptr to data in stack checked in line %zu, function called in line %zu is nullptr\n", errLine, checkingFuncCallLine);
        fclose (log);
        return;
    }

    fprintf (log, "Error in stack \'%s\', [%p], crated in file \'%s\', function \'%s\', line %zu\n", stk->name, stk, stk->file, stk->func, stk->line);

    fprintf (log, "Capacity = %zu\nSize = %zu\nData [%p] is:\n{\n", stk->capacity, stk->size, stk->data);

    fprintf (log, " [%3d]\t" PRINT_TYPE "\tcanary\n", -1, stk->data [-1]);

    for (size_t i = 0; i < stk->capacity + 1; i++)
    {
        if (i < stk->size && i != stk->capacity)
        {
            fprintf (log, "*[%3zu]\t" PRINT_TYPE "\n", i, stk->data [i]);
        }

        else if (i == stk->capacity)
        {
            fprintf (log, " [%3zu]\t" PRINT_TYPE "\tcanary\n", i, stk->data [i]);
        }

        else 
        {
            fprintf (log, " [%3zu]\t" PRINT_TYPE "\tpoison\n", i, stk->data [i]);
        }
    }

    fprintf (log, "}\nError codes is:\n{\n");

    size_t count = 0;

    while (errorsCode != 0)
    {
        if (errorsCode % 2 != 0)
        {
            fprintf (log, "\t%zu\n", (size_t) 1 << count);
        }
        errorsCode >>= 1;
        count++;
    }

    fprintf (log, "}\nAssert line is %zu\nCall before error in line %zu\n", errLine, checkingFuncCallLine);

    #ifdef ON_HASH
    fprintf (log, "Struct hash = %zu, true hash = %zu\n", stk->struckHash, DJB2_HashStruct (stk));
    fprintf (log, "Data hash = %zu, true hash = %zu\n", stk->dataHash, DJB2_HashCount (stk));
    #endif

    fclose (log);
}

#endif


int CmpDouble (double first, double second)
{
    if (isnan (first))
    {
        if (isnan (second))
        {
            return 1;
        }

        return 0;
    }

    if (fabs (first - second) < ACURACY)
    {
        return 1;
    }

    return 0;
}


#ifdef ON_HASH

size_t DJB2_HashCount (stack_t* stk)
{
    size_t hash = 5381;

    char* dataStart = (char*) stk->data;
    size_t dataSize = stk->capacity * sizeof (stk->data [0]);

    for (size_t i = 0; i < dataSize; i++)
    {
        hash = (hash << 5) + hash + dataStart [i];
    }

    return hash;
}

size_t DJB2_HashStruct (stack_t* stk)
{
    size_t hashCopy = stk->struckHash;

    stk->struckHash = 0;

    size_t hash = 5381;

    char* dataStart = (char*) stk;
    size_t dataSize = sizeof (stack_t);

    for (size_t i = 0; i < dataSize; i++)
    {
        hash = (hash << 5) + hash + dataStart [i];
    }

    stk->struckHash = hashCopy;

    return hash;
}

#endif