#include "malloc_l.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//information for my_mallocinfo

int bytes_allocated = 0;
int free_space = 0;
int largest_free_space = 0;
int num_malloc_called = 0;

char* my_malloc_error;  
int* cur_program_break; // pointer points to the current program break
int cur_policy = 0;

//define free node list
typedef struct freelist
{
  struct freelist *next_node;
  struct freelist *prev_node;
} freelist;

//define head and tail of free node list
freelist *head;
freelist *tail;

//remove a node from freelist
void rm_node(freelist *current_pos) 
{
  if(current_pos == head){
    head = current_pos->next_node;
  }
  if(current_pos == tail){
    tail = current_pos->prev_node;
  }
  if(current_pos->next_node != NULL){
    current_pos->next_node->prev_node = current_pos->prev_node;
  }
  if(current_pos->prev_node != NULL){
    current_pos->prev_node->next_node = current_pos->next_node;
  }
  current_pos->prev_node = NULL;
  current_pos->next_node = NULL;
  current_pos = NULL;
}

//add a node to the doubly linked list
void add_node(freelist *new_node) 
{
  if(head == NULL)
  {
    head = new_node;
    tail = new_node;
  }
  else{
    head->prev_node = new_node;
    new_node->next_node = head;
    head = new_node;
    new_node->prev_node = NULL;
    }
  }

//modify tags with updated information
void modify_tags (int length, int free_flag, int* tag){
  *tag = (length << 1) + (free_flag == 0 ? 0b1 : 0b0); //if free_flag = 0, free_flag = 1;else free_flag =0
}

//keep track the largest block in freelist and update largest
void largest_free_space_tracker() 
{     
  int largest = 0;                  
  freelist *current_pos = head;
 
  while(current_pos != NULL)
  {
    int* cur = (int*)current_pos;
    cur = (int*)((char*)cur- TAG_SIZE);//points to the tag position
    int cur_size = cur[0] >> 1;        //retrive size information of the segmentation(bitwise operation)
    if(largest<cur_size )              //search for the largest free space
      largest = cur_size;
    current_pos = current_pos->next_node;
  }
  largest_free_space = largest;
}

//Allocates memory in many different situations
void* allocator(int size, int fit_size)
{
  int full_size = size+EXCEED_SPACE;   //my_malloc allocates larger memory to reduce calls to sbrk()

  /*---------------------------------------did not find such a block in free list-------------------*/
   if(fit_size == -2 || fit_size == TOP_FREE_SPACE){  //if there is a need to create a new memory segmentation
    
    int* free_last = (int*)sbrk(0);
    int* begin = (int*)sbrk(full_size + META_DATA);   //update the program break
    
    cur_program_break = free_last;    
    bytes_allocated += size;
     

    /*-----------------------------update metadata------------------------------*/
    modify_tags(size,0,begin);
    begin = (int*)((char*)begin + TAG_SIZE);
    int* last = (int*)((char*)begin + size);

    modify_tags(size,0,last);

    int* free_begin = (int*)((char*)last+ TAG_SIZE);
    modify_tags(full_size - size,1,free_begin);
    free_begin = (int*)((char*)free_begin+ TAG_SIZE);
    /*----------------------------add new free segmentation node to the free list---------------*/
    freelist *new_node = (freelist*)free_begin;  //add a new_node free block
    add_node(new_node);

    free_space += (full_size - size);
    free_last = (int*)((char*)free_begin+ (full_size-(size + META_DATA)));

    modify_tags(full_size - size,1,free_last);

    largest_free_space_tracker();
    return (void*)begin;  //return the address of the free segmentation to the caller
  }
  else
  { 
    /*------------------------------found a big enough block in freelist----------------------------*/
    freelist *current_pos = head;
    while(current_pos != NULL)
    {
      int* cur = (int*)(current_pos);
      cur = (int*)((char*)cur - TAG_SIZE);

      unsigned size_available = *cur >> 1;

      if(size_available >= fit_size) //relocate found node
        break;
      else
        current_pos = current_pos->next_node;
    }
    int* checker = (int*)(current_pos);   
    checker = (int*)((char*)checker - TAG_SIZE);
    unsigned size_available = checker[0] >> 1;

    /*----------------return block or split it up----------------------------------*/
    if(size_available - META_DATA >= size && size_available > (size + sizeof(freelist) + META_DATA))
    {
      int new_size = (size_available - (size + META_DATA));
      int* begin = (int*)checker;
      int* free_last = (int*)((char*)checker +(size_available));

      /*--------------------modify new tags ------------------------------------*/
      modify_tags(size,0,begin);
      begin = (int*)((char*)begin + TAG_SIZE);
      int* last = (int*)((char*)begin + size);
      modify_tags(size,0,last);


      int* free_begin = (int*)((char*)last + TAG_SIZE);

      modify_tags(new_size,1,free_begin);
      free_begin = (int*)((char*)free_begin + TAG_SIZE);

      /*----------------update free list ------------------*/
      rm_node(current_pos);                           

      freelist *new_node = (freelist*)free_begin;
      add_node(new_node);

      free_last = (int*)((char*)free_last - TAG_SIZE);

      modify_tags(new_size,1,free_last);

      bytes_allocated += size;
      free_space -= (size+META_DATA);
      largest_free_space_tracker();
      return (void*)begin;      
    } 
    else if(size_available - META_DATA >= size)
    { 
      /*----------------------------------- return block without split--------------------*/
      int* begin = (int*)checker;

      modify_tags(size_available-META_DATA,0,begin);

      begin = (int*)((char*)begin + TAG_SIZE);
      int* last = (int*)((char*)begin +(size_available-META_DATA));
   
      modify_tags(size_available-META_DATA,0,last);
      /*----------------------------------- modify freelist------------------------*/
      rm_node(current_pos);        

      bytes_allocated += (size_available-META_DATA);
      free_space -= size_available;

      largest_free_space_tracker();
      return (void*)begin;
    }
  }
  return NULL;
}

