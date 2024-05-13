
#include <syscalls.h>
#include <helper.h>
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;

SyscallHandler::SyscallHandler(InterruptManager *interruptManager, uint8_t InterruptNumber)
    : InterruptHandler(interruptManager, InterruptNumber + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}

// void SyscallHandler::syscall_fork(){
//     Process process();
//     interruptManager->processManager->AddProcess(&process);
//     printf("process created\n");
// }
uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState *cpu = (CPUState *)esp;
    Process* curProcess = interruptManager->processManager->GetCurrentProcess();
    uint32_t child_pid;
    switch (cpu->eax)
    {
    case 1: // exit
        printf("exit called\n");
        curProcess->SetState(Process::Terminated);
        // schedule the next process
        esp = (uint32_t)interruptManager->processManager->Schedule(nullptr);
        break;
    case 2: // fork

        child_pid = interruptManager->processManager->AddProcess(cpu);
        printf("child process created\n");
        ((CPUState *)esp)->ebx = child_pid; // write child pid as return value for parent


        break;
    case 7: // waitpid
    printf("waitpid \n");
        // set the state of current process as waiting
        curProcess->SetState(Process::Waiting);

        // set the waitpid to determine the waited process
        curProcess->SetWaitpid((uint32_t) cpu->ebx);
        // reschedule 
        esp = (uint32_t)interruptManager->processManager->Schedule(cpu);
        break;
    case 11: // execve
        // change the instruction pointer
        cpu->eip = cpu->ecx;
        // reset base pointer
        cpu->ebp = (uint32_t)curProcess->GetCpuState();

        

        break;
    case 4:
        printf((char *)cpu->ebx);
        break;

    default:
        break;
    }

    return esp;
}
