#include <multitasking.h>
 #include <helper.h>
// #include <readyqueue.h>
using namespace myos;
namespace myos
{
    int ReadyQueue::size = 0;
    void myos::ReadyQueue::enqueue(myos::Process *process)
    {

        int index = 0;
        for (int i = 0; i < size; i++)
        {
            if (process->GetPriority() < arr[i]->GetPriority())
            {
                index = i + 1;
            }
        }
        size++;
        insert(process, index);
    }
    myos::Process *myos::ReadyQueue::dequeue()
    {
        size--;
        return arr[size];
    }
    void myos::ReadyQueue::insert(myos::Process *process, int pos)
    {
        if(size == 1){
            arr[0] = process;
            return;
        }

        for (int i = size - 1; i > pos; i--)
        {
            arr[i] = arr[i - 1];
        }

        arr[pos] = process;
    }
    ReadyQueue::ReadyQueue(){
        size = 0;
    }
    void ReadyQueue::printQueue()
    {
        printf("***************QUEUE**************\n");
        printf("size:");
        printInteger(size);
        printf("\n");

        for(int i = 0; i < size;i++){
            arr[i]->PrintProcess();
        }
        printf("****************END***************");

    }
}
