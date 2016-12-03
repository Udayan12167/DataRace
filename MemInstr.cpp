#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <set>
#include <cmath>
#include <map>
#include <vector>
#include <algorithm>

#include "pin.H"
#include "portability.H"
using namespace std;

TLS_KEY tls_key;


map<THREADID, set<ADDRINT> > readSets;
map<THREADID, set<ADDRINT> > writeSets;

class ThreadLocalData
{
public:
	ThreadLocalData(THREADID tid);

    set<ADDRINT> readSet;
    set<ADDRINT> writeSet;
    PIN_LOCK threadlock;
    int inscount;
};

ThreadLocalData::ThreadLocalData(THREADID tid)
{
	inscount = 0;
	PIN_InitLock(&threadlock);
}

ThreadLocalData* getTLS(THREADID tid)
{
	ThreadLocalData* tld = static_cast<ThreadLocalData*>(PIN_GetThreadData(tls_key,tid));
	return tld;
}


inline VOID  MemoryWriteInstrumentation(THREADID threadid,
		ADDRINT effectiveAddr//efecctive addr = memory addr
		){
	ThreadLocalData *tld = getTLS(threadid);
	tld->writeSet.insert(effectiveAddr);
}

inline VOID  MemoryReadInstrumentation(THREADID threadid,
		ADDRINT effectiveAddr//efecctive addr = memory addr
		){
	ThreadLocalData *tld = getTLS(threadid);
	tld->readSet.insert(effectiveAddr);
}

VOID Trace(TRACE trace, VOID *v)
{
    for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl=BBL_Next(bbl))
    {
        for(INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins=INS_Next(ins))
        {
        	tld->inscount++;
            UINT32 memoryOperands = INS_MemoryOperandCount(ins);

            for (UINT32 memOp = 0; memOp < memoryOperands; memOp++)
            {

                // Note that if the operand is both read and written we log it once
                // for each.
                if (INS_MemoryOperandIsRead(ins, memOp))
                {
                    INS_InsertPredicatedCall(
							ins, IPOINT_BEFORE, (AFUNPTR)MemoryReadInstrumentation,
							IARG_THREAD_ID,
							IARG_MEMORYOP_EA, memOp,
							IARG_END);
                }

                if (INS_MemoryOperandIsWritten(ins, memOp))
                {
                    INS_InsertPredicatedCall(
							ins, IPOINT_BEFORE, (AFUNPTR)MemoryWriteInstrumentation,
							IARG_THREAD_ID,
							IARG_MEMORYOP_EA, memOp,
							IARG_END);
                }
            }
        }
    }
}

VOID ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    ThreadLocalData * tld = new ThreadLocalData(tid);
    PIN_SetThreadData(tls_key, tld, tid);
}


VOID ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 code, VOID *v)
{

	ThreadLocalData * tld = getTLS(tid);
	cout << tid << " contains:" << tld->inscount << endl;
    readSets[tid] = tld->readSet;
	writeSets[tid] = tld->writeSet;
    PIN_SetThreadData(tls_key, 0, tid);
}


VOID Fini(INT32 code, void *v){
	set<ADDRINT> s1;
	set<ADDRINT> s2;
   for( map<THREADID,set<ADDRINT> >::iterator ii=writeSets.begin(); ii!=writeSets.end(); ++ii)
    {
        for( map<THREADID,set<ADDRINT> >::iterator i2=writeSets.begin(); i2!=writeSets.end(); ++i2){
        	if((*i2).first!=(*ii).first){
        		set<ADDRINT> out;
        		s1 = (*i2).second;
        		s2 = (*ii).second;
        		set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(out, out.begin()));
        		cout << (*i2).first << " and " << (*ii).first << endl;
        		for (set<ADDRINT>::iterator i = out.begin(); i != out.end(); i++) {
   					cout << *i << ",";
				}
				cout <<endl;
        	}
        }
   }
}

INT32 Usage()
{
    cerr << "This tool calculates " << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}


int main(int argc, char *argv[])
{
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    // Initialize thread-specific data not handled by buffering api.
    tls_key = PIN_CreateThreadDataKey(0);
   
    // add an instrumentation function
    TRACE_AddInstrumentFunction(Trace, 0);

    // add callbacks
    PIN_AddThreadStartFunction(ThreadStart, 0);
    PIN_AddThreadFiniFunction(ThreadFini, 0);

    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
