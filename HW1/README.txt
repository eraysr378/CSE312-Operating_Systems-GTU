open the folder you want to test in terminal
for example RealTime/PARTA 
write "make clean && make mykernel.iso" command
you can test the OS in a virtual machine with the created iso file.

In Test folder, scheduling occurs when m key is pressed instead of occuring at each timer interrupt.
Also, when space bar is pressed, current process runs for one timer interrupt interval.

In RealTime folder, scheduling occurs at each timer interrupt, therefore things happen very fast
and it is hard to see what is happening.

