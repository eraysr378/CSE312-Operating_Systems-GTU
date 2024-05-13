#ifndef __READY_QUEUE_H
#define __READY_QUEUE_H

using namespace myos;
namespace myos
{
    class Process;
    class ReadyQueue
    {
    public:
        void printQueue();
        void enqueue(myos::Process *process);
        myos::Process* dequeue();
        void insert(myos::Process *process,int pos);
        ReadyQueue();
        myos::Process* removePid(int pid);
        myos::Process *arr[30];
        static int size ;
    private:
        
    };
}
#endif