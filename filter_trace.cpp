/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2016 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#include <iomanip>
#include <iostream>
#include <ftsream>

#include "pin.H"
#include "instlib.H"

using namespace INSTLIB;

// Contains knobs to filter out things to instrument
FILTER filter;

ofstream out;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "filter.out", "specify output file name");

INT32 Usage()
{
    cerr <<
        "This pin tool demonstrates use of FILTER to identify instrumentation points\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

VOID PrintRTN(ADDRINT target){
    RTN rtn = RTN_FindByAddress(target);
    out << RTN_Name(rtn) << endl; 
}


VOID Trace(TRACE trace, VOID * val)
{
    if (!filter.SelectTrace(trace))
        return;
    // RTN trace_rtn = TRACE_Rtn(trace);
    // if(RTN_Valid(trace_rtn)){
    //     out << RTN_Name(trace_rtn) <<endl;
    // }
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            out << INS_Disassemble(ins) << endl;
            // out << INS_IsCall(ins) << endl;
            // if (INS_IsCall(ins)){
            //     out << "Its a call" << endl;
            //}
            //if (INS_IsCall(ins) && !INS_IsDirectBranchOrCall(ins)){
            //    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(PrintRTN), IARG_BRANCH_TARGET_ADDR);
            //}

            if (INS_IsDirectBranchOrCall(ins))
            {
                // Is this a tail call?
                //RTN rtn = RTN_FindByAddress(INS_DirectBranchOrCallTargetAddress(ins));
                //out << RTN_Name(rtn) << endl;
            }
        }
    }
}

VOID InstrumentImage(IMG img, VOID *v)
{
    out << IMG_Name(img) << endl;
    for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)){
        for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)){
            out << "Rtn: " << RTN_Name(rtn) << endl;
        }
    }
	// RTN pthreadMutexRTN = RTN_FindByName(img, "pthread_rwlock_rdlock");
	// if(RTN_Valid(pthreadMutexRTN)){
	// 	RTN_Open(pthreadMutexRTN);
	// 	for (INS ins = RTN_InsHead(pthreadMutexRTN); INS_Valid(ins); ins = INS_Next(ins))
 //    	{
 //        // Insert a call to docount to increment the instruction counter for this rtn
 //        out << INS_Disassemble(ins) << endl;
 //    	}
 //    	RTN_Close(pthreadMutexRTN);
	// }

}

VOID InstrumentRTN(RTN rtn, VOID *v){
    out << RTN_Name(rtn) << endl ;
}

VOID Fini(INT32 code, VOID * junk)
{
    out.close();
}

// argc, argv are the entire command line, including pin -t <toolname> -- ...
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
    
    filter.Activate();

    PIN_AddFiniFunction(Fini, NULL);
    
    // Start the program, never returns
    PIN_StartProgram();
    return 0;
}
