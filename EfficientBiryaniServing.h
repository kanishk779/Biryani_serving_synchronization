#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<pthread.h>
#include<time.h>
#include<errno.h>
#define true 1
#define false 0

int student_id;
int serving_table_id;
int robot_id;
typedef struct robot
{
	struct robot * next;
	int number_of_biryani;
	int identifier;
}robot;

typedef struct serving_table
{
	struct serving_table *next;
	int identifier;
	int total_slots_available;
	int student_who_ate;
	int slots_currently_available;
	int can_start_eating;
	int want_biryani;
}serving_table;

typedef struct student
{
	struct student *next;
	int identifier;
}student;

typedef struct program_state
{
	int total_students;
	int total_robots;
	int total_serving_tables;
	int student_finished_eating;	// these students when becomes equal to total_students
	pthread_t * robot_threads;
	pthread_t * table_threads;
	pthread_t * student_threads;
	robot * robot_head;
	serving_table * serving_table_head;
	student * student_head;
}state;
state *st;
pthread_mutex_t mutex;
pthread_cond_t biryani_required = PTHREAD_COND_INITIALIZER;
pthread_cond_t biryani_filled = PTHREAD_COND_INITIALIZER;
pthread_cond_t slots_taken = PTHREAD_COND_INITIALIZER;
pthread_cond_t slots_available = PTHREAD_COND_INITIALIZER;
pthread_cond_t start_eating = PTHREAD_COND_INITIALIZER;
pthread_cond_t stop_eating = PTHREAD_COND_INITIALIZER;
serving_table * temp_table;