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
int biryani_ready(robot * ro)
{	
	
	serving_table * temp = st->serving_table_head;
	int found = false;
	pthread_mutex_lock(&mutex);
	while(temp != NULL)
	{
		//printf("robot - %d slots - %d\n",ro->identifier,temp->total_slots_available );
		
		if(temp->total_slots_available == 0)
		{
			temp->total_slots_available = 5 + rand()%10;
			// printf("The slots on table %d  are %d \n",temp->identifier,temp->total_slots_available);
			ro->number_of_biryani--;
			temp->want_biryani = false;
			found = true;
			pthread_cond_broadcast(&biryani_filled);
			break;	
		}
		
		temp = temp->next;
	}
	pthread_mutex_unlock(&mutex);
	return found;
}
void ready_to_serve_table(serving_table ** table)
{
		while((*table)->total_slots_available > 0)
		{
			// decide a random number of slots from the total_available_slots
			int random_slot = 1 + rand()%((*table)->total_slots_available);
			
			(*table)->total_slots_available -= random_slot;
			(*table)->slots_currently_available = random_slot;
			// give signal to students
			pthread_mutex_lock(&mutex);
			pthread_cond_broadcast(&slots_available); // students will wait on this condition
			pthread_mutex_unlock(&mutex);
			// now wait for the currently_available_slots to become zero;
			int all_student_took_seat = false;
			while(!all_student_took_seat)
			{
				pthread_mutex_lock(&mutex);
				while((*table)->slots_currently_available!=0 && st->student_finished_eating!=st->total_students)
				{
					errno = pthread_cond_wait(&slots_taken,&mutex); // student will signal slot taken
					if(errno)
						perror("student took seat");
				}
				if((*table)->slots_currently_available > 0)
					random_slot = random_slot - (*table)->slots_currently_available;
				all_student_took_seat = true;
				pthread_mutex_unlock(&mutex);
			}
			// now give signal to start eating to students
			pthread_mutex_lock(&mutex);
			(*table)->can_start_eating = true;
			pthread_cond_broadcast(&start_eating);// students will wait on start eating
			pthread_mutex_unlock(&mutex);

			//check for the condition when all students had biryani
			int all_student_had_biryani = false;
			while(!all_student_had_biryani)
			{
				pthread_mutex_lock(&mutex);
				while((*table)->student_who_ate != random_slot)
				{
					errno = pthread_cond_wait(&stop_eating,&mutex); // DO
					if(errno)
					perror("students ate biryani");
				}
				all_student_had_biryani = true;
				(*table)->can_start_eating = false;
				(*table)->student_who_ate = 0;
				pthread_mutex_unlock(&mutex);
			}
		}
}

