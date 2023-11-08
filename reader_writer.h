#ifndef READER_WRITER_H
# define READER_WRITER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "common_threads.h"

# ifdef __APPLE__
#  include "zemaphore.h"
#endif

typedef struct {
    int thread_id;
    int job_type;         // 0: reader, 1: writer
    int arrival_delay;
    int running_time;
} arg_t;

void	*worker(void *arg);
void	*reader(void *arg);
void	*writer(void *arg);

typedef struct	_rwlock_t {
	sem_t	ok_to_read;
	sem_t	ok_to_write;
	sem_t	mutex;
	int	WW;
	int	WR;
	int	AR;
	int	AW;
}	rwlock_t;

void	rwlock_init(rwlock_t *rw);

void	rwlock_acquire_readlock(rwlock_t *rw);
void	rwlock_release_readlock(rwlock_t *rw);

void	rwlock_acquire_writelock(rwlock_t *rw);
void	rwlock_release_writelock(rwlock_t *rw);

#endif
