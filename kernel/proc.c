#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include <limits.h>


struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

// default length of scheduling latency
int cfs_sched_latency = 100;

// max number of timeslices for a process per scheduling latency
int cfs_max_timeslice = 10;

// min number of timeslices for a process per scheduling latency
int cfs_min_timeslice = 1;

// indicate if the fair scheduler is the current scheduler, 0 by default
int cfs = 0;

// the process currently scheduled to run by the fair scheduler and is initialized to 0
struct proc *cfs_current_proc = 0;

// number of timeslices assigned to the above process
int cfs_proc_timeslice_len = 0;

// number of timeslices that the above process can still run
int cfs_proc_timeslice_left = 0;

// Nice to weight conversion table
int nice_to_weight[40] = {
    88761, 71755, 56483, 46273, 36291, /*for nice = -20, ..., -16*/
    29154, 23254, 18705, 14949, 11916, /*for nice = -15, ..., -11*/
    9548, 7620, 6100, 4904, 3906,      /*for nice = -10, ..., -6*/
    3121, 2501, 1991, 1586, 1277,      /*for nice = -5, ..., -1*/
    1024, 820, 655, 526, 423,          /*for nice = 0, ..., 4*/
    335, 272, 215, 172, 137,           /*for nice = 5, ..., 9*/
    110, 87, 70, 56, 45,               /*for nice = 10, ..., 14*/
    36, 29, 23, 18, 15,                /*for nice = 15, ..., 19*/
};

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Return the sum of all RUNNABLE processes
int weight_sum()
{

  int sum = 0;
  struct proc *current_proc;

  // Loop through all processes, check if runnable.
  //
  // If runnable, then add it's current weight using its nice value
  // to the sum.
  for (current_proc = proc; current_proc < &proc[NPROC]; current_proc++)
  {
    // Verify current proc is not null and that it is in a RUNNABLE state
    if (current_proc && current_proc->state == RUNNABLE)
    {
      // +20 since array is range 0-39, but nice values are range -20-19
      sum += nice_to_weight[current_proc->nice + 20];
    }
  }

  return sum;
}

// Return a RUNNABLE proc with the shortest vruntime,
// or 0 if none.
struct proc *shortest_runtime_proc()
{

  int minVRun = INT_MAX;
  struct proc *current_proc;
  struct proc *shortest_proc = 0;

  for (current_proc = proc; current_proc < &proc[NPROC]; current_proc++)
  {
    // Check if the current proc has a shorter vruntime than the current
    // min vruntime. If it is less, update the minVRuntime and set
    // this proc as the potential one to be chosen to run
    // Has to be a RUNNABLE proc.
    if (current_proc && current_proc->state == RUNNABLE && current_proc->vruntime < minVRun)
    {
      minVRun = current_proc->vruntime;
      shortest_proc = current_proc;
    }
  }

  // If null, means no RUNNABLE process found, return 0
  if (!shortest_proc)
  {
    return 0;
  }

  return shortest_proc;
}

// Function to update the caller's nice value
// if it is between -20 and 19.
// Return the value after the potential update
int sys_nice(int new_nice){
   // get the caller’s struct proc
  struct proc *p = myproc();

  argint(0, &new_nice);

  if(new_nice >= -20 && new_nice <= 19){
    p->nice = new_nice;
  }

  return p->nice;
}

// Starts the cfs by setting cfs to 1
int sys_startcfs(){
  printf("Start CFS\n");
  cfs = 1;
  return 1;
}

// Stops the cfs by setting cfs to 0
int sys_stopcfs(){
  printf("Stopped CFS\n");
  cfs = 0;
  return 1;
}

