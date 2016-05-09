#define MAXFILENAME 16
#define MAXFILEEXTENTSION 3
#define TOTAL_NUM_BLOCKS 4096 
#define INODE_POINTERS 13
#define NUMBER_INODES 100
#define NUM_ENTRIES_PERBLOCK 21 //Each directory entry is 24 bytes, can fit 512/24=21 per block.
#define BLOCKS_INODES_TABLE 13
#define TOTAL_NUM_FILES 99 //100 files, 1 is for directory
#define BLOCKSIZE 512


int mksfs(int fresh); 
int sfs_getnextfilename(char* fname);
int sfs_getfilesize(const char* path);
int sfs_fopen(char *name); 
int sfs_fclose(int fileID);  
int sfs_fwrite(int fileID, const char *buf, int length);
int sfs_fread(int fileID, char *buf, int length); 
int sfs_fseek(int fileID, int loc); 
int sfs_remove(char *file); 


// super block
typedef struct{
	int magic; 
	int block_size;
	int file_system_size;
	int inode_table_length;
	int root_dir_inode_number;
}super_block;

//inode 
typedef struct{
	int mode; 
	char link_cnt; 
	char uid;
	char gid;
	int size;
	int pointers[INODE_POINTERS]; 
} INODE;

//directory entry
typedef struct{
	char filename[MAXFILENAME+MAXFILEEXTENTSION+1]; //16+'.'+3
	int inode_number;
} dir_entry;

//File descriptor tables, lives only in memory
typedef struct{
	char open;
	int rw_pointer;
}file_discrtiptor;



