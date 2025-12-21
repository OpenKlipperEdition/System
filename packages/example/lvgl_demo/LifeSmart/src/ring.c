/*
 * @file ring.c - is provided for use with Ingenic products.
 *
 * No license to Ingenic property rights is granted, Ingenic assumes no
 * liability, provides no warranty either expressed or implied relating
 * to the usage, or intellectual property right infringement except
 * as provided for by Ingenic Terms and Conditions of Sale.
 *
 * All rights reserved by Ingenic Semiconductor CO., LTD.
 *
 * Creator: yyxu <yiyuan.xu@ingenic.com>
 * Maintainer: yyxu <yiyuan.xu@ingenic.com>
 * Created: 2020/09/16
 * Updated:
 */


#include <stdio.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MUSIC_PATH_MAX_SIZE 128
//#define SEMID 1
static char* path;
static int flag=0;
static int flag_cycle=0;

pthread_mutex_t mut;

union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short *arry;
};

#ifdef SEMID
static int sem_id = 0;
static int set_semvalue();
static void del_semvalue();
static int semaphore_p();
static int semaphore_v();
#endif

static void* play_music(void *p)
{
	pthread_detach(pthread_self());
	char * cmd_start ="aplay ";
//	char * cmd_end =" &";
	char * cmd;
	cmd = malloc(strlen(cmd_start) + MUSIC_PATH_MAX_SIZE);
	while (1) {
#ifdef SEMID
		if (!semaphore_p()) break;
#else
    pthread_mutex_lock(&mut);
#endif
		if (NULL != path) {
			do {
				if (path) {
					memset(cmd, 0x00, strlen(cmd_start) + MUSIC_PATH_MAX_SIZE);
					strcpy(cmd, cmd_start);
					strcat(cmd, path);
//					strcat(cmd, cmd_end);
					printf("cmd：%s\n", cmd);
					system(cmd);
					//system("Taplay &");
				}
			} while (flag_cycle && path != NULL);
			if (flag) break;
		}
	}
	free(cmd);
	cmd = NULL;
#ifdef SEMID
	del_semvalue();
#endif
	return NULL;
}

void start_ring_thread()
{
	pthread_t th_k;
#ifdef SEMID
	sem_id = semget((key_t)1234, 1, 0666 | IPC_CREAT);
	if (!set_semvalue()) {
		fprintf(stderr, "Failed to initialize semaphore\n");
		exit(EXIT_FAILURE);
	}
#else
    pthread_mutex_init(&mut,NULL);
#endif

	pthread_create(&th_k, NULL, play_music, NULL);
}

void stop_ring_thread()
{
	flag = 1;
    path = NULL;
#ifdef SEMID
	if (!semaphore_v()) exit(EXIT_FAILURE);
#else
    pthread_mutex_unlock(&mut);
#endif
}

void start_ring(char* file_path)
{
	path = file_path;
	printf("path %s\n", path == NULL ? "NULL" : path);
#ifdef SEMID
	if (!semaphore_v()) exit(EXIT_FAILURE);
#else
    pthread_mutex_unlock(&mut);
#endif
}

void start_music(char* file_path)
{
	//usleep(50000);
	path = file_path;
	flag_cycle = 1;
	printf("music path %s\n",path == NULL ? "NULL" : path);
#ifdef SEMID
	if (!semaphore_v()) exit(EXIT_FAILURE);
#else
    pthread_mutex_unlock(&mut);
#endif
}

void stop_music()
{
	flag_cycle = 0;
	if (path != NULL) {
		path = NULL;
		printf("kill aplay\n");
		system("killall -9 aplay");
	}
}
#ifdef SEMID
static int set_semvalue()
{
	//用于初始化信号量，在使用信号量前必须这样做
	union semun sem_union;
	sem_union.val = 0;
	if (semctl(sem_id, 0, SETVAL, sem_union) == -1)
		return 0;

	return 1;
}

static void del_semvalue()
{
	union semun sem_union;
	if(semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
		fprintf(stderr, "Failed to delete semaphore\n");
}

static int semaphore_p()
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;//P()
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1) {
		fprintf(stderr, "semaphore_p failed\n");
		return 0;
	}

	return 1;
}

static int semaphore_v()
{
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = 1;//V()
	sem_b.sem_flg = SEM_UNDO;
	if (semop(sem_id, &sem_b, 1) == -1) {
		fprintf(stderr, "semaphore_v failed\n");
		return 0;
	}

	return 1;
}
#endif
