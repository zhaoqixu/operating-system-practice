#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/mman.h>

//ACTUALLY in this assignment,I did not really create any job,
//just create an array which keep the job records

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
//slots number
int slotnum;





int main(int argc, char *argv[])
{
    int i;
    //check slots
    if(argc<=1){
        printf("Arguments required");
        return -1;
    }
    else
    {
        slotnum = atoi(argv[1]);
    }
    printf("number of slots is %d\n",slotnum);

    /*-----------setup_shared_mem()----------------*/
    //Create shared memory object 
    shmfd = shm_open(name, O_CREAT|O_RDWR, 0666);
    if(shmfd == -1){
        printf("Create Failed\n");
        return -1;
    }
    //Set shm size
    if(ftruncate(shmfd,SIZE) == -1){
        printf("Size set failed\n");
        return -1;
    }
    /*---------------------attach_shared_mem-----------------------*/
    //Map shared memory object
    attach = mmap(NULL,SIZE, PROT_READ|PROT_WRITE , MAP_SHARED, shmfd, 0);
    if (attach == MAP_FAILED) {
        printf("Map failed\n");
        return -1;
    }

    //attach joblist to the shared memory
    job *joblist = (struct job*)attach;
    //initialize the joblistlist
    joblist->waitlist  = slotnum;
    joblist->slotnumber = slotnum;
    joblist->emptybuffer = slotnum;
    
    
    for(i=0;i<slotnum;i++)
    {
        joblist->b[i].clientID=-1; //client id
        joblist->b[i].page=0;   
    }

    /*------------------------------initial_semaphore--------------------*/
    //incase that some other processes use the same name as an id 
    sem_unlink("EC");
    sem_unlink("FC");
    sem_unlink("ME");
    //initial and open three semaphores
    sem_t *emptyCount = sem_open("EC", O_CREAT, 0644, slotnum);
    sem_t *fillCount  = sem_open("FC", O_CREAT, 0644, 0);
    /* used to protect the semaphores from race condition, locks for mutual exclusion*/
    //binary semaphore
    sem_t *mutex = sem_open("ME",O_CREAT, 0644,1);
  
  while(1){

    if(joblist->emptybuffer == slotnum){
        printf("No request in buffer, Printer sleeps\n");
    }
    /*   take_a_job()   */
    sem_wait(fillCount);  
    
    
    int jid;    //job id
    for(jid=0;jid<slotnum;jid++){
        if(joblist->b[jid].page!=0){
            /* print_a_msg()*/
          printf("Printer starts printing %d pages from Buffer[%d] from client %d\n",joblist->b[jid].page,jid,joblist->b[jid].clientID);
          break;
        }
    }
    /*go_sleep()*/
    //page to be printed is the job duration
    sleep(joblist->b[jid].page);
    
    printf("Printer finishes printing %d pages from Buffer[%d] from client %d\n",joblist->b[jid].page,jid,joblist->b[jid].clientID);
    
    joblist->b[jid].clientID=-1; // release joblist slots
    joblist->b[jid].page=0;
    joblist->waitlist++;        //update information
    joblist->emptybuffer++;
   
    
    sem_post(emptyCount);
}

//remove the semaphore named by the string name
  sem_unlink("EC");
  sem_unlink("FC");
  sem_unlink("ME");
  return 0;
}