void *my_malloc(int size)
{
  //if allocating touches this line ,reporting error
  if(num_malloc_called == 0){
    my_malloc_error = (char*)sbrk(ERROR_REPORT); 
  }

  num_malloc_called++;
  //if size < 0,reporting error
  if(size < 0)
  {
    my_malloc_error = "Mallocing requested invalid space";
    return NULL;
  }
  if(cur_policy!=firstfit && cur_policy!=bestfit){
    return NULL;
  }
  //policy = 1, first fit
  if(cur_policy == firstfit)  
  {

    freelist *current_pos = head;  
    while(current_pos != NULL)
    {
      int* checker = (int*)(current_pos);
      checker = (int*)(((char*)checker) - TAG_SIZE);
      unsigned size_available = checker[0] >>1;
      if(size_available - META_DATA >= size)
        //found fit segmentation
        return allocator(size, size_available);   
      if(current_pos != NULL)
        current_pos = current_pos->next_node;
    }
     return allocator(size, -2);
  }
  //if policy = 2, best fit
  else if(cur_policy == bestfit)   
  {
    freelist *current_pos = head;
    unsigned best = TOP_FREE_SPACE;
    while(current_pos != NULL)   
    {
      int* checker = (int*)(current_pos);
      checker = (int*)(((char*)checker) - TAG_SIZE);
      unsigned size_available = checker[0]>>1;
      if(size_available - META_DATA >= size && size_available - META_DATA <= best)
        best = size_available;
      if(current_pos != NULL)
        current_pos = current_pos->next_node;
    }
    return allocator(size, best); //found best space 
  }
  else return allocator(size, -2); //if no block was big enough,extlast the heap

  my_malloc_error = "my_malloc Error";  //if there is an error, return NULL
  return NULL;
}



