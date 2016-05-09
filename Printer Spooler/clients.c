#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>


typedef struct buffer{
  int page;
  int clientID;

}buffer;

typedef struct job{
  int waitlist;
  int slotnumber;
  int emptybuffer;
  buffer b[200];
}job;



// used to identify shared memory
const char *name = "/ZhaoqiXu_shm";    
// file size
const int SIZE = 3000;              
// base address, from mmap()
void *attach;
// file descriptor, returned from shm_open()
int shmfd;          


int main(int argc, char *argv[])
{

    //create the job record
    int clientID = atoi(argv[1]);
    int page = atoi(argv[2]);

  /*-----------------handle invalid input---------------*/
    if(argc<=2){  //if no argument in command line,set the default id =1,page=1
      printf("Some arguments required\n");
      return -1;
    }
    else if(page==0){          //page cannot be 0
          printf("Page number invalid\n");
          return -1;
        }
    else if(argc>=4){
       printf("Too many arguments\n");
       return -1;
     }

   //reopen the semaphores created in printer.c
    sem_t *emptyCount = sem_open("EC", 0);
    sem_t *fillCount = sem_open("FC", 0);
    sem_t *mutex = sem_open("ME", 0);
  
   //Here since the shared memory has already been created,
    //we just open the exist shared memory created by printer
    shmfd = shm_open(name, O_CREAT |O_RDWR, 0666);
    if(shmfd == -1){
        printf("open Failed\n");
        return -1;
    }
   
    //Map shared memory object
    attach = mmap(NULL,SIZE, PROT_READ|PROT_WRITE , MAP_SHARED, shmfd, 0);
    if (attach == MAP_FAILED) {
         printf("Map failed\n");
        return -1;
    }
    //attach joblist to the shared memory
    job *joblist   = (struct job*)attach;
    
    /*-----------------------------------------------------------------*/
    int s = 0;
    //handle overflow
    if (joblist->waitlist <= 0){
      s =1;//FLAG TO CONTROL WHETHER THE "WAKE UP MESSAGE" IS GONNA BE PRINTED OUT 
      printf("Client %d has %d pages to print, buffer full, sleeps\n",clientID,page);
    }
    else{
      int i;
      for(i=0;i<joblist->slotnumber;i++)
      {
        if(joblist->b[i].page == 0){
          break;
        }
      }
      printf("Client %d has %d pages to print, puts request in Buffer[%d]\n",clientID,page,i);
    }

    sem_wait(emptyCount);
    sem_wait(mutex);
    
    int i;
    for(i=0;i<joblist->slotnumber;i++)
    {
      if(joblist->b[i].page==0)
      {
        break;
      }
    }

    if (s==1)
    {
      printf("Client %d wakes up, puts request in Buffer[%d]\n",clientID,i);
    }
    //put the job record into the shared buffer
    //update the information in the joblist queue
    joblist->b[i].clientID = clientID;
    joblist->b[i].page = page;
    joblist->emptybuffer--;
    joblist->waitlist--;
    
    sem_post(mutex);
    sem_post(fillCount);

    // unmap the link between pointer attach and shared memory
    munmap(attach, SIZE);
    /* release the shared memory */
    close(shmfd);
    //indicate that the calling process is finished using the named semaphore
    sem_close(mutex);
    sem_close(emptyCount);
    sem_close(fillCount);

    return 0;
  }
