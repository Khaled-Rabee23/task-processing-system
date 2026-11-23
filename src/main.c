#define _DEFAULT_SOURCE
/*Sudo Squad 
Team: Sudo Squad
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>

// Constants
#define NUM_UNITS 5 //  how many processing units we have
#define M 10000
#define SLEEP_TIME 500000   // 0.5 second

// TASK STRUCTURE
typedef struct task {
    int id;                    // task identifier
    int value;                 // value being transformed
    int unit_count;            // number of units to visit
    int current_unit_index;    // where the task is right now
    int *unit_id;              // list of units to visit
    struct timespec atime;     // time when task entered system
    struct task *next;         // used for queue linking
} task_t;

// QUEUE STRUCTURE
typedef struct {
    task_t *front;
    task_t *rear;
    pthread_mutex_t mutex;
} task_queue;


task_queue waiting_queue; // Tasks created by the receptor
task_queue unit_queues[NUM_UNITS]; // One queue per unit

atomic_int done_reading = 0; // Receptor finished reading input
atomic_int active_tasks = 0; // How many tasks still exist in the system



//Initialize an empty queue.

void init_queue(task_queue *q) {
    q->front = q->rear = NULL;
    pthread_mutex_init(&q->mutex, NULL);
}


 // Add a task to the end of the queue(FIFO).
 
void enqueue(task_queue *q, task_t *t) {
    t->next = NULL;
    pthread_mutex_lock(&q->mutex);

    if (q->rear)
        q->rear->next = t;
    else
        q->front = t;

    q->rear = t;
    pthread_mutex_unlock(&q->mutex);
}

/*
    Remove a task from the front of the queue.
    Returns NULL if the queue is empty.
 */

task_t* dequeue(task_queue *q) {
    pthread_mutex_lock(&q->mutex);

    task_t *t = q->front;
    if (t) {
        q->front = t->next;
        if (!q->front)
            q->rear = NULL;
    }

    pthread_mutex_unlock(&q->mutex);
    return t;
}

// RECEPTOR THREAD
/*
 Reads tasks from the input file.
 For each task, it builds the structure and pushes it into the waiting queue.
 */
void* receptor(void *arg) {
    FILE *fp = fopen("data/tasks.txt", "r");
    if (!fp) {
        perror("fopen");
        atomic_store(&done_reading, 1);
        return NULL;
    }

    while (1) {
        task_t *t = malloc(sizeof(task_t));
        if (!t) break;

        // read basic task info
        if (fscanf(fp, "%d %d %d",
                   &t->id,
                   &t->value,
                   &t->unit_count) != 3) {
            free(t);
            break;
        }

        // allocate unit path
        t->unit_id = malloc(sizeof(int) * t->unit_count);
        if (!t->unit_id) {
            free(t);
            break;
        }

        // read unit sequence
        int ok = 1;
        for (int i = 0; i < t->unit_count; i++) {
            if (fscanf(fp, "%d", &t->unit_id[i]) != 1) {
                ok = 0;
                break;
            }
        }

        if (!ok) {
            free(t->unit_id);
            free(t);
            break;
        }

        // initialize task state
        t->current_unit_index = 0;
        clock_gettime(CLOCK_MONOTONIC, &t->atime);

        // task is now alive in the system
        atomic_fetch_add(&active_tasks, 1);
        enqueue(&waiting_queue, t);
        printf("[NEW ] Task %d entered system (units=%d)\n",
        t->id, t->unit_count);

    }

    fclose(fp);
    atomic_store(&done_reading, 1);
    return NULL;
}

// DISPATCHER THREAD
/*
Takes tasks from the waiting queue and sends them to the correct unit queue.
 */
void* dispatcher(void *arg) {
    while (1) {
        task_t *t = dequeue(&waiting_queue);

        if (!t) {
            pthread_mutex_lock(&waiting_queue.mutex);
            int empty = (waiting_queue.front == NULL);
            pthread_mutex_unlock(&waiting_queue.mutex);

            if (atomic_load(&done_reading) && empty)
                break;

            usleep(1000);
            continue;
        }

        int u = t->unit_id[t->current_unit_index];
        if (u >= 0 && u < NUM_UNITS){
        printf("[DISP] Task %d → Unit %d\n",
        t->id, u);

            enqueue(&unit_queues[u], t);
        }
        else {
            // invalid unit id, drop task
            free(t->unit_id);
            free(t);
            atomic_fetch_sub(&active_tasks, 1);
        }
    }
    return NULL;
}

//UNIT WORKER THREAD
/*
  Each unit has exactly one worker thread.
  The worker processes tasks and forwards them to the next unit or finishes them.
 */
void* unit_worker(void *arg) {
    int unit_id = *(int *)arg;

    while (1) {
        task_t *t = dequeue(&unit_queues[unit_id]);

        if (!t) {
            if (atomic_load(&done_reading) &&
                atomic_load(&active_tasks) == 0)
                break;

            usleep(1000);
            continue;
        }

        // unit-specific work
        switch (unit_id) {
            case 0:
                t->value = (t->value + 7) % M;
                break;
            case 1:
                t->value = (t->value * 2) % M;
                break;
            case 2: {
                long long r = 1, v = t->value;
                for (int i = 0; i < 5; i++)
                    r = (r * v) % M;
                t->value = (int)r;
                break;
            }
            case 3:
                t->value = 19 - t->value;
                break;
            case 4: {
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);

                double duration =
                    (now.tv_sec - t->atime.tv_sec) +
                    (now.tv_nsec - t->atime.tv_nsec) / 1e9;

            /* printf("Task ID = %d - Total time= %.3f sec - Calculated value= %d\n",
                        t->id, duration, t->value); */
                        printf("[DONE] Task %d finished after %.3f sec | value=%d | last unit=%d\n",
        t->id, duration, t->value, unit_id);


                break;
            }
        }

        usleep(SLEEP_TIME);
        t->current_unit_index++;

        // send to next unit or finish task
        if (t->current_unit_index < t->unit_count) {
            int next = t->unit_id[t->current_unit_index];
            if (next >= 0 && next < NUM_UNITS)
                enqueue(&unit_queues[next], t);
            else {
                free(t->unit_id);
                free(t);
                atomic_fetch_sub(&active_tasks, 1);
            }
        } else {
            free(t->unit_id);
            free(t);
            atomic_fetch_sub(&active_tasks, 1);
        }
    }
    return NULL;
}

// MAIN FUNCTION
int main() {
    pthread_t receptor_t, dispatcher_t, unit_t[NUM_UNITS];
    int unit_ids[NUM_UNITS];

    init_queue(&waiting_queue);
    for (int i = 0; i < NUM_UNITS; i++)
        init_queue(&unit_queues[i]);

    pthread_create(&receptor_t, NULL, receptor, NULL);
    pthread_create(&dispatcher_t, NULL, dispatcher, NULL);

    for (int i = 0; i < NUM_UNITS; i++) {
        unit_ids[i] = i;
        pthread_create(&unit_t[i], NULL, unit_worker, &unit_ids[i]);
    }

    pthread_join(receptor_t, NULL);
    pthread_join(dispatcher_t, NULL);
    for (int i = 0; i < NUM_UNITS; i++)
        pthread_join(unit_t[i], NULL);

    printf("All tasks are done. Thanks for using our system :) \n");
    return 0;
}
