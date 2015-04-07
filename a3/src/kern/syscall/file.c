/* BEGIN A3 SETUP */
/*
 * File handles and file tables.
 * New for ASST3
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/unistd.h>
#include <file.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <kern/fcntl.h>
#include <synch.h>
#include <current.h>

/*** openfile functions ***/

/*
 * file_open
 * opens a file, places it in the filetable, sets RETFD to the file
 * descriptor. the pointer arguments must be kernel pointers.
 * NOTE -- the passed in filename must be a mutable string.
 * 
 * A3: As per the OS/161 man page for open(), you do not need 
 * to do anything with the "mode" argument.
 */
int
file_open(char *filename, int flags, int mode, int *retfd)
{
	int open_f;
	struct vnode *vn;
	struct file_handle *fhandle;
	int i;

	// get vnode
	open_f = vfs_open(filename, flags, mode, &vn);
    if (open_f){
        return open_f;
    } 

    fhandle = kmalloc(sizeof(struct file_handle));
    if (fhandle == NULL){
    	vfs_close(vn);
    	return ENOMEM;
    }

    //strcpy(fhandle->fname, filename);

    fhandle->fvnode = vn;
	fhandle->cur_po = 0;
	fhandle->fflag = flags; 
    fhandle->ref_count = 1;

	fhandle->flock = lock_create("file handle lock"); 
	if (fhandle->flock == NULL){
		vfs_close(vn);
		kfree(fhandle);
		return ENOMEM;
	}

	// KASSERT(curthread->t_filetable!=NULL);
	//add file handle to file table
	for (i=0; i<__OPEN_MAX; i++){
		if (curthread->t_filetable->file_handles[i] == NULL){
			curthread->t_filetable->file_handles[i] = fhandle;
			*retfd = i;
			break;
		}
	}
	//file table is full, cannot add fhandle
	if (i == __OPEN_MAX && *retfd != i){
		vfs_close(vn);
		lock_destroy(fhandle->flock);
		kfree(fhandle);
		return ENOMEM;
	}

	return 0;
}


/* 
 * file_close
 * Called when a process closes a file descriptor.  Think about how you plan
 * to handle fork, and what (if anything) is shared between parent/child after
 * fork.  Your design decisions will affect what you should do for close.
 */
int
file_close(int fd)
{
	struct file_handle *fhandle;

	KASSERT(curthread->t_filetable!=NULL);
	if (fd < 3 || fd >= __OPEN_MAX){
		return EBADF;
	};

	fhandle = curthread->t_filetable->file_handles[fd];
	if (fhandle == NULL){
		return EBADF;
	};

	lock_acquire(fhandle->flock);

	//if only 1 reference to the file, can free file handle after close the file
	if (fhandle->ref_count == 1){
		vfs_close(fhandle->fvnode);
		lock_release(fhandle->flock);
		lock_destroy(fhandle->flock);
		kfree(fhandle);
	} else{
		fhandle->ref_count--;
		lock_release(fhandle->flock);
	}
	curthread->t_filetable->file_handles[fd] = NULL;

	return 0;
}

/*** filetable functions ***/

/* 
 * filetable_init
 * pretty straightforward -- allocate the space, set up 
 * first 3 file descriptors for stdin, stdout and stderr,
 * and initialize all other entries to NULL.
 * 
 * Should set curthread->t_filetable to point to the
 * newly-initialized filetable.
 * 
 * Should return non-zero error code on failure.  Currently
 * does nothing but returns success so that loading a user
 * program will succeed even if you haven't written the
 * filetable initialization yet.
 */

int
filetable_init(void)
{
	struct filetable *ft = kmalloc(sizeof(struct filetable));
	
	if (ft == NULL){
		return ENOMEM;
	}

	for (int fd=0; fd<__OPEN_MAX; fd++) {
        ft->file_handles[fd] = NULL;
    }
 	
	curthread->t_filetable = ft;
	
    char path[5];
    int fd;
    int open_f;
	
    strcpy(path, "con:");
	open_f = file_open(path, O_RDONLY, 0, &fd);
	if (open_f){
		return open_f;
	}

	strcpy(path, "con:");
	open_f = file_open(path, O_WRONLY, 0, &fd);
	if (open_f){
		return open_f;
	}

	strcpy(path, "con:");
	open_f = file_open(path, O_WRONLY, 0, &fd); // O_WTONLY??
	if (open_f){
		return open_f;
	}

	return 0;
}	

/*
 * filetable_destroy
 * closes the files in the file table, frees the table.
 * This should be called as part of cleaning up a process (after kill
 * or exit).
 */
void
filetable_destroy(struct filetable *ft)
{
        int fd;
        int close_f;
        
        if (ft == NULL){
        	kprintf("Filetable does not exists\n");
        } else{
        	for (fd = 0; fd < __OPEN_MAX; fd++) {
	            if (ft->file_handles[fd]) {
	                close_f = file_close(fd);
	                if (close_f){
	                	kprintf("Could not close file with fd %d in the table\n", fd);
	                }
	            }
	        }
        }
        kfree(ft);
}	


/* 
 * You should add additional filetable utility functions here as needed
 * to support the system calls.  For example, given a file descriptor
 * you will want some sort of lookup function that will check if the fd is 
 * valid and return the associated vnode (and possibly other information like
 * the current file position) associated with that open file.
 */

/*
 * file_lookup 
 * reutrn the file handle at the given fd.
 *
 */
int
file_lookup(int fd, struct file_handle **fhandle)
{

	if (fd < 0 || fd >= __OPEN_MAX){
		return EBADF;
	}

	KASSERT(curthread->t_filetable!=NULL);
	*fhandle = curthread->t_filetable->file_handles[fd];
	if (*fhandle == NULL) {
		// no open file in the file table at fd
		return EBADF; 
	}

	return 0;
}

int
file_set(int fd, struct file_handle *fhandle)
{
	//fail is there is no filetable associated
	KASSERT(curthread->t_filetable!=NULL);

	if (fd < 0 || fd >= __OPEN_MAX){
		return EBADF;
	}

	// close it if there exists an open file in the file table at fd
	if (curthread->t_filetable->file_handles[fd] != NULL) {
		file_close(fd);
	}
	// set fd points to the new file
	curthread->t_filetable->file_handles[fd] = fhandle;


	return 0;
}

/* END A3 SETUP */
