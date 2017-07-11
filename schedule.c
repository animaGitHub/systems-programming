#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#define STACK_SIZE 4096
#define empty 0
#define ready 1
#define running 2
#define finished 3
#define infoArraySize 5
#define sleeptime 100000000 //100msec
volatile sig_atomic_t k=1;

int min(int a1, int a2){ if (a1>a2) return a2; else return a1;}

struct ThreadInfo {
ucontext_t context;
int state;
};
struct ThreadInfo ThreadInfoArray[infoArraySize];
void catch_alarm (int sig)
{
  signal (sig, catch_alarm);

  if(k!=0)
  {
  	if (ThreadInfoArray[k].state==running)
  		ThreadInfoArray[k].state = ready;
  swapcontext(&ThreadInfoArray[k].context, &ThreadInfoArray[0].context);
  }
  
}
void printFunc(int argc_main, int argv_main)
{	
	
	struct timespec sleepTime;
    struct timespec returnTime;
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = sleeptime;
	for(int i=0; i<argv_main;i++)
	{
		for (int j=0; j<argc_main; j++)
		{
			printf("	");
		}
		if(i==argv_main-1)
			ThreadInfoArray[k].state = finished;
		printf("%d \n", i);
		if(nanosleep(&sleepTime, &returnTime)==-1){};
	}
	catch_alarm(14);			// call alarm handler since the thread is terminated.

	while(1); // in case the alarm handler call doesn't work, wail until alarm is set by itself.
}
int main(int argc, char* argv[])
{

		//initialization
	int fake_argc;
	int* fake_argv;
	if(argc==1) // if there is no argument, create a sample sequence and show how does the program work.
	{
		srand (time (0));
		fake_argc = 4+(rand()%5);
		fake_argv= new int[fake_argc];
		printf("generated sample random sequence with sample input argument size %d is: schedule ", fake_argc);
		for (int i =0; i<fake_argc; i++)
		{
			fake_argv[i] = 5+(rand()%10);
			if(i!=0)
				printf("%d ", fake_argv[i]);
		}
		printf("\n");
	}
	else // if there are arguments given proceed from this block on;
	{
		fake_argc = argc;
		fake_argv= new int[fake_argc];
		fake_argv[0] = 0;
		for(int i=1; i<argc;i++)
		{
			fake_argv[i]=atoi(argv[i]); // cast a string to an integer
		}
	}

	int startSize = min(fake_argc, infoArraySize); //handle inputs with smaller than 4 threads.
	/* Establish a handler for SIGALRM signals. */
	signal (SIGALRM, catch_alarm);

	for (int i=1; i<startSize; i++)
	{
		getcontext(&ThreadInfoArray[i].context);
		ThreadInfoArray[i].context.uc_link = &ThreadInfoArray[0].context;
		ThreadInfoArray[i].context.uc_stack.ss_sp = malloc(STACK_SIZE);
		ThreadInfoArray[i].context.uc_stack.ss_size = STACK_SIZE;
		makecontext(&ThreadInfoArray[i].context, (void (*)(void))printFunc, 2, i, fake_argv[i]);
		ThreadInfoArray[i].state = ready;
	}
	getcontext(&ThreadInfoArray[0].context);


	//scheduler
	int index = startSize;
	while(1)
	{
		alarm(1);
		switch(ThreadInfoArray[k].state)
			{
				case ready: ThreadInfoArray[k].state = running;
				//printf("k is running, %d with index: %d \n", k, index);
				swapcontext(&ThreadInfoArray[0].context, &ThreadInfoArray[k].context);
				if (ThreadInfoArray[k].state == finished)
				{
					//now, this finished thread should be processed, this can be achieved by switching to the same k value.
					// switch will decide it to be a terminated process so it will empty it.
					// k is incremented automatically, therefore to switch on the same k, it is decremented here.
					k = k-1;
					if (k==0)
						k=startSize-1;
				}
				break;
				case running: //printf("there shouldn't be any running threads seen, k is: %d index is %d \n", k, index);
				swapcontext(&ThreadInfoArray[0].context, &ThreadInfoArray[k].context);
				if (ThreadInfoArray[k].state != finished)
				{
					break;
				}
				case finished: free(ThreadInfoArray[k].context.uc_stack.ss_sp);
				//printf("k is finished, %d with index: %d \n", k, index);
				ThreadInfoArray[k].state = empty;
				//printf("k has been emptied, %d with index: %d \n", k, index);
				case empty:
				if(index<fake_argc)
					{
						getcontext(&ThreadInfoArray[k].context);
						ThreadInfoArray[k].context.uc_link = &ThreadInfoArray[0].context;
						ThreadInfoArray[k].context.uc_stack.ss_sp = malloc(STACK_SIZE);
						ThreadInfoArray[k].context.uc_stack.ss_size = STACK_SIZE;
						makecontext(&ThreadInfoArray[k].context, (void (*)(void))printFunc, 2, index, fake_argv[index]);
						//printf("a new thread has been created with k %d and index: %d \n", k, index);
						ThreadInfoArray[k].state = ready;						
						index++;
					}
			}
			k = (k%(startSize-1))+1;
	}

	return 0;
}