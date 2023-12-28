#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include<semaphore.h>
#include <time.h>

struct personal {
    char group; // group identifier ('A' or 'B')
    char driver; // indicator if the person is a driver
};

// custom semaphore structure
typedef struct {
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} custom_sem_t;


pthread_mutex_t lock;
pthread_barrier_t barrier;

// Team A:
custom_sem_t semTeamA;
int groupACnt = 0;

// Team B:
custom_sem_t semTeamB;
int groupBCnt = 0;


int captainID = 0; // counter for captain's ID


// Function to initialize custom semaphore
void custom_sem_init(custom_sem_t *sem, int pshared, int value) {
    sem->value = value;
    pthread_mutex_init(&sem->mutex, NULL);
    pthread_cond_init(&sem->cond, NULL);
}

// function to perform semaphore post (increment)
void custom_sem_post(custom_sem_t *sem) {
    pthread_mutex_lock(&sem->mutex);
    sem->value++;
    pthread_cond_signal(&sem->cond);
    pthread_mutex_unlock(&sem->mutex);
}

// function to perform semaphore wait (decrement)
void custom_sem_wait(custom_sem_t *sem) {
    pthread_mutex_lock(&sem->mutex);
    while (sem->value <= 0) {
        pthread_cond_wait(&sem->cond, &sem->mutex);
    }
    sem->value--;
    pthread_mutex_unlock(&sem->mutex);
}

// function to destroy custom semaphore
void custom_sem_destroy(custom_sem_t *sem) {
    pthread_mutex_destroy(&sem->mutex);
    pthread_cond_destroy(&sem->cond);
}

// function to assign a team to a person
void assign_team(struct personal *person, int *groupACnt, int *groupBCnt, 
                 custom_sem_t **thisTeam_sem, custom_sem_t **elseTeam_sem) {
    if (person->group == 'A') {
        (*groupACnt)++;
        *thisTeam_sem = &semTeamA;
        *elseTeam_sem = &semTeamB;
    } else if (person->group == 'B') {
        (*groupBCnt)++;
        *thisTeam_sem = &semTeamB;
        *elseTeam_sem = &semTeamA;
    }
}

// thread function to handle each person's actions
void* threads(void * argc) {

    pthread_mutex_lock(&lock);

    struct personal * person = (struct personal*) argc;
	
    printf("Thread ID: %ld, Team: %c, I am looking for a car\n", pthread_self(), person -> group);

    custom_sem_t *thisTeam_sem;
    custom_sem_t *elseTeam_sem;

	assign_team(person, &groupACnt, &groupBCnt, &thisTeam_sem, &elseTeam_sem);

    // to determine if the person becomes a driver or not
    if(groupACnt==4||groupBCnt==4){
        person->driver = '+';
        groupACnt = groupACnt % 4;
        groupBCnt = groupBCnt % 4;

		int k = 0;
		while (k < 3){
			custom_sem_post(thisTeam_sem);
			k = k + 1;
		}        
    }

    else if (groupACnt >= 2 && groupBCnt >= 2){

        person->driver = '+';
        groupACnt = groupACnt % 2;
        groupBCnt = groupBCnt % 2;

        custom_sem_post(thisTeam_sem);

		int m = 0;
		while (m < 2){
			custom_sem_post(elseTeam_sem);
			m = m + 1;
		}
    }
    else{
        pthread_mutex_unlock(&lock);
        custom_sem_wait(thisTeam_sem);
    }
    
    printf("Thread ID: %ld, Team: %c, I have found a spot in a car\n", pthread_self(), person->group);
    pthread_barrier_wait(&barrier);

    if(person->driver == '+') {
		
        printf("Thread ID: %ld, Team: %c, I am the captain and driving the car with ID %d\n", pthread_self(), person->group, captainID);
		captainID = captainID + 1;
        pthread_mutex_unlock(&lock);
    }
}


int main(int argc, char* argv[]){

	int counter;
	int groupA;
	groupA = atoi(argv[1]);
	int groupB;
	groupB = atoi(argv[2]);
	int all = groupA + groupB;


	if (argc == 3){
		if (groupA % 2 != 1 && groupB % 2 != 1){
			if (all % 4 == 0){

				pthread_t threadList[all];
				struct personal semList[all];

				custom_sem_init(&semTeamA, 0, 0);
				custom_sem_init(&semTeamB, 0, 0);
				pthread_barrier_init(&barrier, NULL, 4);

				counter = 0;
				while (counter < groupA){
					semList[counter].group = 'A'; 
					counter = counter + 1;
				}
				counter= groupA;
				while (counter < all){
					semList[counter].group = 'B'; 
					counter = counter + 1;
				}

				for (counter = 0; counter < all; counter++){
					semList[counter].driver='-';
					if (pthread_create(&threadList[counter], NULL, threads, &semList[counter]) != 0) {
						perror("Failed to create thread\n");
					return -1;
					}
				}

				counter = 0;
				while (counter < all){
					pthread_join(threadList[counter], NULL);
					counter = counter + 1;
				}
				pthread_barrier_destroy(&barrier);
				custom_sem_destroy(&semTeamA);
				custom_sem_destroy(&semTeamB);
			}
		}
	}

	printf("The main terminates\n");
	return 0;

}
