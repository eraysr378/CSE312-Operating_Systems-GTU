
#include <multitasking.h>
#include <helper.h>
using namespace myos;
using namespace myos::common;

Process ProcessManager::processTable[256];
BlockedList ProcessManager::blockedList = BlockedList();
ReadyQueue ProcessManager::readyQueue = ReadyQueue();
Process::Process()
{
    cpustate = (CPUState *)(stack + 4096 - sizeof(CPUState));
    pid = -1;
}

Process::Process(GlobalDescriptorTable *gdt, void entrypoint())
{
    cpustate = (CPUState *)(stack + 4096 - sizeof(CPUState));

    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;

    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = 0;

    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */

    // cpustate -> error = 0;

    // cpustate->esp = (uint32_t)cpustate;

    cpustate->eip = (uint32_t)entrypoint;      // instruction pointer
    cpustate->cs = gdt->CodeSegmentSelector(); // code segment

    // cpustate -> ss = (uint32_t)cpustate ;
    cpustate->eflags = 0x202;
}

common::uint32_t Process::GetPid()
{
    return pid;
}
Process::~Process()
{
}
void Process::Init(uint32_t cs, uint32_t eip, uint32_t pid, uint32_t ppid)
{
    cpustate = (CPUState *)(stack + 4096 - sizeof(CPUState));
    cpustate->eax = 0;
    cpustate->ebx = 0;
    cpustate->ecx = 0;
    cpustate->edx = 0;

    cpustate->esi = 0;
    cpustate->edi = 0;
    cpustate->ebp = (uint32_t)cpustate;

    // cpustate->error=0;

    cpustate->esp = 0;
    cpustate->eip = eip;
    cpustate->cs = cs;
    cpustate->eflags = 0x202;
    // cpustate->ss = (uint32_t)stack;
    this->pid = pid;
    this->ppid = ppid;
    this->state = Process::Ready;
    this->priority = 100;
}

CPUState *Process::GetCpuState()
{
    return cpustate;
}
ProcessManager::ProcessManager()
{
    numProcess = 0;
    currentProcess = -1;
}

void Process::SetState(State state)
{
    this->state = state;
}
void Process::SetPriority(uint32_t priority)
{
    this->priority = priority;
}
void Process::SetWaitpid(uint32_t pid)
{
    this->waitpid = pid;
}
uint32_t Process::GetWaitpid()
{
    return waitpid;
}
Process::State Process::GetState()
{
    return state;
}
int Process::GetPriority()
{
    return priority;
}

bool ProcessManager::IsAllProcessesTerminated()
{
    int count = 0;
    for (int i = 0; i < numProcess; i++)
    {
        if (processTable[i].state == Process::Terminated)
            count++;
    }
    if (count == numProcess)
        return true;
    else
        return false;
}

void printCpuState(CPUState *cpustate)
{
    printf("eax: ");
    printfHex32((cpustate->eax));
    printf("\n");
    printf("ebx: ");
    printfHex32((cpustate->ebx));
    printf("\n");
    printf("ecx: ");
    printfHex32((cpustate->ecx));
    printf("\n");
    printf("edx: ");
    printfHex32((cpustate->edx));
    printf("\n");
    printf("esi: ");
    printfHex32((cpustate->esi));
    printf("\n");
    printf("edi: ");
    printfHex32((cpustate->edi));
    printf("\n");
    printf("ebp: ");
    printfHex32((cpustate->ebp));
    printf("\n");
    printf("esp: ");
    printfHex32((cpustate->esp));
    printf("\n");
    printf("eip: ");
    printfHex32((cpustate->eip));
    printf("\n");
    printf("cs: ");
    printfHex32((cpustate->cs));
    printf("\n");
    printf("eflags: ");
    printfHex32((cpustate->eflags));
    printf("\n");
    printf("ss: ");
    printfHex32((cpustate->ss));
    printf("\n");
    printf("error: ");
    printfHex32((cpustate->error));
    printf("\n");
}
Process *ProcessManager::GetRunningProcess()
{
    return runningProcess;
}
Process *ProcessManager::GetProcessByPid(int pid)
{
    for (int i = 0; i < 256; i++)
    {
        if (processTable[i].pid == pid)
        {
            return &processTable[i];
        }
    }
    return nullptr;
}