void my_free(void *ptr)
{
  /*-----------------------------------retrive block size -------------------------------*/
  int* new_free = (int*)(ptr);  
  new_free = (int*)(((char*)new_free) - TAG_SIZE);

  int free_size = new_free[0]>>1;

  bytes_allocated -= free_size;
  free_space += (free_size + META_DATA);

  /*------------------checker bottom and top block free or not for merging----------*/
  int* bot_tracker = (int*)(((char*)new_free) - TAG_SIZE);
  int* top_tracker = (int*)((char*)new_free + (free_size+META_DATA));

  int bot_free = bot_tracker[0]&0b1;  //bitwise operations to checker free bits
  int top_free = top_tracker[0]&0b1;
  int bot_size_checker = bot_tracker[0]>>1;
  //if bottom block is the block where we start,do not merge it
  if(bot_size_checker == 0)
    bot_free = -1;
  //if both bottom and top are free,merge them all
  if(!bot_free && !top_free) 
  {
    int bot_size = bot_tracker[0]>>1;
    bot_tracker = (int*)((char*)bot_tracker - (bot_size - META_DATA));

    int top_size = top_tracker[0]>>1;
    top_tracker = (int*)((char*)top_tracker + TAG_SIZE);

    /*---------------------remove old nodes-----------------*/
    freelist *bot_node = (freelist*)(bot_tracker); 
    freelist *top_node = (freelist*)(top_tracker);
    rm_node(bot_node);
    rm_node(top_node);

    /*----------------------------modify new tags--------------------*/
    free_size += (bot_size + top_size + META_DATA);           
    bot_tracker = (int*)((char*)bot_tracker- TAG_SIZE);
    top_tracker = (int*)((char*)top_tracker+(top_size - META_DATA));
    modify_tags(free_size,1,bot_tracker);
    modify_tags(free_size,1,top_tracker);

    /*-----------------------------add new nodes-----------------*/
    int* begin = (int*)((char*)bot_tracker +TAG_SIZE);
    freelist *new_node = (freelist*)begin;
    add_node(new_node);                                       
  }
  /*---------------------only bottom block is free----------------------------*/
  else if(!bot_free)             
  {
    int bot_size = bot_tracker[0]>>1;
    bot_tracker = (int*)((char*)bot_tracker-(bot_size - META_DATA));

    /*--------------------remove bot node in freelist--------------------------*/
    freelist *bot_node = (freelist*)(bot_tracker);   
    rm_node(bot_node);

    bot_tracker = (int*)((char*)bot_tracker- TAG_SIZE);
    new_free = (int*)((char*)new_free +(free_size + TAG_SIZE));

    /*-----------------------modify tags-----------------------------*/
    free_size = free_size+(bot_size + META_DATA);                         
    modify_tags(free_size,1,bot_tracker);
    modify_tags(free_size,1,new_free);

    int* begin = (int*)((char*)bot_tracker+ TAG_SIZE);
    /*-----------------------add new node in free list--------------------------*/
    freelist *new_node = (freelist*)begin;
    add_node(new_node);                                     
  }
  //only top block is free
  else if(!top_free)   
  {
    int top_size = top_tracker[0]>>1;
    top_tracker = (int*) ((char*)top_tracker + TAG_SIZE);

     /*--------------------remove top node in freelist--------------------------*/
    freelist *top_node = (freelist*)(top_tracker);   
    rm_node(top_node);

     /*-----------------------modify tags-----------------------------*/
    top_tracker = (int*)((char*)top_tracker+ (top_size - META_DATA));  
    free_size = free_size+(top_size + META_DATA);
  
    modify_tags(free_size,1,new_free);
    modify_tags(free_size,1,top_tracker);

    /*-----------------------add new node in free list--------------------------*/
    int* begin = (int*)((char*)new_free+ TAG_SIZE);
    freelist *new_node = (freelist*)begin;       
    add_node(new_node);
  }
  else
  //neither is free, add a new free node to the list and modify the tag
  { 
    modify_tags(free_size+META_DATA,1,new_free);
    new_free = (int*)((char*)new_free + (free_size + TAG_SIZE));
    modify_tags(free_size+META_DATA,1,new_free);
    int* begin = (int*)((char*)new_free-free_size);
    freelist *new_node = (freelist*)begin;
    add_node(new_node);
  }
    
  int* checker_top = (int*)(cur_program_break);
  checker_top = (int*)((char*)checker_top- TAG_SIZE);
  int top_tag = checker_top[0]>>1;
  if(top_tag == 0)
  {
    unsigned top_free_space = checker_top[0]>>1;

    //checker if top free space is larger than 128KB
    if(top_free_space >=  TOP_FREE_SPACE) 
    {                         
      //if yes,reduce it by TOP_FREE_REDUCE;
      checker_top = (int*)((char*)checker_top - (top_free_space - TAG_SIZE));
      sbrk(-TOP_FREE_REDUCE);
      top_free_space = top_free_space-TOP_FREE_REDUCE;
      //modify top free space tag 
      modify_tags(top_free_space,1,checker_top);
      checker_top = (int*)((char*)checker_top + (top_free_space - TAG_SIZE));
      modify_tags(top_free_space,1,checker_top);
      cur_program_break = (int*)((char*)cur_program_break - TOP_FREE_REDUCE);

      free_space = free_space-TOP_FREE_REDUCE;
    }
  }
  largest_free_space_tracker(); 
}

//used to change my_malloc policy, 1 means first-fit, 2 means best-fit
void my_mallopt(int policy) 
{
  if(policy == firstfit)
    cur_policy = firstfit;
  else if(policy == bestfit)
    cur_policy = bestfit;
  else
  {
    // char str[50];
    // sprintf(str, "Wrong option code");
    // puts(str);
    return ;
  }
}

//printout the information tracked through the whole program
void my_mallinfo()  
{
  char str1[100],str2[100],str3[100],str4[100],str5[100];
  sprintf(str1, "Current number of bytes allocated: %d", bytes_allocated);
  puts(str1);
  sprintf(str2, "Total free space: %d", free_space);
  puts(str2);
  sprintf(str3, "Largest contiguous free space: %d", largest_free_space);
  puts(str3);
  sprintf(str4, "Number of my_malloc() gets called: %d", num_malloc_called); 
  puts(str4);
  sprintf(str5, "----------------------------------------------------------------");
  puts(str5);
}

