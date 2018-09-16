# Multi-threadingAnalysis-Part1
An analysis of the efficiency of various locking mechanisms on additions and basic list operations. Timing measurements were performed and recorded in a csv file. This csv file was then used to generate plots for analysis purposes.

The following is a series of questions and answers regarding the analysis of the data shown in the plots.


Question 2.1.1 - causing conflicts:
Why does it take many iterations before errors are seen?
Why does a significantly smaller number of iterations so seldom fail?

It takes many iterations before errors occur because increasing the number of iterations increases
the time that each thread will take to execute, which increases the chance for race conditions to occur.

A small number of iterations seldom fails because threads have a greater chance of completing before
another thread is scheduled. It takes time for a thread to be created, and with a small number of
iterations, each thread may finish before the next runs. For a small number of iterations, there may not
be true parallelism because only one thread will ever be accessing the shared data at one time.


Question 2.1.2 - cost of yielding:
Why are the --yield runs so much slower?
Where is the additional time going?
Is it possible to get valid per-operation timings if we are using the --yield option?
If so, explain how. If not, explain why not.

The --yield runs are much slower because there are many extra context switches than in the previous case.
This is because each thread is being forced to yield the cpu to another thread, which incurs a context switch.

The additional time is taken up by context switches, which includes saving and restoring registers.

It is not possible to get valid per-operation timings with --yield, because a large amount of time is spent
doing context switches. Our program does not differentiate between the time spent doing operations and the
time spent doing context switches.


Question 2.1.3 - measurement errors:
Why does the average cost per operation drop with increasing iterations?
If the cost per iteration is a function of the number of iterations, how do we know how many iterations to run (or what the "correct" cost is)?

There is an overhead cost induced in thread creation which is constant regardless of the number
of iterations. Therefore, increasing iterations will result in this overhead cost contributing less to
the total cost per opertaion.

With a large enough number of iterations, the cost per opertaion will begin to level out. Mathematically,
this number is infinity, but in reality a very large number of iterations should well approximate the
asymptote at which the cost per operation levels out. This number can be determined experimentally by
increasing the number of iterations until the cost levels out.


Question 2.1.4 - costs of serialization:
Why do all of the options perform similarly for low numbers of threads?
Why do the three protected operations slow down as the number of threads rises?

For low numbers of threads, there is a lower probability of two threads attempting to access the critical
section at the same time, which means that the cost of locking is not too high.

As the number of threads increases, the probability of two threads attempting to acquire a lock at the same time
increases. Theerefore cost associated with each lock increases as more threads are stuck as they wait to acquire
the lock.


Question 2.2.1 - scalability of Mutex:
Compare the variation in time per mutex-protected operation vs the number of threads in Part-1 (adds) and Part-2 (sorted lists).
Comment on the general shapes of the curves, and explain why they have this shape.
Comment on the relative rates of increase and differences in the shapes of the curves, and offer an explanation for these differences.

The time per mutex operation increases as the number of threads increases in both the add and list program.
However, the time per mutex opertaion increases more rapidly for the list program because the critical
section requires more expensive operations in the list program than in the add program.

The plot for mutex-protected operations in both the add and the list program appears to be linear. This makes
sense because the number of threads waiting to acquire a lock is proportional to the amount of time that a thread
must wait to acquire the lock.

The list's plot has a much greater rate of increase than the add's plot because the critical section is more
expensive in the list program. For the add program, each critical section simply consists of incrementing or
decrementing a counter, while the critical sections in the list program include inserting and removing elements. 


Question 2.2.2 - scalability of spin locks:
Compare the variation in time per protected operation vs the number of threads for list operations protected by Mutex vs Spin locks.
Comment on the general shapes of the curves, and explain why they have this shape.
Comment on the relative rates of increase and differences in the shapes of the curves, and offer an explanation for these differences.

Both the mutex and spin lock graphs appear to be increasing linearly with increasing threads, but the
rate of increase for spin locks is much greater than that of the mutex locks. The curves are linear because
the time that each thread spends waiting for a lock is proportional to the number of threads attempting to
acquire the lock.

The spin lock has a greater rate of increase because its cost for a low number of threads is similar to that of
the mutex lock because each thread can simultaneously havd access to a CPU. However, as the number of threads
increases, the cost of spin waiting becomes much more expensive than mutex locks because of memory contention
as each thread can only hold one CPU, and therefore can not share its L1 and L2 cached values with the other CPUs.
This results in wasted time.
