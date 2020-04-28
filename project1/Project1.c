#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<wait.h>
#include<sched.h>
#include<sys/sysinfo.h>
#include<sys/time.h>
#include<sys/resource.h>
#include<ctype.h>
#include<string.h>
#include <sys/syscall.h>

#define max_num 100

typedef struct Pro{
	char name[10];
	int id;
	int c_pid;
	int ready;
	int execution;
	int priority;
	int start;
	int finish;
	int taround;
	int remain;
	struct Pro *next;
}Process;
Process *head;

Process process[max_num];
Process process2[max_num];

int cpu_num;
int t_cnt = 0;
int priority = 99;
int policy = -1;
int process_num;

void count_time(){
	volatile unsigned long i;
	for(i=0;i<1000000UL;i++);
	t_cnt ++;
	return; 
} 

void swap(int a, int b){
	Process tmp = process2[a];
	process2[a] = process2[b];
	process2[b] = tmp;
}

void set_cpu(pid_t pid,int k){
	cpu_set_t newmask;
	CPU_ZERO(&newmask);
	CPU_SET(k,&newmask);
	int res = sched_setaffinity(pid,sizeof(newmask),&newmask);
}

void child_process(pid_t pid,int order){
	int pri;
	if(policy == 1){
		setpriority(PRIO_PROCESS,getpid(),1);
		pri = getpriority(PRIO_PROCESS,getpid());
	//	printf("%s priority is %d\n",process[order].name,pri);
	}
	set_cpu(getpid(),1);
	struct sched_param param;
//	printf("%s set itself %d\n",process[order].name,process[order].priority);
	param.sched_priority = process[order].priority;
//	if(sched_setscheduler(0,SCHED_FIFO,&param)==-1)
//		printf("set schedule error!\n");

	printf("%s %d\n",process[order].name,getpid());
	struct timespec start,end;
	syscall(334,&start);
	for(int j = 0 ; j < process[order].execution; j++){
		count_time();
		if(policy == 1 && (j+1)%500 == 0 && j != process[order].execution-1){
//			printf("%s turn%d\n",process[order].name,j/500);
			param.sched_priority = 98;
//			printf("%s pri = %d after run %d unit\n",process[order].name,param.sched_priority,j);
			setpriority(PRIO_PROCESS,getpid(),2);
			if(sched_setscheduler(0,SCHED_FIFO,&param)==-1)
				printf("RR set schedule error!\n");
		}
	}	
	syscall(334,&end);
	syscall(335,start,end,getpid());
//	printf("%s finished\n",process[order].name);	
	if(policy == 1){
		setpriority(PRIO_PROCESS,getpid(),-1);	
	}
	exit(1);	
}

void FIFO(pid_t pid, int order){
	struct sched_param param;
	param.sched_priority = 99;
	process[order].priority = 99;
	if(sched_setscheduler(pid,SCHED_FIFO,&param)==-1)
		printf("parent set error!\n");
}

void RR(pid_t pid, int order){
	struct sched_param param;
	param.sched_priority = 99;
	process[order].priority = 99;
	if(sched_setscheduler(pid,SCHED_FIFO,&param)==-1)
		printf("parent set error!\n");
}

void SJF_pre(){
	for(int i = process_num - 1; i > 0; i--){
		if(process2[i].ready == process2[i-1].ready && process2[i].execution < process2[i-1].execution){
			Process tmp = process2[i];		
			process2[i] = process2[i-1];
			process2[i-1] = tmp;	
		}
	}
	int tmp,key;
	process2[0].finish = process2[0].ready + process2[0].execution;
	process2[0].taround = process2[0].finish - process2[0].ready;
	process2[0].start = process2[0].taround - process2[0].execution;
	for(int i = 1; i < process_num; i++){
		tmp = process2[i-1].finish;
		int low = process2[i].execution;
		key = i;
		for(int j = i; j < process_num; j++){
			if(tmp >= process2[j].ready && low > process2[j].execution){
				low = process2[j].execution;
				key = j;
			}
		}
		process2[key].finish = tmp + process2[key].execution;
		process2[key].taround = process2[key].finish - process2[key].ready;
	    process2[key].start = process2[key].taround - process2[key].execution;
		swap(i,key);
	}
	for(int i = 0 ; i < process_num; i++){
		process[process2[i].id].priority = 99 - i;
	}
}

void SJF(pid_t pid, int order){
	struct sched_param param;
	param.sched_priority = process[order].priority;
	if(sched_setscheduler(pid,SCHED_FIFO,&param)==-1)
		printf("parent set error!\n");
}

void op(){
	Process *cur = head->next;
	printf("link list:");
	while(cur != NULL){
		printf("process%d ",cur->id);
		cur = cur->next;
	}
	printf("\n");
}

