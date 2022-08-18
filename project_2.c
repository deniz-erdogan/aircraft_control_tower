#include <pthread.h>
#include "queue.c"
#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include <sys/time.h>


int simulationTime = 20;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 40; // frequency of emergency
float p = 0.2;               // probability of a ground job (launch & assembly)
int t = 2;
int n_log_time = 10;
int emergency_40_t_counter;

void* LandingJob(int thread_id);
void* LaunchJob(int thread_id);
void* EmergencyJob(int thread_id);
void* AssemblyJob(int thread_id);
void* ControlTower(void *arg);

void* Pad_A();
void* Pad_B();


pthread_mutex_t mutex_landing_queue; //
pthread_mutex_t mutex_launch_queue;
pthread_mutex_t mutex_assembly_queue;
pthread_mutex_t mutex_emergency_queue;

pthread_mutex_t mutex_padA;
pthread_mutex_t queue_switch;
pthread_mutex_t mutex_padB;
pthread_mutex_t mutex_log_file;


Queue *landing_queue, *launch_queue, *assembly_queue, *emergency_queue;
Queue *pada, *padb;
Queue *pada_landing, *padb_landing;

pthread_t rocket_thread[1024];
Job jobs[1024];
int thread_count = 0;
int rolling_thread_ids = 0;

struct arg_struct
{
    time_t arg1;
    int arg2;
}*args;

float generate_p();

pthread_attr_t attr;

time_t start_time;
time_t current_time;
FILE *events;

// pthread sleeper function
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;

    pthread_mutex_lock (&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock (&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);

    //Upon successful completion, a value of zero shall be returned
    return res;
}

int main(int argc,char **argv){
    // -p (float) => sets p
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-p")) {p = atof(argv[++i]);}
        else if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-n")) {n_log_time = atoi(argv[++i]);}
    }

    /* set the timers */
    start_time = time(NULL);
    current_time = time(NULL);

    srand(seed); // feed the seed
    pthread_mutex_init(&mutex_assembly_queue, NULL);
    pthread_mutex_init(&mutex_landing_queue, NULL);
    pthread_mutex_init(&mutex_launch_queue, NULL);
    pthread_mutex_init(&mutex_log_file, NULL);
    pthread_mutex_init(&mutex_emergency_queue, NULL);
    pthread_mutex_init(&queue_switch, NULL);

    // open event.log file

    events = fopen("events.log", "w");
    fprintf(events, "|%-10s|%-10s|%-15s|%-10s|%-15s|%-10s \n","EventID","Status", "RequestTime", "EndTime","TurnaroundTime", "Pad");




    /* Queue usage example
        Queue *myQ = ConstructQueue(1000);
        Job j;
        j.ID = myID;
        j.type = 2;
        Enqueue(myQ, j);
        Job ret = Dequeue(myQ);
        DestructQueue(myQ);
    */

    // your code goes here
    landing_queue = ConstructQueue(1000);
    launch_queue = ConstructQueue(1000);
    assembly_queue = ConstructQueue(1000);
    emergency_queue = ConstructQueue(1000);
    pada = ConstructQueue(500);
    padb = ConstructQueue(500);
    pada_landing = ConstructQueue(500);
    padb_landing = ConstructQueue(500);



    Job deneme;
    deneme.type = 1;
    deneme.ID = 31;
    deneme.arrival_time = time(NULL);
    Enqueue(pada, deneme);



    /* initialization thread attr */
    pthread_attr_init(&attr);



    int job_index = 0;

    /* formatted time for start time. */
    struct tm tm_start = *localtime(&start_time);

    printf("-----------%02d:%02d:%02d - ", tm_start.tm_hour, tm_start.tm_min, tm_start.tm_sec);
    printf("Running Simulation... s: %d - p: %f-----------\n", seed, p);


    pthread_t tower;
    printf("Starting tower thread. \n");
    pthread_create(&tower, &attr, ControlTower, NULL);

    pthread_t padA;
    printf("Starting PAD:A thread. \n");
    pthread_create(&padA, &attr, Pad_A, NULL);

    pthread_t padB;
    printf("Starting PAD:B thread. \n");
    pthread_create(&padB, &attr, Pad_B, NULL);


    float current_p;
    int current_second;
    NODE *curr;
    while (time(NULL) < (start_time + simulationTime)) {
//        if (((time(NULL) - start_time) >= n_log_time)) {
//            current_second = time(NULL) - start_time;
//            printf("At %d sec landing :", current_second);
//            curr = pada_landing->head;
//            while (curr->prev != NULL && curr != NULL) {
//                printf("%d ", curr->data.ID);
//                curr = curr->prev;
//            }
////            printf("%d ", curr->data.ID);
//            printf("At %d sec launch :", current_second);
//
//            printf("At %d sec assembly :\n", current_second);
//        }
        pthread_sleep(t);
        emergency_40_t_counter++;


        current_p = generate_p();
        if(emergency_40_t_counter == 5){
            emergency_40_t_counter = 0;
//            printf("Emergency yapma zamani geldi usta. \n");
            pthread_create(&rocket_thread[thread_count], &attr, EmergencyJob, rolling_thread_ids );
            rolling_thread_ids++;
            rolling_thread_ids++;
            thread_count++;
            thread_count++;
        }
        if (current_p < (p/2)) {
            //Launch
//            printf("Launch yapma zamani geldi usta. \n");
            pthread_create(&rocket_thread[thread_count], &attr, LaunchJob, rolling_thread_ids );
            rolling_thread_ids++;
            thread_count++;

        } else if (current_p < p) {
            //Assembly
//            printf("Assembly yapma zamani geldi usta. \n");
            pthread_create(&rocket_thread[thread_count], &attr, AssemblyJob, rolling_thread_ids );
            rolling_thread_ids++;
            thread_count++;
        } else{
//            printf("Landing yapma zamani geldi usta. \n");
            pthread_create(&rocket_thread[thread_count], &attr, LandingJob, rolling_thread_ids );
            rolling_thread_ids++;
            thread_count++;
        }
    }




    pthread_join(tower, NULL);
    pthread_join(padA, NULL);
    pthread_join(padB, NULL);

    fclose(events);


    return 0;
}

