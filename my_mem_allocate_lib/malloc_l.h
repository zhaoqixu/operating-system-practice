//policy
#define firstfit 1
#define bestfit 2
//define constants
#define EXCEED_SPACE 100000
#define TOP_FREE_SPACE 131072			//128 * 1024 (128KB)
#define TOP_FREE_REDUCE 20000
#define META_DATA 8
#define TAG_SIZE 4
#define ERROR_REPORT 1024

extern char *my_malloc_error;

void *my_malloc(int size);
void my_free(void *ptr);
void my_mallopt(int policy);
void my_mallinfo();
