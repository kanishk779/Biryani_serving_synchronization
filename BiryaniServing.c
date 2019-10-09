#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<pthread.h>
#include<time.h>
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
	pthread_mutex_t lock;
}robot;
typedef struct serving_table
{
	struct serving_table *next;
	int identifier;
	int filled_slots; // this can only take two values either (completely filled) or empty
	pthread_mutex_t lock;
}serving_table;
enum student_state{eating,hungry,finished};
typedef struct student
{
	struct student *next;
	int identifier;
	int apetite;    // state
}student;
typedef struct program_state
{
	int total_students;
	int total_robots;
	int total_serving_tables;
	int students_per_vessel;
	int student_finished_eating;
	pthread_t * robot_threads;
	pthread_t * table_threads;
	pthread_t * student_threads;
	robot * robot_head;
	serving_table * serving_table_head;
	student * student_head;
}state;
state st;
void biryani_ready(struct robot * r)
{
	// called by the chef if the biryani is ready, 
	// waiting for a table to become empty .
	// so call wait on all the serving_table, to fill all the vessels generated by robot
	// robot can give (fill vessel) biryani to a table only if the semaphore of table is 0
	// meaning that there is no biryani left in the table
	
	int all_biryani_served = false;
	while(1)
	{
		serving_table * temp = st.serving_table_head;
		while(temp != NULL)
		{
			if(r->number_of_biryani == 0)
			{
				printf("The robot with id %d finished distributing all the biryani's\n", r->identifier);
				all_biryani_served = true;
				break;
			}
			pthread_mutex_lock(&(temp->lock));
			// just check the value of semaphore of table
			if(temp->filled_slots == 0)
			{
				// fill this table
				printf("The robot with id %d has filled table with id %d and now has %d remaining biryani vessels\n",r->identifier,temp->identifier,r->number_of_biryani-1 );
				temp->filled_slots += st.students_per_vessel;
				// now the table becomes ready to serve
				// notice that there is no need to lock the robot because each robot 
				// is decreasing its own number of biryani, hence it is not a shared variable 
				r->number_of_biryani--;
			}
			pthread_mutex_unlock(&(temp->lock));
		}
		if(all_biryani_served)
			break;
	}
	return ;
}
void ready_to_serve(struct serving_table * table)
{
	// the table will be serving the biryani once it has got its
	// vessel filled by the robot and this function will not return 
	// till all the biryani portions are eaten up
	pthread_mutex_lock(&(table->lock));
	// decrease the number of the filled slots if the students are eating the biryani
	if(table->filled_slots > 0)
	{
		// students can eat biryani (give signal) here the table will specify the number of slots available
	}
	pthread_mutex_unlock(&(table->lock));
}
void * tables(void * args)
{
	// total number of the biryani served must be equal to the P(biryani per serving)
	struct serving_table * table = (serving_table *)args;
	printf("the table with id %d is created\n",table->identifier );
	// i don't think this function will do anything
	return NULL;
}
void * robots(void * args)
{
	struct robot * ro = (robot *)args;
	printf("The robot with id %d is created\n",ro->identifier );
	while(1)
	{
		// come here and start making a random number of biryani
		int random_biryani = rand()%10;
		// take some time to cook those;
		int random_time_to_cook = rand()%5;
		printf("The robot with id %d takes %d time to make %d biryani vessels\n",ro->identifier,random_time_to_cook,random_biryani );
		sleep(random_time_to_cook);

		// now increase the available biryani of the respective chef
		pthread_mutex_lock(&(ro->lock));
		ro->number_of_biryani += random_biryani;
		pthread_mutex_unlock(&(ro->lock));
		//now the robot will only make the batch of biryani when this batch is completely distributed

		// HANDLE THIS CASE the below function will not until all the biryanis are finished
		biryani_ready(ro);
	}
}
void * students_eat(void * args)
{
	// poll all the eating tables whether they have biryani 
	serving_table *  temp = st.serving_table_head;
	int had_biryani = false;
	student* curr_stu = (student *)args;
	while(temp != NULL)
	{
		pthread_mutex_lock(&(temp->lock));
		if(temp->filled_slots > 0)
		{
			temp->filled_slots--;
			// the student got a place now
			had_biryani = true;
			
			break;
		}
		pthread_mutex_unlock(&(temp->lock));
	}
	if(had_biryani)
	{
		st.student_finished_eating++;
		curr_stu->apetite = eating;
		// sleep for sometime and than finish
		int random_sleep = 3 + rand()%10;
		sleep(random_sleep);
		curr_stu->apetite = finished;
		printf("student with id %d had biryani\n",curr_stu->identifier);
		
	}
	return NULL;
}
void create_students()
{
	// after eating students will go out of the system;
	st.student_threads = (pthread_t *)malloc(st.total_students * sizeof(pthread_t));
	struct student * temp;
	for (int i = 0; i < st.total_students; ++i)
	{
		struct student * stu = (student *)malloc(sizeof(student));
		stu->identifier = ++student_id;
		stu->apetite = hungry;
		if(i == 0)
		{
			st.student_head = stu;
			temp = stu;
		}
		else
		{
			temp->next = stu;
			temp = stu;
		}
		pthread_create(&st.student_threads[i],NULL,&students_eat,(void *)stu);
	}
	// wait for all student threads to return after eating
	for (int i = 0; i < st.total_students; ++i)
	{
		pthread_join(st.student_threads[i],NULL);
	}
	printf("All students finished eating biryani, Lunch over\n");
}
void create_robots()
{
	st.robot_threads = (pthread_t *)malloc(st.total_robots * sizeof(pthread_t));
	struct robot * temp;
	for (int i = 0; i < st.total_robots; ++i)
	{
		struct robot * ro = (robot *)malloc(sizeof(robot));
		ro->identifier = ++robot_id;
		if(i == 0)
		{
			st.robot_head = ro;
			temp = ro;
		}
		else
		{
			temp->next = ro;
			temp = ro;
		}
		pthread_create(&st.robot_threads[i],NULL,&robots,(void *)ro);
	}
}
void create_tables()
{
	st.table_threads = (pthread_t *)malloc(st.total_serving_tables * sizeof(pthread_t));
	struct serving_table * temp;
	for (int i = 0; i < st.total_serving_tables; ++i)
	{
		struct serving_table * table = (serving_table*)malloc(sizeof(serving_table));
		table->identifier = ++serving_table_id;
		table->filled_slots = 0;
		if(i == 0)
		{
			st.serving_table_head = table;
			temp = table;
		}
		else
		{
			temp->next = table;
			temp = table;
		}
		pthread_create(&st.table_threads[i],NULL,&tables,(void *)table); // what should the table do
	}
}
int main()
{
	printf("give the number of students,robots,tables,students_per_vessel (in that order)\n");
	scanf("%d %d %d %d",&st.total_students,&st.total_robots,&st.total_serving_tables,&st.students_per_vessel);
	st.student_finished_eating = 0;
	create_tables();
	create_robots();
	create_students();

	// note that we are only waiting for the student threads to return ,not for
	// robots or the serving table
	return 0;
}