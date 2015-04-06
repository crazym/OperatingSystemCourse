/* BEGIN A3 SETUP */
/*
 * Declarations for file handle and file table management.
 * New for A3.
 */

#ifndef _FILE_H_
#define _FILE_H_

#include <kern/limits.h>

//struct vnode;

/**
 * file table handles
 */
struct file_handle {
	//char *fname;
	struct vnode *fvnode;
	off_t cur_po;
	int fflag; 
    int ref_count;
	struct lock *flock; 
    
};

/*
 * filetable struct
 * just an array, nice and simple.  
 * It is up to you to design what goes into the array.  The current
 * array of ints is just intended to make the compiler happy.
 */
struct filetable {
	struct file_handle* file_handles[__OPEN_MAX]; /* dummy type */
};

/* these all have an implicit arg of the curthread's filetable */
int filetable_init(void);
void filetable_destroy(struct filetable *ft);

/* opens a file (must be kernel pointers in the args) */
int file_open(char *filename, int flags, int mode, int *retfd);

/* closes a file */
int file_close(int fd);

/* A3: You should add additional functions that operate on
 * the filetable to help implement some of the filetable-related
 * system calls.
 */
int file_lookup(int fd, struct file_handle **fhandle);
int file_set(int fd, struct file_handle *fhandle);
#endif /* _FILE_H_ */

/* END A3 SETUP */
