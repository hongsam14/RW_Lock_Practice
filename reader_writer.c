#include "reader_writer.h"

void rwlock_init(rwlock_t *rw)
{
	Sem_init(&rw->ok_to_read, 1);
	Sem_init(&rw->ok_to_write, 1);
	Sem_init(&rw->mutex, 1);
	rw->AR = 0;
	rw->AW = 0;
	rw->WR = 0;
	rw->WW = 0;
}

void rwlock_acquire_readlock(rwlock_t *rw)
{
	Sem_wait(&rw->mutex);
	while (rw->WW + rw->AW > 0)
	{
		rw->WR++;
		Sem_post(&rw->mutex);
		Sem_wait(&rw->ok_to_read);
		Sem_wait(&rw->mutex);
		rw->WR--;
	}
	rw->AR++;
	Sem_post(&rw->mutex);
}

void rwlock_release_readlock(rwlock_t *rw)
{
	Sem_wait(&rw->mutex);
	rw->AR--;
	if (rw->AR == 0 && rw->WW > 0)
	{
		Sem_post(&rw->ok_to_write);//unlock
	}
	Sem_post(&rw->mutex);
}

void rwlock_acquire_writelock(rwlock_t *rw)
{
	Sem_wait(&rw->mutex);
	while (rw->AW + rw->AR > 0)
	{
		rw->WW++;
		Sem_post(&rw->mutex);
		Sem_wait(&rw->ok_to_write);//unlock
		Sem_wait(&rw->mutex);
		rw->WW--;
	}
	rw->AW++;
	Sem_post(&rw->mutex);
}

void rwlock_release_writelock(rwlock_t *rw)
{
	Sem_wait(&rw->mutex);
	rw->AW--;
	if (rw->WW > 0)
		Sem_post(&rw->ok_to_write);
	else if(rw->WR > 0)
		Sem_post(&rw->ok_to_read);
	Sem_post(&rw->mutex);
}

//
// Don't change the code below (just use it!) But fix it if bugs are found!
// 
#define TAB 10
#define TICK sleep(1)    // 1/100초 단위로 하고 싶으면 usleep(10000)
#define MAX_WORKERS 10

int loops;
int DB = 0;

sem_t print_lock;

void space(int s) {
    Sem_wait(&print_lock);
    int i;
    for (i = 0; i < s * TAB; i++)
        printf(" ");
}

void space_end() {
    Sem_post(&print_lock);
}

rwlock_t rwlock;

void *worker(void *arg) {
    arg_t *args = (arg_t *)arg;
    int i;
    for (i = 0; i < args->arrival_delay; i++) {
        TICK;
        space(args->thread_id); printf("arrival delay %d of %d\n", i, args->arrival_delay); space_end();
    }
    if (args->job_type == 0) reader(arg);
    else if (args->job_type == 1) writer(arg);
    else {
        space(args->thread_id); printf("Unknown job %d\n",args->thread_id); space_end();
    }
    return NULL;
}
    
void *reader(void *arg) {
    arg_t *args = (arg_t *)arg;
    
    TICK;
    rwlock_acquire_readlock(&rwlock);
   // start reading
    int i;
    for (i = 0; i < args->running_time-1; i++) {
        TICK;
        space(args->thread_id); printf("reading %d of %d\n", i, args->running_time); space_end();
    }
    TICK;
    space(args->thread_id); printf("reading %d of %d, DB is %d\n", i, args->running_time, DB); space_end();
    // end reading
    TICK;
    rwlock_release_readlock(&rwlock);
    return NULL;
}

void *writer(void *arg) {
    arg_t *args = (arg_t *)arg;

    TICK;
    rwlock_acquire_writelock(&rwlock);
   // start writing
    int i;
    for (i = 0; i < args->running_time-1; i++) {
        TICK;
        space(args->thread_id); printf("writing %d of %d\n", i, args->running_time); space_end();
    }
    TICK;
    DB++;
    space(args->thread_id); printf("writing %d of %d, DB is %d\n", i, args->running_time, DB); space_end();
    // end writing 
    TICK;
    rwlock_release_writelock(&rwlock);

    return NULL;
}

/***********************************
  arg_t 구조체에 값을 입력한다.
return:
	OK 다음 thread입력값의 첫 글자 주소
	ERR NULL
 ***********************************/

