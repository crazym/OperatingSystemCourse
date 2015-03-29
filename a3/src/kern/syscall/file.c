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
	int cur_fd;

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

    strcp(fhandle->fname, filename);

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

	//add file handle to file table
	for (int i=3; i<__OPEN_MAX; i++){
		if (curthread->t_filetable->file_handles[i] == NULL){
			curthread->t_filetable->file_handles[i] = fhandle;
			*retfd = i;
			break;
		}
	}
	//file table is full, cannot add fhandle
	if (i == __OPEN_MAX and *retfd != i){
		vfs_close(vn);
		lock_destory(fhandle->flock);
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
	int close_f;
	struct file_handle *fhandle;

	*fhandle = curthread->t_filetable->ft_openfiles[fd];
	if (fd < 3 || fd >= __OPEN_MAX || *fhandle == NULL){
		return EBADF;
	};

	lock_aquire(fhandle->flock);

	//if only 1 reference to the file, can free file handle after close the file
	if (fhandle->ref_count == 1){
		vfs_close(fhandle->fvnode);
		lock_release(fhandle->flock);
		lock_destory(fhandle->flock);
		//???kfree(fhandle->fname); and ref_count
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
	struct filetable *filetable = kmalloc(sizeof(struct filetable));
	if (filetable == NULL){
		return ENOMEM;
	}

	/*
	for (fd = 0; fd < __OPEN_MAX; fd++) {
        filetable->file_handles[fd] = NULL;
    }
 	*/

	curthread->t_filetable = filetable;
	

    char path[5];
    int fd;
    int open_f;
	
    strcpy(path, "con:");
	open_f = file_open(path, O_RDONLY, 0, &fd);
	if open_f{
		return open_f;
	}

	strcpy(path, "con:");
	open_f = file_open(path, O_WTONLY, 0, &fd);
	if open_f{
		return open_f;
	}

	strcpy(path, "con:");
	open_f = file_open(path, O_WTONLY, 0, &fd); // O_WTONLY??
	if open_f{
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
        // if ft != NULL ??
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


/* END A3 SETUP */
