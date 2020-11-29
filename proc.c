#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

struct proc* queue[MAX_QUEUES][NPROC] ;

int tailPtr[MAX_QUEUES] = {-1, -1, -1, -1, -1} ;
int maxQticks[MAX_QUEUES] = {1, 2, 4, 8, 16} ; 

void promoteProcess(struct proc* p) {
  acquire(&ptable.lock);
  p->promotionFlag = 1 ; 
  release(&ptable.lock);
}

void updateTime() {
  acquire(&ptable.lock);
  for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if(p->state == RUNNABLE) 
          p->wtime++ ; 
      else if(p->state == SLEEPING)
          p->iotime++ ; 
  }
  release(&ptable.lock);
}

void displayLog() {
  acquire(&ptable.lock);
  for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if(p->pid > 2 && p)
          cprintf("ToRead %d %d %d\n", ticks, p->pid, p->currentQueue);
  }
  release(&ptable.lock);
}

void increaseTicks(struct proc* pp) {
  acquire(&ptable.lock);
  pp->ticksTillNow++ ; pp->qTicks[pp->currentQueue]++ ; pp->rtime++ ; 

  // #ifdef PLOT
      // cprintf("ToRead %d %d %d\n", ticks, pp->pid, pp->currentQueue);
  // #endif 

  // for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
  //     if(p->state == RUNNABLE) 
  //         p->wtime++ ; 
  // }
  release(&ptable.lock);
}

int insertProc(struct proc* p, int whichQueue) {
  // todo

  for(int i = 0; i <= tailPtr[whichQueue]; i++)
    if(p->pid == queue[whichQueue][i]->pid)
        return -1 ; // already present in the queue 

  p->enterTick = ticks ;
  p->wtime = 0;
  p->ticksTillNow = 0 ; 
  p->currentQueue = whichQueue ;
  // insert at the end of queue 
  tailPtr[whichQueue]++ ; 
  queue[whichQueue][tailPtr[whichQueue]] = p ; 

  return 1; 
}

int removeProc(struct proc* p, int whichQueue) {
  // todo 
  int procIndex = -1 ;
  for(int i = 0; i <= tailPtr[whichQueue]; i++)
    if(p->pid == queue[whichQueue][i]->pid && procIndex == -1)
        procIndex = i ;

  // proc not found (so nothing to remove)
  if(procIndex < 0) return -1 ;

  // shift all the processes one step behind to adjust the queue 
  for(int i = procIndex; i < tailPtr[whichQueue]; i++)
      queue[whichQueue][i] = queue[whichQueue][i + 1] ; 

  // move tail pointer one back to remove (since process removed)
  tailPtr[whichQueue]-- ; 
  return  1;

}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->ctime = ticks ; // .... 

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  // new fields 
  p->rtime = 0 ;
  p->etime = 0 ; 
  p->iotime = 0;
  p->n_run = 0 ; 

  #ifdef PBS
      p->priority = DEFAULT_PRIORITY;
  #else 
      p->priority = -1 ;
  #endif

  #ifdef MLFQ 
      p->ticksTillNow = 0 ;
      p->currentQueue = 0 ; 
      p->enterTick = 0 ; 
      p->wtime = 0 ;
      p->promotionFlag = 0 ;
      for(int i = 0; i < MAX_QUEUES; i++)
        p->qTicks[i] = 0 ; 
  #else 
      p->ticksTillNow = -1 ;
      p->currentQueue = -1 ; 
      p->enterTick = -1 ; 
      p->wtime = -1 ; 
      p->promotionFlag = -1 ;
      for(int i = 0; i < MAX_QUEUES; i++)
        p->qTicks[i] = -1 ; 
  #endif 

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  #ifdef MLFQ 
      insertProc(p, 0); // whenever a new process is initialized, it is always assigned to higest priority queue 
  #endif 
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  #ifdef MLFQ 
      insertProc(np, 0); // same as userinit 
  #endif

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;
  curproc->etime = ticks ;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);

        // remove proc from its queue since it has to exit 
        #ifdef MLFQ 
            removeProc(p, p->currentQueue); 
        #endif 

        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