static int	set_thread_info(int id, char *split, arg_t *a)
{
	char	*cpy;
	char	*p;
	char	*e;
	int	idx, size;
	
	idx = 0;
	
	a->thread_id = id;//thread id
	
	size = strlen(split);
	cpy = (char *)malloc(sizeof(char) * (size + 1));
	assert(cpy != NULL);
	strcpy(cpy, split);
	
	p = cpy;
	e = cpy;
	while (*p != '\0')
	{
		int end;
		
		end = 0;
		//mini strtok
		while (*e != ':' && *e != '\0')
			e++;
		if (*e == '\0')
			end = 1;
		*e = '\0';
		switch (idx)
		{
			case 0:
				a->job_type = atoi(p);
				break;
			case 1:
				a->arrival_delay = atoi(p);
				break;
			case 2:
				a->running_time = atoi(p);
				break;
			default:;
		}
		idx++;
		if (!end)
			p = e + 1;
		else
			p = e;
		e = p;
	}
	//free
	free(cpy);
	
	return (int)(idx == 3);
}

/***********************************
  arg_t 리스트를 초기화한다.
return:
	OK 1
	ERR 0(argc가 5가 아닐때),
	ERR -1(n값 중복 입력),
	ERR -2(입력 순서가 틀릴때),
	ERR -3(입력값이 틀린 값일 때),
	ERR -4(n과 a값이 다를때),
	ERR -5(wrong flag)
 ***********************************/

static int	set_arg_from_argv(int argc, char **argv, int *num_workers, arg_t* a)
{
	int	thrd_cnt;
	int	c_idx, a_idx;
	char	*split;

	//init
	thrd_cnt = 0;
	c_idx = -1;
	a_idx = -1;
	
	if (argc != 5)
		return 0;
	for (int i = 1; i < argc; ++i)
	{
		if (!strncmp(argv[i], "-n", 2))//check -n
		{
			c_idx = i + 1;
			continue;
		}
		if (!strncmp(argv[i], "-a", 2))//check -p
		{
			a_idx = i + 1;
			continue;
		}
		if (i == c_idx) //set num_workers
		{
			if (thrd_cnt) //duplicated error
				return -1;
			
			thrd_cnt = atoi(argv[i]);
			*num_workers = thrd_cnt;
			continue;
		}
		if (i == a_idx)//set thread list
		{
			if (!thrd_cnt) //input -n first
				return -2;
			//split str with ','
			int cnt;
			
			split = strtok(argv[i], ",");
			for(cnt = 0; split != NULL; cnt++, split = strtok(NULL, ","))
			{
				//trim
				while (isspace(*split))
					split++;
				printf("dd:%s\n", split);
				if (!(set_thread_info(cnt, split, &a[cnt]))) //wrong value
					return -3;
			}
			if (cnt != thrd_cnt)
				return -4;
			continue;
		}
		return -5;
	}
	return 1;
	
}

int main(int argc, char *argv[]) {
    
    int num_workers;
    pthread_t p[MAX_WORKERS];
    arg_t a[MAX_WORKERS];
    
    Sem_init(&print_lock, 1);//init print_lock 
    rwlock_init(&rwlock);

	//init
    switch (set_arg_from_argv(argc, argv, &num_workers, a))
    {
	    case 1://OK
		    break;
	    case 0:
		    write(2, "wrong arg counts\n", 18);
		    return 1;
	    case -1:
		    write(2, "already input -n value\n", 24);
		    return 1;
	    case -2:
		    write(2, "please input -n value before input -a value\n", 46);
		    return 1;
	    case -3:
		    write(2, "-a value is wrong format\n", 26);
		    return 1;
	    case -4:
		    write(2, "missmatch -a -n value\n", 23);
		    return 1;
	    case -5:
		    write(2, "wrong flag\n", 12);
		    return 1;
	    default:
		    return 1;
    }

    printf("begin\n");   
    printf(" ... heading  ...  \n");   // a[]의 정보를 반영해서 헤딩 라인을 출력
    
    for (int i = 0; i < num_workers; i++)
        Pthread_create(&p[i], NULL, worker, &a[i]);
    
    for (int i = 0; i < num_workers; i++)
        Pthread_join(p[i], NULL);

    printf("end: DB %d\n", DB);

    return 0;
}


