#ifndef __BLOCKED_LIST_H
#define __BLOCKED_LIST_H

using namespace myos;
namespace myos
{
    class Process;
    class BlockedList
    {
    public:
        Process *elementAt(int index);
        Process *removeAt(int index);
        void printList();
        void add(myos::Process *process);
        BlockedList();

        myos::Process *arr[30];
        static int size;

    private:
    };
}
#endif