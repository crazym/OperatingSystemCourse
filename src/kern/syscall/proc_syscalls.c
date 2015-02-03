/*
 * Process-related syscalls.
 * New for ASST1.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <current.h>
#include <pid.h>
#include <machine/trapframe.h>
#include <syscall.h>

/*
 * sys_fork
 * 
 * create a new process, which begins executing in md_forkentry().
 */


int
sys_fork(struct trapframe *tf, pid_t *retval)
{
	struct trapframe *ntf; /* new trapframe, copy of tf */
	int result;

	/*
	 * Copy the trapframe to the heap, because we might return to
	 * userlevel and make another syscall (changing the trapframe)
	 * before the child runs. The child will free the copy.
	 */

	ntf = kmalloc(sizeof(struct trapframe));
	if (ntf==NULL) {
		return ENOMEM;
	}
	*ntf = *tf; /* copy the trapframe */

	result = thread_fork(curthread->t_name, enter_forked_process, 
			     ntf, 0, retval);
	if (result) {
		kfree(ntf);
		return result;
	}

	return 0;
}

/*
 * sys_getpid
 * Placeholder to remind you to implement this.
 */
int 
sys_getpid(pid_t *retval) {
	KASSERT(curthread != NULL);
	*retval = curthread->t_pid;
	return 0;
}


/*
 * sys_waitpid
 * Placeholder comment to remind you to implement this.
 */
 int sys_waitpid(pid_t pid, int *status, int options, pid_t *retval) {
 	
 	// Check if invalid option
 	if (options != 0)
 		return EINVAL;

 	// Check for invalid pointer
 	if (status == NULL)
 		return EFAULT;

 	pid_t target_parent = pid_parent(pid);

 	// Check if pid_exist, not needed since pid_join does it
 	if (target_parent == -1)
 		return ESRCH;

 	// Check if thread is child
 	KASSERT(curthread->t_pid == target_parent);
 	if (curthread->t_pid != target_parent)
 		return ECHILD;

 	pid_t child = pid_join(pid, status, options);
 	
 	// If join was unsuccessful
 	if (child < 0)
 		return -child;
 	else
 		*retval = child;
 		return 0;
 }


/*
 * sys_kill
 * Placeholder comment to remind you to implement this.
 */

 /*
 int kill(pid_t pid, int sig, int *retval) {
 	return 0;
 }*/