void Process::PrintProcess()
{
    printf("| PID:");
    printInteger(pid);

    printf(" | PPID:");
    printInteger(ppid);

    printf(" | STATE:");
    if (state == Ready)
    {
        printf("Ready     ");
    }
    else if (state == Running)
    {
        printf("Running   ");
    }
    else if (state == Waiting)
    {
        printf("Waiting   ");
    }
    else if (state == Blocked)
    {
        printf("Blocked   ");
    }
    else if (state == Terminated)
    {
        printf("Terminated");
    }

    printf(" | PRIORITY:");
    printInteger(priority);
    printf("|\n");
}
ProcessManager::~ProcessManager()
{
}
void Process::CopyStack(common::uint8_t *stack)
{
    for (int i = 0; i < 4096; i++)
    {
        this->stack[i] = stack[i];
    }
}
void Process::CopyCpuState(CPUState *cpustate)
{
    this->cpustate->eax = cpustate->eax;
    this->cpustate->ebx = cpustate->ebx;
    this->cpustate->ecx = cpustate->ecx;
    this->cpustate->edx = cpustate->edx;
    this->cpustate->esi = cpustate->esi;
    this->cpustate->edi = cpustate->edi;
    this->cpustate->ebp = cpustate->ebp;
    this->cpustate->esp = cpustate->esp;
    this->cpustate->eip = cpustate->eip;
    this->cpustate->cs = cpustate->cs;
    this->cpustate->eflags = cpustate->eflags;
    this->cpustate->ss = cpustate->ss;
    this->cpustate->error = cpustate->error;
}
common::pid_t ProcessManager::CreateInitProcess(CPUState *cpustate)
{
    Process *process = &processTable[numProcess];
    // init's parent pid is considered as 0 because there is no parent
    process->Init(cpustate->cs, cpustate->eip, 0, 0);
    process->SetPriority(0);

    numProcess++;

    readyQueue.enqueue(process);

    return 0;
}
void Process::PrintStack()
{
    for (int i = 0; i < 4096; i++)
    {
        printInteger(stack[i]);
    }
}
void ProcessManager::UpdateWaitingProcesses()
{
    for (int k = 0; k < blockedList.size; k++)
    {
        if (blockedList.elementAt(k)->state != Process::Waiting)
        {
            continue;
        }
        uint32_t waitpid = blockedList.elementAt(k)->waitpid;
        // if waitpid parameter is 0, then check for any terminated child
        if (waitpid <= 0)
        {
            // skip init, it cannot be child
            for (int i = 1; i < numProcess; i++)
            {
                if (processTable[i].ppid == blockedList.elementAt(k)->pid && processTable[i].state == Process::Terminated)
                {
                    processTable[i].ppid = -1; // update ppid to not wait it again
                    blockedList.elementAt(k)->state = Process::Ready;
                    readyQueue.enqueue(blockedList.removeAt(k));
                }
            }
        }
        else if (waitpid > 0)
        {
            if (processTable[waitpid].state == Process::Terminated)
            {
                processTable[waitpid].ppid = -1;
                blockedList.elementAt(k)->state = Process::Ready;
                readyQueue.enqueue(blockedList.removeAt(k));
            }
        }
    }
}

common::uint32_t ProcessManager::AddProcess(CPUState *cpustate)
{

    if (numProcess >= 256)
        return 0;

    Process *parent = runningProcess;
    Process *child = &processTable[numProcess];

    uint8_t *stackpPtr = parent->stack;
    child->CopyStack(stackpPtr);

    child->Init(cpustate->cs, cpustate->eip, numProcess, parent->pid);

    child->CopyCpuState(cpustate);
    // collatz will have the lowest priority
    if (child->pid == 1)
    {
        child->SetPriority(150);
    }

    // child will continue from where parent left, so make cpu state point to same place but in child stack
    child->cpustate = (CPUState *)((uint32_t)child->stack + (uint32_t)cpustate - (uint32_t)parent->stack);
    // ebp should be updated as well because we are changing to child stack
    child->cpustate->ebp = ((uint32_t)child->stack + (uint32_t)cpustate->ebp - (uint32_t)parent->stack);
    child->cpustate->eip = cpustate->eip;
    child->cpustate->ebx = 0; // child return value will be 0 for fork

    numProcess++;

    return child->pid;
}

CPUState *ProcessManager::Schedule(CPUState *cpustate)
{

    clearScreen();
    printf("******************************SCHEDULING******************************\n");
    UpdateWaitingProcesses();

    if (readyQueue.size <= 0)
    {
        printf("|------------------PROCESS TABLE------------------|\n");
        for (int i = 0; i < numProcess; i++)
        {
            processTable[i].PrintProcess();
        }
        printf("|-------------------------------------------------|\n");

        return cpustate;
    }

    if (runningProcess == nullptr)
    {
        printf("running process null\n");
        runningProcess = readyQueue.dequeue();
        runningProcess->SetState(Process::Running);
        return runningProcess->cpustate;
    }

    runningProcess->cpustate = cpustate;
    if (runningProcess->state == Process::Running)
    {
        runningProcess->SetState(Process::Ready);
        readyQueue.enqueue(runningProcess);
        runningProcess = nullptr;
    }

    runningProcess = readyQueue.dequeue();
    runningProcess->SetState(Process::Running);
    printf("|------------------PROCESS TABLE------------------|\n");
    for (int i = 0; i < numProcess; i++)
    {
        processTable[i].PrintProcess();
    }
    printf("|-------------------------------------------------|\n");
    readyQueue.printQueue();
    return runningProcess->cpustate;
}
