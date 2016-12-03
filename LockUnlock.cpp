#include <iomanip>
#include <iostream>
#include <fstream>
#include <queue>
#include <list>
#include <algorithm>


#include "pin.H"
#include "instlib.H"

using namespace INSTLIB;

// Contains knobs to filter out things to instrument
FILTER filter;

#define CMPXCHG 111
#define EAX_REG 10
#define RFLAGS_REG 25

ofstream out;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "filter.out", "specify output file name");

typedef struct MemoryWrite_t {
    ADDRINT effective_address;
    UINT32 size;
} MemoryWrite;

std::queue<MemoryWrite *> memoryOperandsBefore;
std::queue<MemoryWrite *> memoryOperandsAfter;
std::list<ADDRINT> lockedAddrs;


int isEAX = 0;
int isZeroBefore = 0;
int isZeroAfter = 0;
int isOneAfter = 0;
int isOneBefore = 0;

INT32 Usage()
{
    cerr <<
        "This pin tool tries to find the locks and unlocks in a program.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

VOID PrintInstruction(INS ins){
    out << INS_Disassemble(ins) << endl;
}

VOID CheckEAX(ADDRINT value, REG _reg){
    if(REG_FullRegName(_reg)==EAX_REG && value==0){
        isEAX = 1;
    }
    
}

VOID SetUpMemoryParams(ADDRINT effective_address, UINT32 size) {
    MemoryWrite *memory = (MemoryWrite *) malloc(sizeof(MemoryWrite));
    memory->effective_address = effective_address;
    memory->size = size;
    memoryOperandsBefore.push(memory);
}

VOID RecordMemoryWriteBeforeINS() {
    ADDRINT value = 0;
    size_t res = 0;

    while (!memoryOperandsBefore.empty()) {
        MemoryWrite *memoryWrite = memoryOperandsBefore.front();
        memoryOperandsBefore.pop();
        memoryOperandsAfter.push(memoryWrite);

        res = PIN_SafeCopy(&value, (VOID *) memoryWrite->effective_address, memoryWrite->size);
        if (res == memoryWrite->size) {
            bool found = (std::find(lockedAddrs.begin(), lockedAddrs.end(), memoryWrite->effective_address) != lockedAddrs.end());
            if(value==0){
                isZeroBefore = 1;
            }
            if(value==1 && found){
                isOneBefore = 1;
            }
        }
    }
}

VOID RecordMemoryWriteAfterINS(INS ins) {
    ADDRINT value = 0;
    size_t res = 0;
    MemoryWrite *lockMemoryWrite;

    while (!memoryOperandsAfter.empty()) {
        MemoryWrite *memoryWrite = memoryOperandsAfter.front();
        memoryOperandsAfter.pop();

        res = PIN_SafeCopy(&value, (VOID *) memoryWrite->effective_address, memoryWrite->size);
        if (res == memoryWrite->size) {
            if(value==1){
                lockMemoryWrite = memoryWrite;
                isOneAfter = 1;
            }
            if(value==0){
                isZeroAfter = 1;
            }
        }
    }

    if(isEAX && isZeroBefore && isOneAfter){
        out << "Lock Detected" << endl;
        out << INS_Disassemble(ins) << endl;
        isEAX = 0;
        isZeroBefore = 0;
        isOneAfter = 0;
        lockedAddrs.push_front(lockMemoryWrite->effective_address);  
    }

    if(isOneBefore && isZeroAfter){
        out << "Unlocked" << endl;
        out << INS_Disassemble(ins) << endl;
    }
}

void check_lock(INS ins)
{
    isEAX = 0;
    isZeroBefore = 0;
    isOneAfter = 0;
    isZeroAfter = 0;
    isOneBefore = 0;
    UINT32 num_operands = INS_OperandCount(ins);
    UINT32 i;
    for (i = 0; i < num_operands; ++i) {
        if (INS_OperandWritten(ins, i)) {
            if (INS_OperandIsReg(ins, i)) {
                REG _reg = INS_OperandReg(ins, i);
                if (_reg != REG_INVALID() && _reg < REG_MM_BASE) {
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) CheckEAX, IARG_REG_VALUE, _reg, IARG_PTR, _reg, IARG_END);
                }
            }
            else if (INS_OperandIsMemory(ins, i)) {
                // Insert a call before to get the effective address and the size.
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) SetUpMemoryParams, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
                // Insert a call before to get the value before.
                INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) RecordMemoryWriteBeforeINS, IARG_PTR, ins, IARG_END);
                // Insert a call after to get the value written.
                INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR) RecordMemoryWriteAfterINS, IARG_PTR, ins, IARG_END);
            }
        } 
    }
}

// void check_unlock(INS ins)
// {
//     UINT32 num_operands = INS_OperandCount(ins);
//     UINT32 i;
//     for (i = 0; i < num_operands; ++i) {
//         if (INS_OperandIsMemory(ins, i)){

//         }
// }

VOID Trace(TRACE trace, VOID * val)
{
    if (!filter.SelectTrace(trace))
        return;

    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            if(INS_IsAtomicUpdate(ins)){
                check_lock(ins);
            }
        }
    }
}

VOID Fini(INT32 code, VOID * junk)
{
    out.close();
}

int main(int argc, char * argv[])
{

    PIN_InitSymbols();
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    out.open(KnobOutputFile.Value().c_str());

    TRACE_AddInstrumentFunction(Trace, 0);

    //RTN_AddInstrumentFunction(InstrumentRTN, 0);

    //IMG_AddInstrumentFunction(InstrumentImage, 0);
    
    PIN_AddFiniFunction(Fini, NULL);
    
    filter.Activate();

    // Start the program, never returns
    PIN_StartProgram();
    return 0;
}