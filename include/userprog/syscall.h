#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* LAB 2: Lock for syscall */
struct lock *file_lock;

void syscall_init (void);
#endif /* userprog/syscall.h */
