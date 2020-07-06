#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "threads/mmu.h"
#include "intrinsic.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

void halt (void);
void exit (int status);

bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);


/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	//lock_init (&file_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	/* Notes from the document (by jeongeon)
	Thus, when the system call handler syscall_handler() gets control, the system call number is in the rax, 
	and arguments are passed with the order %rdi, %rsi, %rdx, %r10, %r8, and %r9. */
	/* load() --> argument passing done
	   thread_create () --> some registers in the intr_frame is also filled */

	int args[6];
	args[0] = f->R.rdi;
	args[1] = f->R.rsi;
	args[2] = f->R.rdx;
	args[3] = f->R.r10;
	args[4] = f->R.r8;
	args[5] = f->R.r9;

	// Check if rsp has a valid address
	
	if (is_kernel_vaddr (f->rsp) && f->rsp != NULL) {
		exit (-1); 
	}
	if (pml4_get_page (thread_current ()->pml4, f->rsp) == NULL) {
		exit (-1);
	}
	
	for (int i = 0; i < 6; i++) {
		/*printf ("data: %x\n", args[i]);
		void * ptr = args[i];
		printf ("pointer: %x\n", ptr);*/
		if (args[i] != NULL) {
			//if (ptr != NULL)
			if (is_kernel_vaddr (args[i])) {
				//printf("at point 3\n");
				exit (-1);
			}
			/*else if (is_user_vaddr (args[i]) && pml4_get_page (thread_current ()->pml4, args[i]) == NULL) {
				printf("at point 4\n");
				printf("%d\n", (uint64_t) pml4_get_page (thread_current ()->pml4, args[i]));
			}*/
		}
	}
	
	// System Calls
	int sysnum;
	sysnum = f->R.rax;
	//printf ("%d\n", sysnum);
	switch (sysnum) {
		case SYS_HALT:
			halt ();
			break;

		case SYS_EXIT:
			exit (f->R.rdi);
			break;

		case SYS_CREATE:
			if (is_user_vaddr (f->R.rdi) && pml4_get_page (thread_current ()->pml4, f->R.rdi) == NULL) {
				exit(-1);
			}
			f->R.rax = create (f->R.rdi, f->R.rsi); // return bool
			break;

		case SYS_REMOVE:
			if (is_user_vaddr (f->R.rdi) && pml4_get_page (thread_current ()->pml4, f->R.rdi) == NULL) {
				exit(-1);
			}
			f->R.rax = remove (f->R.rdi); // return bool
			break;

		case SYS_OPEN:
			if (is_user_vaddr (f->R.rdi) && pml4_get_page (thread_current ()->pml4, f->R.rdi) == NULL) {
				exit(-1);
			}
			f->R.rax = open (f->R.rdi); // return int
			break;

		case SYS_FILESIZE:
			f->R.rax = filesize (f->R.rdi); // return int
			break;

		case SYS_READ:
			if (is_user_vaddr (f->R.rsi) && pml4_get_page (thread_current ()->pml4, f->R.rsi) == NULL) {
				exit(-1);
			}
			f->R.rax = read (f->R.rdi, f->R.rsi, f->R.r10); // return int
			break;

		case SYS_WRITE:
			if (is_user_vaddr (f->R.rsi) && pml4_get_page (thread_current ()->pml4, f->R.rsi) == NULL) {
				exit(-1);
			}
			f->R.rax = write (f->R.rdi, f->R.rsi, f->R.r10); // return int
			break;
	}
}

void
halt (void) {
	power_off();
}

void
exit (int status) {
	struct thread *curr = thread_current ();
	//curr->exit_status = status;
	int count = 0;
	char *token, *save_ptr;
	char *file_name = curr->name;

	for (token = strtok_r (file_name, " ", &save_ptr); token != NULL; token = strtok_r (NULL, " ", &save_ptr)) {
		count += 1;
	}

	printf ("%s: exit(%d)\n", file_name, status);
	thread_exit ();
}

bool
create (const char *file, unsigned initial_size) {
	if (file != NULL) {
		return filesys_create (file, initial_size);
	}
	exit (-1);
}

bool
remove (const char *file) {
	if (file != NULL) {
		return filesys_remove (file);
	}
	exit (-1);
}

int
open (const char *file) {
	struct thread *curr = thread_current ();
	struct file *f = NULL;
	int fd = 0;
	int idx = 0;
	if (file != NULL) {
		f = filesys_open (file);
		if (f != NULL) {
			//while ((struct file_fd *)curr->fdt_ptr[idx] != NULL && idx < 128) {
			//	idx += 1;
			//}
			idx = curr->max_fd;
			if (idx < 128) {
				fd = curr->max_fd + 1;
				struct file_fd *new_file_fd;
				new_file_fd->f = f;
				new_file_fd->fd = fd;
				curr->max_fd = fd;
				curr->fdt_ptr[idx] = new_file_fd;
				printf ("index at open: %d\n", idx);
				printf ("fd at open: %d\n", fd);
				printf ("address of file: %x\n", f);
				return fd;
			}
		}
	}
	return -1;
}

int
filesize (int fd) {
	struct thread *curr = thread_current ();
	int idx = 0;
	while (idx < 128) {
		if (curr->fdt_ptr[idx]->fd == fd) {
			return file_length (curr->fdt_ptr[idx]->f);
		}
		idx += 1;
	}
}

int
read (int fd, void *buffer, unsigned size) {
	lock_acquire (&file_lock);
	struct thread *curr = thread_current ();
	int idx = 1;
	//struct file *fn = NULL;
	int size_read = -1;

	printf ("fd: %d\n", fd);

	if (fd == 0) {
		printf ("Here 1\n");
		buffer = input_getc();
		//lock_release(&file_lock);
		return sizeof(buffer);
	}
	else if (fd != 1) {
		printf ("Here 2\n");
		while (idx < 128) {
			if (curr->fdt_ptr[idx]->fd == fd) {
				//printf ("Here 2.5\n");
				//fn = curr->fdt_ptr[idx]->f;
				//printf ("file address: %x\n", fn);
				printf ("at idx: %d\n", idx);
				break;
			}
			//printf ("Here 3\n");
			idx += 1;
		}
		printf ("%d\n", idx);
		
		size_read = file_read (curr->fdt_ptr[idx]->f, buffer, size);
		if (size_read >= 0) {
			printf ("%d\n", size_read);
			lock_release (&file_lock);
			return size_read;
		}
	}
	printf ("Here 5\n");
	//lock_release(&file_lock);
	return -1;
}


int
write (int fd, const void *buffer, unsigned size) {
	struct thread *curr = thread_current ();
	int idx;
	struct file *f;
	int size_write;
	/* FIXME:
	int mask = 0xFF; */
	//printf ("write entered\n");

	if (fd == 0) {
		return 0;
	}

	if (fd != 0 && fd == 1) {
		/* FIXME: 
		if (sizeof(buffer) > 1<<10) {
			while (buffer != 0x00) {
				void *masked_buffer = buffer & mask; 
				buffer = buffer>>8
				putbuf(masked_buffer);
			}
		}*/
		putbuf(buffer, strlen(buffer));
		return size;
	}
	else {
		lock_acquire(&file_lock);
		while (idx < 128) {
			if (curr->fdt_ptr[idx]->fd == fd) {
				f = curr->fdt_ptr[idx]->f;
			}
			idx += 1;
		}
		size_write = file_write(f, buffer, size);
		if (size_write >= 0) {
			lock_release(&file_lock);
			return size_write;
		}
	}
	return 0;
}