// CUSTOM FUNCTIONS:

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
waitx(int* wtime, int* rtime)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);

        // remove proc from its queue since it has to exit 
        #ifdef MLFQ 
            removeProc(p, p->currentQueue); 
        #endif 

        *rtime = p->rtime;
        // p->etime = ticks;
        *wtime = p->etime - p->ctime - p->rtime - p->iotime ;

        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int set_priority(int newPriority, int pid) {
  if(newPriority < 1 || newPriority > 100) return -1 ; // sanity check 

  // cprintf("Entered!!!\n");

  struct proc *reqProc = 0 ; int found = 0 ;
  for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      // cprintf("Looping\n");
      if(p->pid == pid) { // do i need to check if p->state == RUNNABLE ??
          reqProc = p ; found = 1 ; break ; 
          // cprintf("Woohoo!\n"); 
      }
  }

  if(found == 0) return -1 ; 
  // cprintf("Found\n");

  acquire(&ptable.lock);
  int old = reqProc->priority;
  reqProc->priority = newPriority;
    //  cprintf("Changed priority of process %d from %d to %d\n", pid, old, newPriority);
  release(&ptable.lock);

  if(old > newPriority) yield(); 
  return old;
}

int getPinfo(void) {
    static char *states[] = {
    [UNUSED]    "unused",
    [EMBRYO]    "embryo",
    [SLEEPING]  "sleep ",
    [RUNNABLE]  "runble",
    [RUNNING]   "run   ",
    [ZOMBIE]    "zombie"
    };
    struct proc *p;
    char *state;

    cprintf("\n");
    cprintf("PID\tPriority\tState\tr_time\tw_time\tn_run\tcur_q\tq0\tq1\tq2\tq3\tq4\tName\n");

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == UNUSED)
            continue;
        if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";

        cprintf("%d\t%d\t\t\t%s\t\t%d\t\t%d\t\t%d\t\t%d\t", p->pid, p->priority, state, p->rtime, p->wtime, p->n_run, p->currentQueue);
        for(int i = 0; i < MAX_QUEUES; i++)
          cprintf("%d\t", p->qTicks[i]);
        cprintf("%s", p->name);

        // #ifdef MLFQ
        //     if(p->ticksTillNow != (ticks - p->enterTick))
        //         cprintf(" ooops!!");
        // #endif 

        cprintf("\n");
    }
    return 0;
}

int displayStats(int pid) {
    cprintf("Time to display stats of process with pid = %d \n", pid);
    #ifdef DEFAULT
      cprintf("Running Round Robin (defautlt)\n");
    #elif MLFQ 
      cprintf("Running Multi-level feedback queue scheduler\n");
    #elif FCFS 
      cprintf("Running First come first serve scheduler\n");
    #elif PBS
      cprintf("Running Priority based scheduler\n");
    #endif 
    cprintf("Total ticks till now: %d\n", ticks);
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        // cprintf("Looping\n");
        if(p->pid == pid) {
            cprintf("Process with pid = %d has runtime = %d and number_of_runs = %d \n", pid, p->rtime, p->n_run);
            #ifdef MLFQ 
                cprintf("Queue no=%d\nThe ticks received in each queue are:\n",p->currentQueue);
                for(int i=0; i<MAX_QUEUES; i++)
                    cprintf("%d: %d\n", i, p->qTicks[i]);
            #endif
            return 0;
        }
    }
    return -1; 
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

#ifdef DEFAULT 
    struct proc *p;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      // int entering = ticks ;
      c->proc = p;
      switchuvm(p);
      p->n_run++ ; 
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      // int took = ticks - entering ;
      // cprintf("Time quanta: %d\n", took);
      c->proc = 0;
    }
#endif 
#ifdef FCFS
        struct proc *mintimeProc = 0;
        for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->state == RUNNABLE) {
                if (mintimeProc) {
                    if (p->ctime < mintimeProc->ctime)
                        mintimeProc = p;
                } else {
                    mintimeProc = p;
                }
            }
        }

        if(mintimeProc && mintimeProc->state == RUNNABLE) 
        {
          // T ?? what is T ?? 
          // #ifdef T
            // cprintf("Process %s with PID %d and start time %d running\n",mintimeProc->name, mintimeProc->pid, mintimeProc->ctime);
          // #endif
          struct proc* p ; 
          p = mintimeProc;

          c->proc = p;
          switchuvm(p);
          p->n_run++;
          p->state = RUNNING;

          swtch(&(c->scheduler), p->context);
          switchkvm();

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
        }
