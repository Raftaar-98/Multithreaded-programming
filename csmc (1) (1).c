#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <assert.h>

void printQueue();

//global variable declarations

int max_chairs,initialChairs;
int max_tutors;
int max_students;
int help;
int studentCount = 0;
int curStudentId=0;
int totalRequests;
int currentlyTutored =0;
int totalSessions;
int done=0;

sem_t studentEnteringQueue;
sem_t priorityStudentQueue;

pthread_mutex_t chairNumMutex;
pthread_mutex_t studentCountMutex;
pthread_mutex_t studentQueueMutex;
pthread_mutex_t totalSessionsMutex;
pthread_mutex_t doneMutex;
pthread_mutex_t curStudentIdMutex;


//struct declarations
typedef struct student_st
{
  int help_taken;
  int stu_id;
  int helping_tutor;
  int getHelp;
}student_st;
student_st* stuPtr;

typedef struct tutor_st
{
  int tutor_id;
}tutor_st;
tutor_st* tutorPtr;


typedef struct queue
{
  int id;
  struct queue* next;
}queue;
queue* priorityQueue;
pthread_mutex_t priorityQueueMutex;

int* studentQueue;


void* student(void* stPtr)
{
  student_st* stData = (student_st*) stPtr;

  
    
  while(1)
  {
    
    if(stData->help_taken == help)
    {
      pthread_mutex_lock(&doneMutex);
      done++;
      pthread_mutex_unlock(&doneMutex);
      
      sem_post(&studentEnteringQueue);
   
      pthread_exit(NULL);
    }
    
    float programTime=(float)(rand()%200)/100;
    usleep(programTime);
    
    pthread_mutex_lock(&chairNumMutex);
    if(max_chairs <=0)
    {
      printf("St: Student %d found no empty chair. Will try again later.\n",stData->stu_id);
      pthread_mutex_unlock(&chairNumMutex);
      continue;
    }
    max_chairs--;
    printf("St: Student %d takes a seat. Empty chairs = %d\n",stData->stu_id,max_chairs);
    totalRequests++;
    pthread_mutex_unlock(&chairNumMutex);
    
    
   
    pthread_mutex_lock(&studentQueueMutex);
    studentQueue[stData->stu_id] = 1;
    stData->getHelp = 1;
    pthread_mutex_unlock(&studentQueueMutex);
    
    pthread_mutex_lock(&curStudentIdMutex);
    curStudentId = stData->stu_id;
  
    
    sem_post(&studentEnteringQueue);
    
    while(stuPtr[(stData->stu_id)-1].helping_tutor == 0);
    
    printf("St: Student %d received help from Tutor %d.\n",stData->stu_id,stData->helping_tutor);
    stuPtr[(stData->stu_id)-1].helping_tutor = 0;
    stuPtr[(stData->stu_id)-1].help_taken += 1;
    
    
  }
  return NULL;
}

void* tutor(void* tutorPtr)
{

  tutor_st* tutorData = (tutor_st*) tutorPtr;

  
  while(1)
  { 
    if(done==max_students)
    {
     
      pthread_exit(NULL);
    }
    
   
    sem_wait(&priorityStudentQueue);
    
    pthread_mutex_lock(&priorityQueueMutex);
    queue* curStudent = priorityQueue;
    if(curStudent!=NULL)
    {
      priorityQueue = priorityQueue->next;
      pthread_mutex_unlock(&priorityQueueMutex);
      pthread_mutex_lock(&chairNumMutex);
      max_chairs++;
      currentlyTutored++;
      pthread_mutex_unlock(&chairNumMutex);
      
      float tutorTime=(float)(rand()%200)/1000;
      usleep(tutorTime);
      
      pthread_mutex_lock(&totalSessionsMutex);
      currentlyTutored--;
      totalSessions++;
      printf("Tu: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n",curStudent->id,tutorData->tutor_id,currentlyTutored,totalSessions);
      pthread_mutex_unlock(&totalSessionsMutex);
      
      stuPtr[(curStudent->id)-1].helping_tutor = tutorData->tutor_id;

      
      pthread_mutex_lock(&studentQueueMutex);
      studentQueue[curStudent->id] = 0;
      pthread_mutex_unlock(&studentQueueMutex);
    
    }
    
    else
      pthread_mutex_unlock(&priorityQueueMutex);
  }
  
  return NULL;
}

