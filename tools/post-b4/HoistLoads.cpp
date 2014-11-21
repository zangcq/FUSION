#include "HoistLoads.h"

uint32_t makeCacheAddr(uint32_t addr)
{
    return (addr >> 5) << 5;
}

void processTrace(unsigned ScratchpadSize)
{
    Inst_info *inst = (Inst_info *)malloc(sizeof(Inst_info));    
    while(gzread(OrigTrace, (void*)inst, sizeof(Inst_info)) > 0)
    {
        Inst_info *II = (Inst_info*)malloc(sizeof(Inst_info));
        memcpy((void *)II, (void *)inst, sizeof(Inst_info));

        if(II->acc_heap_load)
        {
            if(LoadInsts.count(makeCacheAddr(II->ld_vaddr1)) == 0)
            {
                LoadInsts.insert(make_pair(makeCacheAddr(II->ld_vaddr1),II));
                CacheBlocks.insert(makeCacheAddr(II->ld_vaddr1));
            }
        }

        // Other insts including the load insts themselves
        OtherInsts.push_back(II);

        bool SegDrain = II->acc_segment_delim;

        // If ScratchpadSize is exceeded, then drain
        if(CacheBlocks.size()*32 == ScratchpadSize || SegDrain )
        {
            // Write out the load instructions
            for(auto &l : LoadInsts)
            {
                gzwrite(NewTrace, l.second, sizeof(Inst_info));
            }
            LoadInsts.clear();

            // Write out the other instructions 
            for(auto &i : OtherInsts)
            {
                gzwrite(NewTrace, i, sizeof(Inst_info));
                free(i);
            }
            OtherInsts.clear();

            // Add acc_window delim if segment delim not present
            if(!SegDrain)
            {
                Inst_info t;
                memset((void*)&t, 0, sizeof(t));
                t.opcode = TR_NOP;
                t.acc_window_delim = true;
                gzwrite(NewTrace, &t, sizeof(t));
            }
            
            // Reset load size
            CacheBlocks.clear();
        }
    }
    free(inst);
}

int main(int argc, char *argv[])
{
    if(argc != 2)    
    {
        std::cerr << "Usage " << argv[0] << "<Scratchpad Size>" << std::endl;
        return 0; 
    }

    // Open trace.txt to find out the number of traces

    ifstream traceTxt("trace.txt",ios::in);
    if(!traceTxt.is_open())
    {
        cerr << "Could not open trace.txt\n";
        return 0;
    }

    int numTraces = 0;
    traceTxt >> numTraces;
    traceTxt.close();

    assert(numTraces > 2 && numTraces <= 8 && "Need 1 DMA trace and 6 or less ACC traces");

    //cerr << "numTraces: " << numTraces << "\n";

    for(int i = 2; i < numTraces; i++)
    {
        string OrigFilename = string("trace_") + to_string(i) + string(".raw");
        OrigTrace = gzopen(OrigFilename.c_str(), "rb");
        string NewFilename = string("new.trace_") + to_string(i) + string(".raw");
        NewTrace = gzopen(NewFilename.c_str(),"wb");

        if(!OrigTrace || !NewTrace)
        {
            cerr << "Could not open trace files" << endl;
            return 0;
        }

        unsigned ScratchpadSize = 0;
        istringstream(argv[1]) >> ScratchpadSize;

        assert(ScratchpadSize % 32 == 0 && "ScratchpadSize should be a multiple of 32");

        processTrace(ScratchpadSize);

        gzclose(OrigTrace);
        gzclose(NewTrace);

        string OldFilename = string("orig.") + OrigFilename;
        system((string("mv ")+OrigFilename+string(" ")+OldFilename).c_str());
        system((string("mv ")+NewFilename+string(" ")+OrigFilename).c_str());
    }

    return 0;
}