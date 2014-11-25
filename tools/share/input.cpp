#include "HoistLoads.h"
#include <fstream>

uint32_t makeCacheAddr(uint32_t addr)
{
    return (addr >> 5) << 5;
}

unsigned long long  processTrace()
{
    unsigned long long  loadSize = 0;
    //unsigned storeSize = 0;
    Inst_info *inst = (Inst_info *)malloc(sizeof(Inst_info));    
    while(gzread(OrigTrace, (void*)inst, sizeof(Inst_info)) > 0)
    {
        Inst_info *II = (Inst_info*)malloc(sizeof(Inst_info));
        memcpy((void *)II, (void *)inst, sizeof(Inst_info));

        
            if(II->acc_heap_load && LoadInsts.count(makeCacheAddr(II->ld_vaddr1)) == 0)
            {
                // Assuming Byte  addresable memory. Hence EA used for matching is [3:0]  
                LoadInsts.insert(make_pair(makeCacheAddr(II->ld_vaddr1),II));
                loadSize += II->mem_read_size; 
            }
 
        
            if(II->acc_heap_load && LoadInsts.count(makeCacheAddr(II->ld_vaddr2)) == 0)
            {
                // Assuming Byte  addresable memory. Hence EA used for matching is [3:0]  
                LoadInsts.insert(make_pair(makeCacheAddr(II->ld_vaddr2),II));
                loadSize += II->mem_read_size; 
            }
         
            if(II->acc_heap_store && LoadInsts.count(makeCacheAddr(II->st_vaddr)) == 0)
            {
                 //Assuming Byte  addresable memory. Hence EA used for matching is [3:0]  
                LoadInsts.insert(make_pair(makeCacheAddr(II->st_vaddr),II));
                loadSize += II->mem_write_size;

            }


//        if(II->acc_heap_store)
//        {
//             if(StoreInsts.count(II->st_vaddr) == 0)
//            {
//                 //Assuming Byte  addresable memory. Hence EA used for matching is [3:0]  
//                StoreInsts.insert(make_pair(II->st_vaddr>>16,II));
//                storeSize += II->mem_write_size;
//
//            }
//        }

    }
    free(inst);
    return loadSize;
}


//----------------------------
void storeTrace( int i )
{

    string LdMemTraceFilename = string("mem_trace_") + to_string(i) + string(".out");
    //MemLoadFilename = gzopen(LdMemTraceFilename.c_str(), "wb");    
    ofstream myfile;
    myfile.open (LdMemTraceFilename);
    for(auto &dl : LoadInsts)
    {
        //gzwrite(MemLoadFilename, dl.second, sizeof(Inst_info));
        uint32_t  x = dl.first;
        myfile <<hex<<x<<dec<<endl;

    }
    
    myfile.close();
    LoadInsts.clear();
    //gzclose(MemLoadFilename);

//
//    string StMemTraceFilename = string("st_trace_") + to_string(i) + string(".out");
//    MemStoreFilename = gzopen(StMemTraceFilename.c_str(), "wb");    
//
//    for(auto &dl : StoreInsts)
//            {
//                gzwrite(MemStoreFilename, dl.second, sizeof(Inst_info));
//            }
//    
//    StoreInsts.clear();
//    gzclose(MemStoreFilename);


}

//----------------------------
void  countDatasharing(int i, int j)
{
    int data_sharing_count= 0;
    unsigned int memSize = 0;
//    Inst_info *inst_i = (Inst_info *)malloc(sizeof(Inst_info));    
//    while(gzread(trace_i, (void*)inst_i, sizeof(Inst_info)) > 0)
//    {
// 
//
//        Inst_info *inst_j = (Inst_info *)malloc(sizeof(Inst_info));    
//        while(gzread(trace_j, (void*)inst_j, sizeof(Inst_info)) > 0)
//        {
//            cout<<inst_i<<"\t"<<inst_j<<endl;
//            if (inst_i==inst_j)    
//                data_sharing_count++; 
//            //memSize += inst_j->mem_read_size; 
//        }
//        free(inst_j);
//    }

    string traceFilename_i = string("mem_trace_") + to_string(i) + string(".out");
    string traceFilename_i_1 = string("mem_trace_") + to_string(j) + string(".out");
    ifstream trace_i (traceFilename_i);
    ifstream trace_j (traceFilename_i_1);
    string line;
    string line2;
    //ifstream myfile (trace_i);
    if (trace_i.is_open() && trace_j.is_open())
    {
            while ( getline (trace_i,line) )
            {

                while(getline (trace_j,line2))
                {
                    if(line==line2) 
                        data_sharing_count++;
                                    
                }
                trace_j.seekg(0);
            }
    }

    trace_i.close();
    trace_j.close();
    
    cout <<"DATA sharing between " <<i <<"and "<<j << "=  "<<data_sharing_count<<"\n"<<endl;

    //free(inst_i);
}


int main()
{
       // Open trace.txt to find out the number of traces

    ifstream traceTxt("trace.txt",ios::in);
    if(!traceTxt.is_open())
    {
        cerr << "Could not open trace.txt\n";
        return 0;
    }

    int numTraces = 0;
    traceTxt >> numTraces;
    cout<<"num of traces "<<numTraces<<"\n";
    traceTxt.close();

    assert(numTraces > 2 && numTraces < 9 && "Need 1 DMA trace and 6 or less ACC traces");

    //cerr << "numTraces: " << numTraces << "\n";

    for(int i = 2; i < numTraces  ; i++)
    {
        string OrigFilename = string("trace_") + to_string(i) + string(".raw");
        OrigTrace = gzopen(OrigFilename.c_str(), "rb");
        

        if(!OrigTrace )
        {
            cerr << "Could not open trace files  " <<OrigFilename <<endl;
            return 0;
        }
        //unsigned long long  total_load_size =0;
        cout<< "Total load + Store size for i=" <<i<<" is "<<processTrace()<<endl;
        storeTrace(i);

        gzclose(OrigTrace);

    }

    //for(int i = 2; i < numTraces; i++)
    //{
        ////string traceFilename_i = string("mem_trace_") + to_string(i) + string(".out");
        ////trace_i = gzopen(traceFilename_i.c_str(), "rb");
        ////ifstream trace_i (traceFilename_i);
        //int j;
        //j=i+1;
        //// comparing 2->3  | 2-> 4  if numTrace=4
        //while(j< numTraces + 1)
        //{
        
            ////string traceFilename_i_1 = string("mem_trace_") + to_string(j) + string(".out");
            ////trace_j = gzopen(traceFilename_i_1.c_str(), "rb");
            ////ifstream trace_j (traceFilename_i_1);
            //countDatasharing(i,j);
            //j++;
            ////gzclose(trace_j);
            ////trace_j.close();
        //}
        ////gzclose(trace_i);
        ////trace_i.close();
////        string OldFilename = string("orig.") + OrigFilename;
////        system((string("mv ")+OrigFilename+string(" ")+OldFilename).c_str());
////        system((string("mv ")+NewFilename+string(" ")+OrigFilename).c_str());
    //}

    return 0;
}