void* Pad_A() {
    //does launch and landing
    char status[10];
    char pad_name[5];
    strcpy(pad_name, "A");
    Job job;
    int job_done;
    while (time(NULL) < start_time + simulationTime) {
        job_done = 0;

        if (isEmpty(pada_landing)) {// No landing jobs
            if (isEmpty(pada)) { // No jobs at all
                pthread_sleep(2);
                job_done = 0;
            }
            else{ // No landing but other
                job = Dequeue(pada);
                // simulate doing the job -> the pad is occupied
                if (job.type == 1) {
//                    printf("PAD A : FOUND JOB. JOB ID IS: %d. JOB TYPE IS: %d\n", job.ID, job.type);
                    pthread_sleep(2 * t);
                    job_done = 1;
                }
                else {
                    pthread_sleep(t);
                    job_done = 1;
                }
            }

        } else { //There are landing jobs
            job = Dequeue(pada_landing);
//            printf(" PAD A : Taken LANDING job, JOB ID is: %d \n", job.ID);
            pthread_sleep(t);
            job_done = 1;
        }


        if (job_done) {
            job.end_time = (time(NULL) - start_time);
            job.turnaround_time = job.end_time - (job.arrival_time - start_time);

            if (job.type == 1) {
                strcpy(status, "D");

            } else if (job.type == 0){
                strcpy(status, "L");
            } else {
                strcpy(status, "E");
            }

            fprintf(events, "|%-10d|%-10s|%-15d|%-10d|%-15d|%-10s\n", job.ID, status, (job.arrival_time- start_time), job.end_time,
                    job.turnaround_time,pad_name);
            

        }

    }
}


void* Pad_B() {
    //does assembly and landing
    char status[10];
    char pad_name[5];
    strcpy(pad_name, "B");
    Job job;
    int job_done;
    while (time(NULL) < start_time + simulationTime) {
        job_done = 0;
        if (isEmpty(padb_landing)) {
            if (isEmpty(padb)) {
                pthread_sleep(2);
                job_done = 0;
            } else {
                job = Dequeue(padb);
                job_done = 1;
                // simulate doing the job -> the pad is occupied
//                printf("PAD B : FOUND JOB. JOB ID IS: %d JOB TYPE IS: %d\n", job.ID, job.type);
                if (job.type == 2) {
                    pthread_sleep(6 * t);
                    job_done = 1;
                } else {
                    pthread_sleep(t);
                    job_done = 1;
                }

            }
        } else {

            job = Dequeue(padb_landing);
            job_done = 1;
//            printf(" PAD B : Taken LANDING job.\n");
            if (job.type != 2){
                pthread_sleep(t);
            } else if (job.type == 2) {
                pthread_sleep(6 * t);
            }

        }

        if (job_done) {
            job.end_time = (time(NULL) - start_time);
            job.turnaround_time = job.end_time - (job.arrival_time - start_time);

            if (job.type == 2) {
                strcpy(status, "A");

            } else if (job.type == 0){
                strcpy(status, "L");

            } else {
                strcpy(status, "E");
            }

            fprintf(events, "|%-10d|%-10s|%-15d|%-10d|%-15d|%-10s\n", job.ID, status, (job.arrival_time- start_time), job.end_time,
                    job.turnaround_time,pad_name);

        }
    }
}

