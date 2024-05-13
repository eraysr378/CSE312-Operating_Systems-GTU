 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>
#include <readyqueue.h>
#include <blockedlist.h>
// #include <readyqueue.h>

namespace myos
{
    
    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));
    
    
    class Process
    {
    friend class ProcessManager;
    public:
        enum State
        {
            Ready = 0,
            Running = 1,
            Terminated = 2,
            Waiting = 3,
            Blocked = 4
        };

        Process(GlobalDescriptorTable *gdt, void entrypoint());
        Process(CPUState* cpustate);
        Process();
        void Init(common::uint32_t cs, common::uint32_t eip,common::uint32_t pid,common::uint32_t ppid);
        ~Process();
        CPUState* GetCpuState();
        common::uint32_t GetPid();
        void CopyCpuState(CPUState* cpustate);
        void CopyStack(common::uint8_t *stack);
        void SetState(State state);
        void SetPriority(common::uint32_t priority);
        void SetWaitpid(common::uint32_t pid);
        common::uint32_t GetWaitpid();
        void PrintProcess();
        void PrintStack();
        State GetState();
        int GetPriority();

    private:
        
        common::uint8_t stack[4096]; // 4 KiB
        // State state;
        CPUState* cpustate;
        common::uint32_t pid;
        common::uint32_t ppid;
        common::uint32_t waitpid;
        State state;
        common::uint32_t priority;
        
    
    };
    
    
    class ProcessManager
    {
    private:
        static Process processTable[256];
        int numProcess;
        int currentProcess;
    public:
        Process* runningProcess = nullptr;
        static ReadyQueue readyQueue;
        static  BlockedList blockedList;
        ProcessManager();
        ~ProcessManager();
        common::uint32_t AddProcess(CPUState* cpustate);
        void FillTable();
        common::pid_t CreateInitProcess(CPUState* cpustate);
        common::pid_t CreateProcess(CPUState* cpustate);
        CPUState* Schedule(CPUState* cpustate);
        Process* GetRunningProcess();
        static Process* GetProcessByPid(int pid);
        bool IsAllProcessesTerminated();
        void UpdateWaitingProcesses();
    };
    
   
    
}


#endif