void * robots(void * args)
{
	struct robot * ro = (robot *)args;
	int random_sleep = 2 + rand()%4;
	int random_vessel = 1 + rand()%10;
	printf("The robot with id %d is created and is creating %d biryani vessels in %d seconds\n",ro->identifier,random_vessel,random_sleep );
	
	sleep(random_sleep);
	ro->number_of_biryani = random_vessel;
	while(1)
	{
		// robot generates random number of vessels each having random number of eating capacity
		
		while(ro->number_of_biryani > 0)
		{
			//printf("ro->number_of_biryani %d robot %d,OUT\n",ro->number_of_biryani,ro->identifier );
			int res = biryani_ready(ro);
			while(!res)
			{
				//printf("ro->number_of_biryani %d robot %d,IN\n",ro->number_of_biryani,ro->identifier );
				pthread_mutex_lock(&mutex);
				errno = pthread_cond_wait(&biryani_required,&mutex); // the table will signal that it requires biryani
				//printf("request rec for robot %d\n",ro->identifier );
				if(errno)
					perror("robot is making biryani");
				pthread_mutex_unlock(&mutex);
				res = biryani_ready(ro);
			}
			
		}
		random_sleep = 2 + rand()%4;
		random_vessel = 1 + rand()%10;
		//printf("The robot with id %d is creating %d biryani vessels in %d seconds\n",ro->identifier,random_vessel,random_sleep );
		sleep(random_sleep);
		ro->number_of_biryani = random_vessel;
		
	}
	//pthread_mutex_unlock(&mutex);
}
void * tables(void * args)
{
	// total number of the biryani served must be equal to the P(biryani per serving)
	struct serving_table * table = (serving_table *)args;
	// first it has to acquire biryani from the robot chef that has prepared , so wait for it
	while(1)
	{
		pthread_mutex_lock(&mutex);
		int biryani_obtained = false;
		pthread_cond_signal(&biryani_required);
		table->want_biryani = true; // signalling on biryani required
		pthread_mutex_unlock(&mutex);

		while(!biryani_obtained)
		{
			pthread_mutex_lock(&mutex);
			while(table->total_slots_available <= 0)
			{
				errno = pthread_cond_wait(&biryani_filled,&mutex);// waiting till the biryani is filled
				if(errno)
					perror("biryani wait");
			}
			biryani_obtained = true;
			table->want_biryani = false;
			pthread_mutex_unlock(&mutex);
		}
		// now the slots are filled so start distributing the biryani
		ready_to_serve_table(&table);
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

int wait_for_slot(serving_table ** found_table,int * not_found,student * curr_stu)
{
	serving_table * temp = st->serving_table_head;
	while(temp != NULL)
	{
		if(temp->slots_currently_available > 0)
		{
			// send the signal to the table and the table will check the currently available slots
			temp->slots_currently_available--;
			*found_table = temp;
			*not_found = false;
			st->student_finished_eating++;
			errno = pthread_cond_broadcast(&slots_taken);
			if(errno)
				perror("signalling table");
			printf("student with id %d has taken seat on table %d \n",curr_stu->identifier,temp->identifier );
			return true;
		}
		temp = temp->next;
	}
	return false;
}
void student_in_slot(serving_table * found_table)
{
	while(found_table->can_start_eating == false)
	{
		errno = pthread_cond_wait(&start_eating,&mutex); // waiting on start eating
		if(errno) // DO
			perror("found_biryani wait for eating");
	}
}
void * students_eat(void * args)
{
	// here the student will have to wait for the signal given by the table
	// and than will have to check with all the table to find out which one is actually empty
	student* curr_stu = (student *)args;
	int not_found = true;
	serving_table * found_table;
	while(not_found)
	{
		pthread_mutex_lock(&mutex);
		while(!wait_for_slot(&found_table,&not_found,curr_stu))
		{
			errno = pthread_cond_wait(&slots_available,&mutex); 
			if(errno)
				perror("cond_wait error");
		}
		pthread_mutex_unlock(&mutex);
	}
	// wait for the signal from the table to start eating
	int invited_to_eat = false;
	while(!invited_to_eat)
	{
		pthread_mutex_lock(&mutex);
		student_in_slot(found_table);
		printf("student with id %d has started eating on table %d \n",curr_stu->identifier,found_table->identifier );
		invited_to_eat = true;
		pthread_mutex_unlock(&mutex);
	}
	// now you can eat for some random amount of time
	int random_sleep = 3 + rand()%10;
	sleep(random_sleep);
	printf("student with id %d HAD biryani on table with id %d \n",curr_stu->identifier,found_table->identifier);
	// now signal the table that you are done with eating
	pthread_mutex_lock(&mutex);
	found_table->student_who_ate++;   // increasing the number of student who ate
	pthread_cond_broadcast(&stop_eating); // student tell table that they have stopped eating
	pthread_mutex_unlock(&mutex);

	return NULL;
}
void create_students()
{
	// after eating students will go out of the system;
	st->student_threads = (pthread_t *)malloc(st->total_students * sizeof(pthread_t));
	student * temp;
	for (int i = 0; i < st->total_students; ++i)
	{
		struct student * stu = (student *)malloc(sizeof(student));
		stu->identifier = ++student_id;
		if(i == 0)
		{
			st->student_head = stu;
			temp = stu;
		}
		else
		{
			temp->next = stu;
			temp = stu;
		}
		pthread_create(&st->student_threads[i],NULL,&students_eat,(void *)stu);
	}
	// wait for all student threads to return after eating
	for (int i = 0; i < st->total_students; ++i)
	{
		pthread_join(st->student_threads[i],NULL);
	}
	printf("All students finished eating biryani, Lunch over\n");
}
void create_robots()
{
	st->robot_threads = (pthread_t *)malloc(st->total_robots * sizeof(pthread_t));
	struct robot * temp;
	for (int i = 0; i < st->total_robots; ++i)
	{
		robot * ro = (robot *)malloc(sizeof(robot));
		ro->identifier = ++robot_id;
		ro->number_of_biryani = 0;
		ro->next = NULL;
		if(i == 0)
		{
			st->robot_head = ro;
			temp = ro;
		}
		else
		{
			temp->next = ro;
			temp = ro;
		}
		pthread_create(&st->robot_threads[i],NULL,&robots,(void *)ro);
	}
}

void create_tables()
{
	st->table_threads = (pthread_t *)malloc(st->total_serving_tables * sizeof(pthread_t));
	temp_table = NULL;
	for (int i = 0; i < st->total_serving_tables; ++i)
	{
		serving_table * table = (serving_table*)malloc(sizeof(serving_table));
		table->identifier = ++serving_table_id;
		table->can_start_eating = false;
		table->slots_currently_available = 0;
		table->total_slots_available = 0;
		table->student_who_ate = 0;
		table->want_biryani = false;
		table->next = NULL;
		if(i == 0)
		{
			st->serving_table_head = table;
			temp_table = table;
		}
		else
		{
			temp_table->next = table;
			temp_table = table;
		}
		errno = pthread_create(&st->table_threads[i],NULL,&tables,(void *)table); // what should the table do
		if(errno)
			perror("create error");
	}
}
int main()
{
	st = (state * )malloc(sizeof(state));
	printf("give the number of students,robots,tables (in that order)\n");
	scanf("%d %d %d",&st->total_students,&st->total_robots,&st->total_serving_tables);
	st->student_finished_eating = 0;
	st->serving_table_head = NULL;
	st->robot_head = NULL;
	st->student_head = NULL;
	errno = pthread_mutex_init(&(mutex),NULL);
	if(errno)
		perror("mutex init error");
	create_robots();
	create_tables();
	create_students();
	return 0;
}