// the function that creates plane threads for landing
void* LandingJob(int thread_id){
    Job new_job;
//    printf("LANDINGJOB CALISIYOR USTA : %d \n", thread_id);
    new_job.ID =thread_id;
    new_job.type = 0;
    new_job.arrival_time = time(NULL);

    //    printf("WHEN CREATING ARRIVAL: ");
    //    struct tm tm_start = *localtime(new_job.arrival_time);
    //    printf("-----------%02d:%02d:%02d - ", tm_start.tm_hour, tm_start.tm_min, tm_start.tm_sec);
    pthread_mutex_lock(&mutex_landing_queue);
    Enqueue(landing_queue, new_job);
    pthread_mutex_unlock(&mutex_landing_queue);



    pthread_exit(NULL);
}

// the function that creates plane threads for departure
void* LaunchJob(int thread_id){

    Job new_job;
//    printf("LAUNCHJOB CALISIYOR USTA : %d \n", thread_id);
    new_job.ID =thread_id;
    new_job.type = 1;
    new_job.arrival_time = time(NULL);
    pthread_mutex_lock(&mutex_launch_queue);
    Enqueue(launch_queue, new_job);
    pthread_mutex_unlock(&mutex_launch_queue);



    pthread_exit(NULL);
}

// the function that creates plane threads for emergency landing
void* EmergencyJob(int thread_id){
    Job new_job;
    Job new_job2;
//    printf("EMERGENCY JOB CALISIYOR USTA : %d, %d \n", thread_id,thread_id+1);
    new_job.ID =thread_id;
    new_job.type = 3;
    new_job2.ID =thread_id +1;
    new_job2.type = 3;
    new_job.arrival_time = time(NULL);
    new_job2.arrival_time = time(NULL);

    pthread_mutex_lock(&mutex_emergency_queue);
    Enqueue(emergency_queue, new_job);
    Enqueue(emergency_queue, new_job2);
    pthread_mutex_unlock(&mutex_emergency_queue);



    pthread_exit(NULL);
}

// the function that creates plane threads for emergency landing
void* AssemblyJob(int thread_id){
    Job new_job;
//    printf("ASSEMBLY CALISIYOR USTA : %d \n", thread_id);
    new_job.ID =thread_id;
    new_job.type = 2;
    new_job.arrival_time = time(NULL);
    pthread_mutex_lock(&mutex_assembly_queue);
    Enqueue(assembly_queue, new_job);
    pthread_mutex_unlock(&mutex_assembly_queue);


    pthread_exit(NULL);
}

// the function that controls the air traffic
void* ControlTower(void *arg) {
    Job j;
    Job j2;
    int queue_switch = 0;
    Queue *temp;
    while (time(NULL) < start_time + simulationTime) {


        //
        //        determine which queue(s) to take job(s) from
        if (!isEmpty(emergency_queue)) { // There are emergency jobs
            j = Dequeue(emergency_queue);
//            printf("Dequed Emergency JOB ID %d \n", j.ID);
            Enqueue(padb_landing, j);
            j2 = Dequeue(emergency_queue);
//            printf("Dequed Emergency JOB ID %d \n", j2.ID);
            Enqueue(pada_landing, j2);
        } else {
            if (!isEmpty(landing_queue)) { // There are landing jobs
                if (pada_landing->size > padb_landing->size) {// Give job to Pad B queue
                    j = Dequeue(landing_queue);
                    Enqueue(padb_landing, j);
                } else { // Give job to Pad A queue
                    j = Dequeue(landing_queue);
                    Enqueue(pada_landing, j);
                }
            } else {
                if (!isEmpty(launch_queue)) {
                    j = Dequeue(launch_queue);
                    Enqueue(pada, j);
                }

                if (!isEmpty(assembly_queue)) {
                    j = Dequeue(assembly_queue);
                    Enqueue(padb, j);

                }
            }
            //        modify the logs

            pthread_sleep(2);
        }

        // check if queue switching is necessary
        // make the change if the flag was not set
        // flag the change if swapped
        ////////////////////////////////MUTEX HERE/////////////////////////////
        pthread_mutex_lock(&queue_switch);
        if ((pada_landing->size + padb_landing->size) > ((pada->size + padb->size) * 4) && !queue_switch) {
            temp = pada_landing;
            pada_landing = pada;
            pada = temp;

            temp = padb_landing;
            padb_landing = padb;
            padb = padb_landing;

            queue_switch = 1;
            printf("QUEUE SWITCH!\n");
        } else if ((pada_landing->size + padb_landing->size) < ((pada->size + padb->size) * 4) && queue_switch) {
            // switch them back
            temp = pada_landing;
            pada_landing = pada;
            pada = temp;

            temp = padb_landing;
            padb_landing = padb;
            padb = padb_landing;

            queue_switch = 0;
            printf("SWITCH BACK!\n");
        }
        pthread_mutex_unlock(&queue_switch);
    }
}

float generate_p() {
    return (float) rand() / (float) RAND_MAX;
}

