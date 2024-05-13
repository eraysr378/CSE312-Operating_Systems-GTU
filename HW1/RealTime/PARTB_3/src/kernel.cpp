
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>
#include <helper.h>
#include <drivers/amd_am79c973.h>

// #define GRAPHICSMODE

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char *foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;

public:
    MouseToConsole()
    {
        uint16_t *VideoMemory = (uint16_t *)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }

    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);

        x += xoffset;
        if (x >= 80)
            x = 79;
        if (x < 0)
            x = 0;
        y += yoffset;
        if (y >= 25)
            y = 24;
        if (y < 0)
            y = 0;

        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }
};

void sysprintf(char *str)
{
    asm("int $0x80" : : "a"(4), "b"(str));
}
void sysexit()
{
    CanRun::canRun = false;

    asm("int $0x80" : : "a"(1));
}
void sysblock(){
    asm("int $0x80" : : "a"(99));
}
uint32_t sysfork()
{

    printf("sysfork called\n");
    uint32_t childPid;

    // system call fork and put the ebx register value into childpid (it will be 0 for child)
    asm("int $0x80" : "=b"(childPid) : "a"(2));
    printf("sysfork finished1\n");

    return childPid;
}
void sysexecve(void entrypoint())
{
    // set ecx register to entrypoint that points to the program that the process will run
    // the process will set its eip to ecx value and will be able to load the program
    asm("int $0x80" : : "a"(11), "c"(entrypoint));
}
void syswaitpid(uint32_t pid)
{
    asm("int $0x80" : : "a"(7), "b"(pid));
}
uint32_t rand(int min, int max)
{
    uint32_t clock_lo;
    int result = -1;
    while (result < min)
    {
        asm("rdtsc" : "=a"(clock_lo));
        result = clock_lo % max;
    }

    return result;
}
void linearSearch()
{
    int arr[] = {10, 20, 80, 30, 60, 50, 110, 100, 130, 170};
    int x = 130;
    int index = -1;
    for (int i = 0; i < 10; i++)
    {
        if(arr[i] == x){
            index = i;
            break;
        }
    }
     printf("Linear search result: ");

     if (index == -1)
     {
         printf("not found");
     }
     else
     {
         printInteger(index);
     }
     printf("\n");

     sysexit();
}
void selectionSort(int arr[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        // Find the minimum element in unsorted array
        int minIndex = i;
        for (int j = i + 1; j < n; ++j) {
            if (arr[j] < arr[minIndex]) {
                minIndex = j;
            }
        }
        // Swap the found minimum element with the first element
        int temp = arr[minIndex];
        arr[minIndex] = arr[i];
        arr[i] = temp;
    }
}
void binarySearch()
{
    int arr[] =  {10, 20, 80, 30, 60, 50, 110, 100, 130, 170};
    selectionSort(arr,10);
    int left = 0, mid = 0, right = 9;
    int key = 110, index = -1;

    while (left <= right)
    {
        mid = (left + right) / 2;

        if (arr[mid] == key)
        {
            index = mid;
            break;
        }
        else if (arr[mid] < key)
            left = mid + 1;
        else
            right = mid - 1;
    }
    printf("Binary search result: ");

    if(index == -1){
        printf("not found");

    }else{
        printInteger(index);
    }
    printf("\n");
    sysexit();
}
void longRunningProgram()
{
    int n = 10000;

    // for(;;);
    int result = 0;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            result += i * j;
        }
    }
    printf("Result:");
    printInteger(result);
    printf("\n");
    sysexit();
}
void collatz()
{
    for (int i = 2; i < 300; i++)
    {
        int n = i;

        printf("collatz sequence for ");
        printInteger(n);
        printf(":");
        while (n != 1)
        {
            if (n % 2 == 1)
                n = (3 * n) + 1;
            else
                n /= 2;

            printInteger(n);
            printf(",");
        }
        printf("\n");
    }
    sysexit();
}

void init()
{
    printf("init created\n");
    uint32_t pidArray[4];
    void (*programs[])() = {collatz,longRunningProgram,binarySearch,linearSearch};
    uint32_t pid;
    pid = sysfork();
    if (pid == 0)
    {
        sysexecve(collatz);
    }
    pidArray[0] = pid;

    sysblock();

    pid = sysfork();
    if (pid == 0)
    {
        sysexecve(longRunningProgram);
    }
    pidArray[1] = pid;
    pid = sysfork();
    if (pid == 0)
    {
        sysexecve(binarySearch);
    }
    pidArray[2] = pid;
    pid = sysfork();
    if (pid == 0)
    {
        sysexecve(linearSearch);
    }
    pidArray[3] = pid;
 
    // Wait for all children
    for (int i = 0; i < 4; i++)
    {
        syswaitpid(pidArray[i]);
        printf("pid terminated:");
        printInteger((int)pidArray[i]);
        printf("\n");
    }

    sysexit(); // after all children terminated, exit.
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

void printCpuState(CPUState *cpustate);
extern "C" void kernelMain(const void *multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello World! --- http://www.AlgorithMan.de\n");

    GlobalDescriptorTable gdt;

    ProcessManager processManager;
    InterruptManager interrupts(0x20, &gdt, &processManager);
    SyscallHandler syscalls(&interrupts, 0x80);

    Process processInit(&gdt, init);
    processManager.CreateInitProcess(processInit.GetCpuState());

    DriverManager drvManager;

    PrintfKeyboardEventHandler kbhandler;
    KeyboardDriver keyboard(&interrupts, &kbhandler);

    drvManager.AddDriver(&keyboard);

    MouseToConsole mousehandler;
    MouseDriver mouse(&interrupts, &mousehandler);

    drvManager.AddDriver(&mouse);

    drvManager.ActivateAll();

    interrupts.Activate();

    while (1)
        ;
}