void insert(int k){
//	printf("insert for %d\n",k);
	if(head->next == NULL){
		head->next = &process[k];
		return;
	}

	Process *cur = head->next;
	int t_dif = process[k].ready - process[k-1].ready;
//	printf("t dif = %d\n",t_dif);
	while(t_dif > 0){
		int min = t_dif < cur->remain? t_dif:cur->remain;
		t_dif -= min;
		cur->remain -= min;
		if(cur->next != NULL)
			cur = cur->next;
		else
			break;
	}
//	op();
	cur = head;
	while(1){
		if(cur->next == NULL){
			cur->next = &process[k];
			break;
		}
		if(cur->next->remain > process[k].remain){
			process[k].next = cur->next;
			cur->next = &process[k];
			break;
		}
		cur = cur->next;
	}
//	op();
}

void PSJF(pid_t pid,int order){
	insert(order);
	Process *cur = head->next;
	for(int i = 99; cur != NULL; i--){
		struct sched_param param;
		cur->priority = i;
		param.sched_priority = i;
		if(cur->remain > 0){
		//	printf("set %s with priority %d\n",cur->name,i);
			if(sched_setscheduler(cur->c_pid,SCHED_FIFO,&param)==-1) 
				printf("set error\n");
		}
		cur = cur->next;
	}
	return;
}

int main(){
	set_cpu(getpid(),0);

	struct sched_param mainparam;
	mainparam.sched_priority = 99;
	if(sched_setscheduler(getpid(),SCHED_FIFO,&mainparam)==-1) 
		printf("set error\n");

	head = (Process*)malloc(sizeof(Process));
	head->c_pid = -1;
	head->next = NULL;

	char Pol[4][5] = {"FIFO","RR","SJF","PSJF"};
	char P[5];
	scanf("%s%d",P,&process_num);
	for(int j = 0 ; j < 4; j++){
		if(strcmp(P,Pol[j]) == 0)
			policy = j;}

	
	for(int i = 0 ; i < process_num; i++){
		scanf("%s%d%d",process[i].name,&process[i].ready,&process[i].execution);
		process[i].id = i;
		process[i].priority = 99;
		process[i].start = -1;
		process[i].finish = -1;
		process[i].remain = process[i].execution;
		process[i].next = NULL;
		process2[i] = process[i];
	}
	
	if(policy == 2){;
		SJF_pre();
	}	


	for(int i = 0 ; i < process_num; i++){
		while(t_cnt < process[i].ready){
			if(policy == 1){
			//	printf("t_cnt = %d\n",t_cnt);
				for(int j = 0; j < i; j++){
					int pri = getpriority(PRIO_PROCESS,process[j].c_pid);
					if(pri == 2){
				//		printf("check process %d when t_cnt = %d\n",j,t_cnt);
						struct sched_param tmpparam;
						tmpparam.sched_priority = 99;
						sched_setscheduler(process[j].c_pid,SCHED_FIFO,&tmpparam);
						setpriority(PRIO_PROCESS,process[j].c_pid,1);		
					}			
				}		
			}
			for(int j = 0; j < 100; j++)
				count_time();		
		}

//		printf("main:round%d\n",i);
		int pid;
		pid = fork();

		if(pid == 0){ //child process
			child_process(getpid(),i);
			exit(1);
		}
		else if(pid < 0){	//error
			printf("fork error!\n");
		}

		// parent process
		set_cpu(pid,1);
		process[i].c_pid = pid;

		switch(policy){
			case 0:{	//FIFO
				FIFO(pid,i);
				break;
			}

			case 1:{	//RR
				RR(pid,i);
				break;
			}

			case 2:{	//SJF
				SJF(pid,i);
				break;		
			}

			case 3:{	//PSJF
				PSJF(pid,i);
				break;		
			}

		}//switch end

	}

	if(policy == 1){
		
		int run_process = process_num;
		while(run_process > 0){
			if(t_cnt%100 == 0){
				int curpro = 0;
			//	printf("t_cnt = %d\n",t_cnt);
				for(int i = 0; i < process_num;i++){
					if(getpriority(PRIO_PROCESS,process[i].c_pid) == -1){
					//	printf("%s finished when %d\n",process[i].name,t_cnt);
					}
					else{
						curpro++;		
					}
					if(getpriority(PRIO_PROCESS,process[i].c_pid) == 2){
						struct sched_param param;
						setpriority(PRIO_PROCESS,process[i].c_pid,1);
						param.sched_priority = 99;
						if(sched_setscheduler(process[i].c_pid,SCHED_FIFO,&param)==-1)
							printf("RR reset schedule error!\n");
					}
				}
				run_process = curpro;
			}
			count_time();
		}
	}
	
	int status;
	for(int i=0;i<process_num;i++){	
		wait(&status);
	}
					
	return 0;
}