void* coordinator()
{
 
  int curStudent;
  while(1)
  {
    if(done==max_students)
    {
  
      
      for(int i=0;i<max_tutors;i++)
        sem_post(&priorityStudentQueue);
        
      pthread_exit(NULL);
    }
    
  
    sem_wait(&studentEnteringQueue);

    curStudent = curStudentId;
    pthread_mutex_unlock(&curStudentIdMutex);
   
    
    
    if(curStudent ==0 || stuPtr[curStudent-1].getHelp!=1)
      continue;
   
  
    
    stuPtr[curStudent-1].getHelp = 0;
    //add to priority queue;
    queue* node = malloc(sizeof(queue));
    
    pthread_mutex_lock(&priorityQueueMutex);
    if(priorityQueue == NULL)
    {
      node->id = curStudent;
      node->next = NULL;
      priorityQueue = node;
    
    }
    else
    {
      queue* head = priorityQueue;
      node->id = curStudent;
      while(head->next != NULL && stuPtr[(head->id) - 1].help_taken < stuPtr[curStudent-1].help_taken)
      {
      
          head = head->next;
      }
      //if position is at the end
      if(head->next == NULL)
      { 
        node->next = NULL;
        head->next = node;
      }
      //if position is somewhere in between
      else
      {
        node->next = head->next;
        head->next = node;
      }
    }
    
    printQueue();
    pthread_mutex_lock(&chairNumMutex);
    printf("Co: Student %d with priority %d in the queue. Waiting students now = %d. Total requests = %d\n",curStudent,stuPtr[curStudent-1].help_taken,initialChairs-max_chairs,totalRequests);
    pthread_mutex_unlock(&chairNumMutex);
    
    pthread_mutex_unlock(&priorityQueueMutex);
    
    sem_post(&priorityStudentQueue);
    
  }

  return NULL;
}

void printQueue()
{
  queue* head = priorityQueue;
  while(head != NULL)
  {
  
    head = head->next;
  }
}

int main(int argc, const char * argv[])
{
	int i;
	
	
	max_students = atoi(argv[1]);
	max_tutors = atoi(argv[2]);
	max_chairs = atoi(argv[3]);
  initialChairs = max_chairs;
	help = atoi(argv[4]);
	
	pthread_t student_thds[max_students];
	pthread_t tutor_thds[max_tutors];
	pthread_t coordinator_thd;
  
  stuPtr = malloc(sizeof(student_st) * 2*max_students);
  studentQueue = calloc(2*max_students,sizeof(int));

  tutorPtr = malloc(sizeof(tutor_st) * 2*max_tutors);
 
  priorityQueue = NULL;
  
	
	for(i=0;i<max_students;i++)
	{
    stuPtr[i].stu_id = i+1;
    stuPtr[i].help_taken = 0;
    
    pthread_create(&student_thds[i],NULL,student,(void*)&stuPtr[i]);
	}
	
	for(i = 0 ; i < max_tutors ; i++)
	{
		tutorPtr[i].tutor_id = i+1;
    pthread_create(&tutor_thds[i], NULL, tutor, (void*)&tutorPtr[i]);
	}

	pthread_create(&coordinator_thd, NULL, coordinator, NULL);
	
	//join threads
  pthread_join(coordinator_thd, NULL);
  
  for(i =0; i < max_students; i++)
  {
      pthread_join(student_thds[i],NULL);
  }
  
  for(i =0; i < max_tutors; i++)
  {
      pthread_join(tutor_thds[i],NULL);
  }
  
  printQueue();
  
	free(stuPtr);
	return 0;
	
}