// Implementation of our CFS Scheduler
void cfs_scheduler(struct cpu *c)
{
  // initialize c->proc, which is the process to be run in the next timeslice
  c->proc = 0;
  // decrement the current process’ left timeslice
  cfs_proc_timeslice_left -= 1;
  if (cfs_proc_timeslice_left > 0 && cfs_current_proc->state == RUNNABLE)
  {
    // when the current process hasn’t used up its assigned timeslices and is runnable
    // it should continue to run the next timeslce
    c->proc = cfs_current_proc;
  }
  else if (cfs_proc_timeslice_left == 0 ||
           (cfs_current_proc != 0 && cfs_current_proc->state != RUNNABLE))
  {
    // when the current process uses up its timeslices or becomes not runnable
    // it should not be picked to run next and its vruntime should be updated
    int weight = nice_to_weight[cfs_current_proc->nice + 20]; // convert nice to weight
    int inc = (cfs_proc_timeslice_len - cfs_proc_timeslice_left) * 1024 / weight;
    // compute the increment of its vruntime according to CFS design
    if (inc < 1)
      inc = 1;                         // increment should be at least 1
    cfs_current_proc->vruntime += inc; // add the increment to vruntime
    // prints for testing and debugging purposes
    printf("[DEBUG CFS] Process %d used up %d of its assigned %d timeslices and is swapped out !\n",
           cfs_current_proc->pid,
           cfs_proc_timeslice_len - cfs_proc_timeslice_left,
           cfs_proc_timeslice_len);
  }
  if (c->proc == 0)
  {
    // to add:
    //(1) Call shortest_runtime_proc() to get the proc with the shorestest vruntime
    //(2) If (1) returns a valid process, set up cfs_current_proc, cfs_proc_timeslice_len,
    //  cfs_proc_timeslice_left and c->proc accordingly.
    //  Notes: according to CFS, a process is assigned with time slice of
    //  ceil(cfs_sched_latency * weight_of_this_process / weights_of_all_runnable_process)
    //  and the timeslice length should be in [cfs_min_timeslice, cfs_max_timeslice]
    //(3) If (1) returns 0, do nothing.

    struct proc *shortestRuntimeProc = shortest_runtime_proc();

    // If there is a runnable process, assign the values
    if (shortestRuntimeProc != 0)
    {

      cfs_current_proc = shortestRuntimeProc;

      // Helper variables for readability
      int weightSum = weight_sum();
      int schedLatencyTimesWeight = cfs_sched_latency * nice_to_weight[cfs_current_proc->nice + 20];

      // calculate timeslice len using the equation above
      cfs_proc_timeslice_len = schedLatencyTimesWeight / weightSum;

      // Use mod to determine if there is a remainder, meaning we should
      // add 1 to "round up" to account for the ceil() function.
      if(schedLatencyTimesWeight % weightSum > 0){
        cfs_proc_timeslice_len += 1;
      }

      // Check bounds for timeslice, if greater than max
      // set it to max.
      // If less than min, set timeslice len to min
      if (cfs_proc_timeslice_len > cfs_max_timeslice)
      {
        cfs_proc_timeslice_len = cfs_max_timeslice;
      }
      if (cfs_proc_timeslice_len < cfs_min_timeslice)
      {
        cfs_proc_timeslice_len = cfs_min_timeslice;
      }

      // On initalization, timeslice left should equal to total;
      cfs_proc_timeslice_left = cfs_proc_timeslice_len;

      c->proc = cfs_current_proc;
    }
    if (c->proc > 0)
    {
      // prints for testing and debugging purposes
      printf("[DEBUG CFS] Process %d will run for %d timeslices next!\n",
             c->proc->pid,
             cfs_proc_timeslice_len);
      // schedule c->process to run
      acquire(&c->proc->lock);
      c->proc->state = RUNNING;
      swtch(&c->context, &c->proc->context);
      release(&c->proc->lock);
    }
  }
}

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void procinit(void)
{
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == UNUSED)
    {
      goto found;
    }
    else
    {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if ((p->trapframe = (struct trapframe *)kalloc()) == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if (p->trapframe)
    kfree((void *)p->trapframe);
  p->trapframe = 0;
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
  p->swapcount = 0;
  p->nice = 0;  // Set nice to 0
  p->vruntime = 0; // Set vrtuntime to 0
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE,
               (uint64)trampoline, PTE_R | PTE_X) < 0)
  {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE,
               (uint64)(p->trapframe), PTE_R | PTE_W) < 0)
  {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
    0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
    0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
    0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
    0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;     // user program counter
  p->trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0)
  {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0)
    {
      return -1;
    }
  }
  else if (n < 0)
  {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
  {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p)
{
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++)
  {
    if (pp->parent == p)
    {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status)
{
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd])
    {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++)
    {
      if (pp->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if (pp->state == ZOMBIE)
        {
          // Found one.
          pid = pp->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                   sizeof(pp->xstate)) < 0)
          {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p))
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

// The original RR scheduler is moved to old_scheduler
void old_scheduler(struct cpu *c)
{
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == RUNNABLE)
    {
      // Switch to chosen process. It is the process's job
      // to release its lock and then reacquire it
      // before jumping back to us.
      p->state = RUNNING;
      c->proc = p;
      swtch(&c->context, &p->context);
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&p->lock);
  }
}

// The scheduler runs the original RR scheduler (if cfs==0) or our new fair scheduler (if cfs==1)
void scheduler(void)
{
  struct cpu *c = mycpu();
  c->proc = 0;
  for (;;)
  {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    if (cfs)
    {
      cfs_scheduler(c);
    }
    else
    {
      old_scheduler(c);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&p->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  p->swapcount++;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Function to get the current process'
// parent id and returns it.
uint64 sys_getppid(void)
{
  // Call myproc() to get current proc.
  // Get parent proc, then parent pid.
  struct proc *p = myproc();
  return p->parent->pid;
}

// Returns the number of child processes the current
// calling process has.
//
// If there are children, copies the
// children's pid to the user array
uint64 sys_getcpids(void)
{

  int child_pids[64];
  int number = 0;

  // get the caller’s struct proc
  struct proc *p = myproc();
  struct proc *current_proc;

  // Loop through the proc array (each process).
  // For each process, compare proc->parent->pid
  // with our current proc, p-pid.
  //
  // Any matches will determine that p is a parent
  // of the current proc.
  // Increment a count of children.
  for (current_proc = proc; current_proc < &proc[NPROC]; current_proc++)
  {

    // Check for validity (not null) & if child's parent pid matches current
    // proc ppid
    if (current_proc && current_proc->parent && current_proc->parent->pid == p->pid)
    {
      // Save the child proc pid to child_pids
      // and increment number
      child_pids[number] = current_proc->pid;
      number++;
    }
  }

  // get the argument (i.e., address of an array)
  // passed by the caller
  uint64 user_array;
  argaddr(0, &user_array);

  // copy array child_pids from kernel memory
  // to user memory with address user_array
  if (copyout(p->pagetable, user_array, (char *)child_pids, number * sizeof(int)) < 0)
  {
    return -1;
  }

  // TODO: return the number of child processes found.
  return number;
}

// Return the number of swaps for the
// the current process.
int sys_getswapcount(void)
{
  return myproc()->swapcount;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); // DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p != myproc())
    {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan)
      {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->pid == pid)
    {
      p->killed = 1;
      if (p->state == SLEEPING)
      {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int killed(struct proc *p)
{
  int k;

  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if (user_dst)
  {
    return copyout(p->pagetable, dst, src, len);
  }
  else
  {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if (user_src)
  {
    return copyin(p->pagetable, dst, src, len);
  }
  else
  {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [USED] "used",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  struct proc *p;
  char *state;

  printf("\n");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}
