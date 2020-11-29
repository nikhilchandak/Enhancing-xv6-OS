# Enhancing xv6 Operating System

## Overview

Various improvements have been made to the xv6 operating system such as the waitx, setPriority and ps syscall. Scheduling techniques such as FCFS, PBS, and MLFQ have also been implemented and a report is also compiled to compare performance of different scheudling algorithms. 

### Task 1

#### Waitx syscall


The fields ctime (CREATION TIME), etime (END TIME), rtime (calculates RUN TIME) & iotime (IO TIME) fields have been added to proc structure of proc.h file

In trap.c, when case `T_IRQ0 + IRQ_TIMER` is satisfied, then rtime and iotime are increased:

    if (myproc())
    {
      #ifndef MLFQ 
        if (myproc()->state == RUNNING)
          myproc()->rtime += 1;

        if (myproc()->state == SLEEPING)
          myproc()->iotime+=1;
      #endif 
    }
proc.c contains the actual waitx() system call. 

Code is same as wait() and does the following:
        
* Search for a zombie child of parent in the proc table.
* When the child was found , following pointers were updated :
* wtime= p->etime - p->ctime - p->rtime - p->iotime;
* rtime=p->rtime;

sysproc.c is just used to call waitx() which is present in proc.c. The sys_waitx() function in sysproc.c passes the parameters (rtime,wtime) to the waitx() of proc.c, just as other system calls do. 

##### Testing

To get the waiting time, and run time for some command for some command, say `ls`, run the following: 
`> make clean; make qemu-nox`
`$ time ls `

#### ps (user program)

A system call `getPinfo()` has been created which is called by the `ps.c` user program. It iterates over the processes table (`ptable`) and prints the required information for each (valid) process with appropriate values where not applicable.

### Task 2 - Scheduling Algorithms

All scheduling techiniques have been added to the `scheduler` function in `proc.c`. Add the flag `SCHEDULER` to choose between RR (default), FCFS, PBS, and MLFQ. This has been implemented in `Makefile`. See Report for comparison of different scheduling techniques. 


#### FCFS 

* Non preemptive policy so we removed the the yield() call in trap.c in case of FCFS.
* Iterate through the process table to find the process with min creation time.
* Check if the process found is runnable, if it is, execute it

#### PBS


* Assign default priority 60 to each entering process
* Find the minimum priority process by iterating through the process table (min priority number translates to maximum preference).
* To implement RR for same priority processes, iterate through all the processes again. Whichever has same priority as the min priority found, execute that. `yield()` is enabled for PBS in `proc.c()` so the process gets yielded out and if any other process with same priority is there, it gets executed next.
* After running the chosen process, it checks if it got yielded because of higher priority process coming into its way. If so, it breaks out otherwise it continues.
* `int set_priority(int newPriority, int pid)` has been implemented to change priority of a process with given `pid` to `newPriority` by looping over the processes table and finding the appropriate process. A user program `setPriority.c` has also been implemented to facilitate the same.

#### MLFQ 

* We declared 5 queues with different priorities based on time slices, i.e. 1, 2, 4, 8, 16 timer ticks.
* These queues contain runnable processes only.
* The add process to queue and remove process from queue functions take arguments of the process and queue number and make appropriate changes in the array (pop and push).
`int add_proc_to_q(struct proc *p, int q_no);`
`int remove_proc_from_q(struct proc *p, int q_no);`
* We add a process in a queue in userinit() and fork() and kill() functions in proc.c i.e. wherever the process state becomes runnable.
* wtime (waiting time in scheduler) has been added to proc structure and is incremented whenever a process is `RUNNABLE`.
* Aging is implemented by iterating through queues 1-4 and checking if any process has exceeded the age limit and subsequently moving it up in the queues.
* Next, we iterate over all the queues in order, increase the tick associated with that process and its number of runs.
* In the trap file, we check if the `ticksTillNow` of the process >= permissible ticks of the queue. If that's the case, we call yield and push the process to the next queue. Otherwise increment the ticks and let the process remain in queue.

### Report + Bonus

A pdf file -- Report.pdf -- has been created for report. 
Graphs have been plotted using Python and log files like have been created to facilitate them (mlfqLog.txt, values.txt, etc)

