# Biryani_serving_synchronization
## Synchronization Problem
* This is a project done during my Operating Systems course to understand the fundamentals of multithreading and concurrency.
* The Project is about solving a real life problem that IIIT-Hyderabad faces.
* On sundays the queue for Biryani in Kadamb mess becomes exceedingly large and student have to wait for a long time.
* We model the problem by introducing ** Robot chefs , serving tables and ofcourse the students ** in our solutions as threads.
* The general requirement of the project is that all students must get Biryani and no deadlocks occurs, under some constraints.
* The Table first request the biryani from the robot chef and once a robot fills the vessel of that table, table calls the students to start eating.
* The robot tries to fill the tables until it has biryani vessels left, once they are finished it has to start making other batch of biryani.
* The student has to wait for some table to declare that it has empty slots, once the student gets signal he/she grabs a seat on a table.

## My Implementation
* I have used six condition variables and one mutex.
* The condition variable biryani_required is used by robot chef to wait on and is signalled by the table.
* The condition variable biryani_filled is used by the table to wait on and is signalled by the robot chef.
* The condition variable slots_taken is signalled by student and the table wait on this.
* The condition variable slots_available is signalled by the table and the students waits on it.
* The condition variable start_eating is signalled by the table and is waited upon by the students.
* The condition variable stop_eating is signalled by the students and the table waits for this event.
* First take inputs from the user for students , robots, tables.
* Than create threads for each of the students , robots and tables.
* The students arrive at random time , this is done by making the students thread sleep for random time before calling its method.
* The robots first starts preparing the biryani , the number of biryani and the time to make them is random.
* Once the robot finishes making biryani , it fills an empty table and broadcast signal that biryani is filled.
* Now this signal is received by all the tables and than each table check if the number of total_slots_available.
* Then the table that finds that his slots have increased gets to know that it can start distributing biryani to students.
* Table then makes some random amount of slots available less than the total slots (obviously).
* Then it broadcast the signal to all students which are present in the mess.
* Upon receiving this signal the students who were waiting for this wakes and races to the table, well actually the threads races to grab a seat at the table.
* This depends upon the OS scheduler which students get the seats.
* If the student gets the seat than he prompts about his seat or else he continues waiting for another signal from some table.
* Once all students take their seats on the table or the students in the mess who did not take their seats becomes zero, the table signals all the students to start eating.
* Now each of the student start eating the lunch for some random amount of time and than leaves the mess by printing the appropriate message.
* When all the students have finished eating the lunch, The program stops displaying message about the same.
* This is achieved by waiting for all the students threads to join. 
