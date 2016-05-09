#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_l.h"


#define SIZE_1 2000
#define SIZE_2 4000
#define SIZE_3 8000


int main(int argc, char* argv[])
{

  my_mallinfo();

  my_mallopt(2); //change options by uncomment
  //my_mallopt(1);

  int* buffer = my_malloc(SIZE_1);

  my_mallinfo();

  int* buffer2 = my_malloc(SIZE_2);

  my_mallinfo();

  my_free(buffer);

  my_mallinfo();

  int* buffer3 = my_malloc(SIZE_3);

  my_mallinfo();

  my_free(buffer3);

  my_mallinfo();

  int* buffer4 = my_malloc(SIZE_3);

  my_mallinfo();

  my_free(buffer2);

  my_mallinfo();

  my_free(buffer4);
 
  my_mallinfo();

  return 0;
}