#endif
#ifdef PBS 
        int minimumPriority = DEFAULT_PRIORITY + 1 ;
        for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) 
            if (p->state == RUNNABLE) 
                if (p->priority < minimumPriority)
                    minimumPriority = p->priority;

        for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->state == RUNNABLE) {
                if (p->priority == minimumPriority) {

                    // Switch to chosen process.  It is the process's job
                    // to release ptable.lock and then reacquire it
                    // before jumping back to us.
                    c->proc = p;
                    switchuvm(p);
                    p->n_run++ ; 
                    p->state = RUNNING;
                    // cprintf("[PBSCHEDULER] pid %d on cpu %d (prio %d)\n",
                    //         p->pid, c->apicid, p->priority);
                    swtch(&(c->scheduler), p->context);

                    switchkvm();
                    // Processis done running for now.
                    // It should have changed its p->state before coming back.
                    c->proc = 0;

                    // Check if yielded because of higher priority process coming into my way
                    // exit if so 
                    int minimumPriority2 = DEFAULT_PRIORITY + 2 ;
                    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) 
                        if (p->state == RUNNABLE) 
                            if (p->priority < minimumPriority2)
                                minimumPriority2 = p->priority;

                    if (minimumPriority2 < minimumPriority) {
                        break;
                    }
                }
            }
        }
#endif 
#ifdef MLFQ 

      // AGING: check from 1 to 4 (since obv 0 can't be promotd)
      for(int i=1; i < MAX_QUEUES; i++)
      {
          for(int j=0; j <= tailPtr[i]; j++)
          {
              struct proc *p = queue[i][j]; 
              if(p->currentQueue != i) {
                  cprintf("Errroooooorrr!!!\n");
              }
              int age ;
              // age = ticks - p->enterTick;
              // age = p->etime - p->ctime - p->rtime - p->iotime ; // same as wtime in waitx 
              age = p->wtime ; 

              if(age > MAX_AGE)
              {
                  // cprintf("Process with pid %d has aged! Currently in queue %d \n", p->pid, i);
                  removeProc(p, i);
                  // #ifdef T
                  // cprintf("Process %d moved up to queue %d due to age time %d\n", p->pid, i-1, age);
                  // #endif
                  insertProc(p, i-1);
              }

          }
      }

      struct proc *p = 0;
      for(int i=0; i < MAX_QUEUES; i++)
      {
          if(tailPtr[i] >= 0)
          {
              // choose the first process from the first queue which is available 
              p = queue[i][0];
              removeProc(p, i);
              break;
          }
      }

      if(p != 0 && p->state == RUNNABLE)
      {
          // p->ticksTillNow++;

          // #ifdef T
            // cprintf("Scheduling %s with PID %d from Queue %d with current tick %d\n",p->name, p->pid, p->currentQueue, p->ticksTillNow);
          // #endif
          // p->qTicks[p->currentQueue]++;
          c->proc = p;
          switchuvm(p);
          p->state = RUNNING;
          p->n_run++;
          p->wtime = 0 ; 
          swtch(&c->scheduler, p->context);
          switchkvm();
          c->proc = 0;
          // cprintf("%d\n", p->state);

          if(p !=0 && p->state == RUNNABLE)
          {
            // reset current queue running time:
            // p->ticksTillNow = 0;

            if(p->promotionFlag == 1)
            {
                p->promotionFlag = 0;
                // promote the process to next queue (decrease priority)
                if(p->currentQueue != 4)
                  p->currentQueue++;
              
              // cprintf("Moving Process from Queue %d to Queue %d\n", old,p->currentQueue);
            }
            
            // cprintf("Adding Process %d to Queue %d\n",p->pid ,p->currentQueue);
            insertProc(p, p->currentQueue);

          }
      }
        
#endif 
    release(&ptable.lock);

  }

}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;

      // since a process may have given up the CPU to go to sleep, reinsert the process where it belongs with fresh start (enterTick)
      #ifdef MLFQ 
        p->enterTick = 0 ;
        insertProc(p, p->currentQueue);
      #endif 
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
        // CHECK MLFQ thing!!!!!!!!

        // since a process may have given up the CPU to go to sleep, reinsert the process where it belongs with fresh start (enterTick)
        #ifdef MLFQ 
          p->enterTick = 0 ;
          insertProc(p, p->currentQueue);
        #endif 

      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}




