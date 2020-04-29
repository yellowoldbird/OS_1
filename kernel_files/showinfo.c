#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/time.h>

asmlinkage long sys_showinfo(struct timespec start,struct timespec finish,pid_t pid)
{
	printk("[Project1] %d %ld.%ld %ld.%ld\n",pid,start.tv_sec,start.tv_nsec,finish.tv_sec,finish.tv_nsec);
	return 0;
}
