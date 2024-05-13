#include <multitasking.h>
#include <helper.h>
using namespace myos;
namespace myos
{
    int BlockedList::size = 0;

    BlockedList::BlockedList()
    {
        size = 0;
    }
    void BlockedList::add(Process *process)
    {
        arr[size] = process;
        size++;
    }
    Process *BlockedList::elementAt(int index)
    {
        if (index < size && index >= 0)
        {
            return arr[index];
        }
        return nullptr;
    }
    Process *BlockedList::removeAt(int index)
    {
        // Check if index is out of bounds
        if (index < 0 || index >= size)
        {
            return nullptr; // Return nullptr if index is invalid
        }

        Process *removedProcess = arr[index]; // Store the process to be removed

        // Shift elements to the left to fill the gap
        for (int i = index; i < size - 1; i++)
        {
            arr[i] = arr[i + 1];
        }

        size--; // Decrement size after removal

        return removedProcess; // Return the removed process
    }

    void BlockedList::printList()
    {
        printf("***************QUEUE**************\n");
        printf("size:");
        printInteger(size);
        printf("\n");

        for (int i = 0; i < size; i++)
        {
            arr[i]->PrintProcess();
        }
        printf("****************END***************");
    }
}
