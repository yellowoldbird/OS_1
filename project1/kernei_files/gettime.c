#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

asmlinkage long sys_gettime(struct timespec *time)
{
	getnstimeofday(time);
	return 0;
}
