//Zhaoqi Xu
//260563752

#include "disk_emu.h"
#include "sfs_api.h"
#include <stdio.h>
#include <string.h>
#include <libgen.h> //for using basename()
#include <stdio.h>
#include <math.h>

#define DISK_FILE "sfs_disk.disk"

unsigned char free_bitmap[BLOCKSIZE]; //one block,each bits represent whether a block is free or not
dir_entry dir_table[TOTAL_NUM_FILES];
file_discrtiptor file_des_t[TOTAL_NUM_FILES];
INODE inodes[NUMBER_INODES];
int cur_pos = 0;


//return total numbers of pointers in an inode
int get_num_pointers_inode(int inode_index){
	int num_of_pointers=0;
	int k;
	for(k=0;k<INODE_POINTERS;k++){
		if(inodes[inode_index].pointers[k]!=0){
			num_of_pointers++;
		}
	}
	if(num_of_pointers==INODE_POINTERS){
		int indirect_buffer[BLOCKSIZE/4]; //indirect block is used
		memset(indirect_buffer,0,BLOCKSIZE);
		read_blocks(inodes[inode_index].pointers[INODE_POINTERS-1], 1, indirect_buffer);
		int i=0;
		while(indirect_buffer[i]!=0){ //check the number of pointers in indirect node
			i++;
		}
		if(i==0){
			return -1; // empty indirect pointer
		}
		else{
			num_of_pointers = num_of_pointers+i; 
		}	
	}
	return num_of_pointers;
}


//find a free block in bitmap
int findfreeblock(void){
	int i;
	for(i=0;i<BLOCKSIZE;i++){
		if((free_bitmap[i]&128)==0){return i*8+1;}//each entry has 8 bits which can reprensent 8 blocks
		else if((free_bitmap[i]&64)==0){return i*8+2;}
		else if((free_bitmap[i]&32)==0){return i*8+3;}
		else if((free_bitmap[i]&16)==0){return i*8+4;}
		else if((free_bitmap[i]&8)==0){return i*8+5;}
		else if((free_bitmap[i]&4)==0){return i*8+6;}
		else if((free_bitmap[i]&2)==0){return i*8+7;}
		else if((free_bitmap[i]&1)==0){return i*8+8;}
	}
	return -1;//disk is full
}


//mark a free block to be occupied
void markfreeblock(int block_to_be_marked){

	int i=(block_to_be_marked-1)/8; //because each byte can represent 8 blocks
	char bit_to_be_updated = block_to_be_marked%8; 
	if(bit_to_be_updated==1){free_bitmap[i]=free_bitmap[i]|128;} //& | bitewise operators
	else if(bit_to_be_updated==2){free_bitmap[i]=free_bitmap[i]|64;}
	else if(bit_to_be_updated==3){free_bitmap[i]=free_bitmap[i]|32;}
	else if(bit_to_be_updated==4){free_bitmap[i]=(free_bitmap[i])|16;}
	else if(bit_to_be_updated==5){free_bitmap[i]=free_bitmap[i]|8;}
	else if(bit_to_be_updated==6){free_bitmap[i]=free_bitmap[i]|4;}
	else if(bit_to_be_updated==7){free_bitmap[i]=free_bitmap[i]|2;}
	else if(bit_to_be_updated==0){free_bitmap[i]=free_bitmap[i]|1;}	
}

//Creates the file system
int mksfs(int fresh){
	char buf[BLOCKSIZE];
	memset(buf,0, BLOCKSIZE); //sets a block-size buffer  and set it to zero
	
	if(fresh){
		init_fresh_disk(DISK_FILE, BLOCKSIZE, TOTAL_NUM_BLOCKS); 
		
		//initialize super block
		super_block superblock;
		superblock.magic=0xAABB0005;
		superblock.block_size=BLOCKSIZE;
		superblock.file_system_size=TOTAL_NUM_BLOCKS;
		superblock.inode_table_length=NUMBER_INODES;
		superblock.root_dir_inode_number=0;	//set root directory at the first inode 
		//copy super block to buffer
		memcpy((void *)buf,(const void *) &superblock,  sizeof(super_block)); 
		//write the superblock to the first block in the disk
		write_blocks(0,1, buf);
		//initialize inode table
		memset(inodes, 0, sizeof(inodes));
		//Set first inode to root directory inode 
		INODE root_dir_inode={
			.mode=0777, 
			.link_cnt=1, 
			.uid=0, 
			.gid=0,
			.size=0,
			.pointers={14,15,16,17,18,0,0,0,0,0,0,0,0} //pointer to the first data block starting at 14
		};

		inodes[0]=root_dir_inode;
		//write inode into disk
		int inode_number = 0;
		int block_of_inode=inode_number/8+1; 
		char inode_update_buf[BLOCKSIZE];
		memset(inode_update_buf,0, BLOCKSIZE);
		read_blocks(block_of_inode, 1, inode_update_buf);
		memcpy((void *)inode_update_buf+(inode_number%8)*sizeof(INODE),(const void *) &inodes[inode_number], sizeof(INODE));
		write_blocks(block_of_inode, 1, inode_update_buf);	

		//initialize the bitmap
		memset(free_bitmap, 0, sizeof(free_bitmap));
		free_bitmap[0]=255; //superblock and first part of inodes to occupied.
		free_bitmap[1]=255; //inodes table to occupied.
		free_bitmap[2]=224; //directory entry table to occupied. 
		free_bitmap[BLOCKSIZE-1]=1; //last block for the bitmap	
		write_blocks(TOTAL_NUM_BLOCKS-1,1,free_bitmap);//write the bitmap into disk

		//initialize directory table into memory
		memset(dir_table, 0, sizeof(dir_table));

		//Initialize all file's RW pointers to 0 and open to 0 
		memset(file_des_t, 0, sizeof(file_des_t));
		cur_pos=-1;
		return 0;
	}
	else{ 
		//file system already exists
		init_disk(DISK_FILE, BLOCKSIZE, TOTAL_NUM_BLOCKS);
		// set free_bitmap to zeros
		memset(free_bitmap,0, sizeof(free_bitmap));
		//load existing bitmap 
		read_blocks(TOTAL_NUM_BLOCKS-1,1,free_bitmap);
		//load existing superblock
		super_block superblock;
		read_blocks(0,1,&superblock);
		//load existing inode table
		char inodes_buffer[BLOCKS_INODES_TABLE*BLOCKSIZE];
		memset(inodes_buffer,0,sizeof(inodes_buffer));
		read_blocks(1,BLOCKS_INODES_TABLE,inodes_buffer);
		
		int n;
		for(n=0;n<NUMBER_INODES;n++){
			memcpy((void *)&(inodes[n]),(const void *)(inodes_buffer+n*(BLOCKSIZE/8)), BLOCKSIZE/8); 
		}
		
		//load existing directory table
		INODE root_directory=inodes[superblock.root_dir_inode_number];
		int i;
		
		char root_dir_buffer[BLOCKSIZE];
		
		memset(root_dir_buffer,0,BLOCKSIZE);
		
		for(i=0;i<INODE_POINTERS;i++){
			if(root_directory.pointers[i]!=0){
				read_blocks(root_directory.pointers[i],1, root_dir_buffer); 
				int j;
				for(j=0;j<NUM_ENTRIES_PERBLOCK;j++){
					if((j+NUM_ENTRIES_PERBLOCK*i) >= TOTAL_NUM_FILES){
						break;
					}
					memcpy((void *)&(dir_table[j]), (const void *)(root_dir_buffer+j*(sizeof(dir_entry))), sizeof(dir_entry));
				}
				memset(root_dir_buffer,0,BLOCKSIZE);
			}
		}
		//initialize all rw pointers to 0 and open to 0 
		memset(file_des_t, 0, sizeof(file_des_t));
		cur_pos=-1;
		return 0;
	}
}


int sfs_getnextfilename(char* fname){
//copies name of the next file in the directory into filename 
	while(1){
		if (cur_pos == TOTAL_NUM_FILES+1) {//end of directory return 0
			cur_pos = -1;		
			return 0;
		}
		cur_pos++;
		if (dir_table[cur_pos].inode_number != 0) {	//there is a file in the next position
			strncpy(fname, dir_table[cur_pos+1].filename,20); //length of filename+fileextension+1
			return 1;
		}
	}
}


//copy all pointers into an array of block pointers return nothing
int get_pointers(int inode_index, int *array_blockptr, int num_pointers){
	if(num_pointers==0){
		return -1;	//empty return -1
	}
	//int indirect_pointers=0;
	int direct_ptrs=0;
	if(num_pointers < INODE_POINTERS){
		//direct pointers
		direct_ptrs = num_pointers;
		int k;
		for(k=0;k<direct_ptrs;k++){
			array_blockptr[k]=inodes[inode_index].pointers[k];
		}
	}
	else{
		direct_ptrs=INODE_POINTERS-1;
		int k;
		for(k=0;k<direct_ptrs;k++){
			array_blockptr[k]=inodes[inode_index].pointers[k];
		}
		int indirect_buf[BLOCKSIZE/sizeof(int)];
		memset(indirect_buf,0,BLOCKSIZE);
		read_blocks(inodes[inode_index].pointers[INODE_POINTERS-1], 1, indirect_buf);
		int j=0;
		while(indirect_buf[j]!=0){
			array_blockptr[j+direct_ptrs]=indirect_buf[j];
			j++;
		}
		return 0;
	}
	return 0;
}


int return_filesize(int inode_index){
	
	int num_pointers = get_num_pointers_inode(inode_index);
	if(num_pointers>NUMBER_INODES-1){
		num_pointers--; //check if indirect pointer used,if yes,removed
	}
	int array_blockptr[num_pointers]; 
	get_pointers(inode_index,array_blockptr,num_pointers);//copy all pointers except indirect pointer into an array of block pointers
	int num_full_block=num_pointers-1; //except last block because it might not full
	//check data in last unfull block
	int last_block_data=0;
	char buffer[BLOCKSIZE];
	memset(buffer,0,BLOCKSIZE);
	read_blocks(array_blockptr[num_pointers-1],1, buffer); //read it into memory
	int a = BLOCKSIZE-1;
	while(buffer[a]==0){
		a--;
	}
	last_block_data = a+1;
	return num_full_block*BLOCKSIZE + last_block_data;
}

int sfs_getfilesize(const char* path){
	// return a pointer to the final component of the pathname,because we only have root directory 
	//so just search it directly in the dir_table
	int k;
	int inode_num;

	char *filename = basename((char *)path); 
	
	for(k=0;k<TOTAL_NUM_FILES;k++){
		if(strcmp(dir_table[k].filename,filename)==0){
			inode_num=dir_table[k].inode_number;
			break;
		}
		if(k==TOTAL_NUM_FILES-1){
			return -1;
		}
	}
	return return_filesize(inode_num); 
}


int sfs_fopen(char *name){
	
	int i;
	for(i=0;i<TOTAL_NUM_FILES;i++){
		if(strncmp(dir_table[i].filename, name, MAXFILENAME+1+MAXFILEEXTENTSION)==0){ //check the whole name including the extention
			if(file_des_t[i].open == 1){//check if it is opened
				return i; //return the index in file descriptor table		
			}
			//else open this file 
			file_des_t[i].open = 1;
			//put the pointer after the new opened file
			file_des_t[i].rw_pointer = inodes[i+1].size;
			return i;
		}
	}
	//file does not exist
	int unoccupied_inode_index;
	int j;
	for(j=1;j<NUMBER_INODES;j++){//j =1 because j = 0 is the root inode
		if(inodes[NUMBER_INODES-1].link_cnt==1){
			return -1;
		}
		if(inodes[j].link_cnt==0){ //link_cnt = 0,means unoccupied
			unoccupied_inode_index=j;
			break;
		}
	}	

	//initialize a new node
	INODE new_inode={
		.mode=0777, 
		.link_cnt=1, // has one link pointing to it 
		.uid=0, 
		.gid=0, 
		.size=0,
		.pointers={0,0,0,0,0,0,0,0,0,0,0,0,0} 
	};
	
	inodes[unoccupied_inode_index] = new_inode;
	
	int inode_num = unoccupied_inode_index;
	int num_blocks = inode_num/8+1; 
	char update_inode_buffer[BLOCKSIZE];
	memset(update_inode_buffer,0, BLOCKSIZE);
	read_blocks(num_blocks, 1, update_inode_buffer);
	memcpy((void *)update_inode_buffer+(inode_num%8)*sizeof(INODE),(const void *) &inodes[inode_num], sizeof(INODE));
	write_blocks(num_blocks, 1, update_inode_buffer);	

	//find a free position in directory table
	int index_of_directory;
	for (index_of_directory=0; index_of_directory < TOTAL_NUM_FILES; index_of_directory++){
		if(dir_table[index_of_directory].inode_number==0){
			break;	
		}
	}
	
	//initialize new directory entry in 
	dir_entry new_dir_entry;
	strcpy(new_dir_entry.filename, name);
	new_dir_entry.inode_number=unoccupied_inode_index;
	dir_table[index_of_directory]=new_dir_entry;

	//write it into disk
	int index_dir_entry_block = index_of_directory/NUM_ENTRIES_PERBLOCK+BLOCKS_INODES_TABLE+1; //+1 for superblock
	char buf[BLOCKSIZE];
	memset(buf,0, BLOCKSIZE);
	read_blocks(index_dir_entry_block, 1, buf);
	memcpy((void *)buf+(index_of_directory%NUM_ENTRIES_PERBLOCK)*sizeof(dir_entry),(const void *) &dir_table[index_of_directory], sizeof(dir_entry));
	write_blocks(index_dir_entry_block,1,buf);
	file_des_t[index_of_directory].open = 1;
	return index_of_directory;
} 

//close opened file by setting open arrtibute to 0 
int sfs_fclose(int fileID){
	if(file_des_t[fileID].open==1){
		file_des_t[fileID].open=0;
		return 0;
	}
	else
	{
		return -1;
	}
} 

//helper function to help update the block numbers by received an offset bytes and update the buffered  memory of the block
int update_block(int block_num, char *data, int offset_bytes, int num_bytes_write){
	char buffer[BLOCKSIZE];
	memset(buffer, 0, BLOCKSIZE);
	//read existing blocks into buffer
	read_blocks(block_num,1, buffer);
	memcpy(buffer+offset_bytes, data, num_bytes_write);
	int flag = write_blocks(block_num, 1, buffer);
	if(flag){
		return num_bytes_write;
	}
	else{
		return -1;
	}
}


// Writing the indirect block to disk
void update_inode_pointers(int inode_index, int *array_blockptr, int num_pointers){
	if(num_pointers <= INODE_POINTERS-1){
		//direct pointers only 1-12
		int i;
		for(i=0;i<num_pointers-1;i++){
			inodes[inode_index].pointers[i]=array_blockptr[i];
		}
	}
	else{//hanles the indirect pointer
		int i;
		for(i=0;i<INODE_POINTERS-1;i++){
			inodes[inode_index].pointers[i]=array_blockptr[i];
		}
		int buffer[BLOCKSIZE/sizeof(int)];
		memset(buffer,0,BLOCKSIZE);
		if(inodes[inode_index].pointers[INODE_POINTERS-1]!=0){
		//append indirect pointer
			read_blocks(inodes[inode_index].pointers[INODE_POINTERS-1],1,buffer);
			int initial_number_of_indirects=get_num_pointers_inode(inode_index)-INODE_POINTERS;
			int final_number_of_indirects=num_pointers-INODE_POINTERS+1;
			int k;
			for(k=0;k<(final_number_of_indirects-initial_number_of_indirects)-1;k++){
				buffer[k+initial_number_of_indirects]=array_blockptr[(INODE_POINTERS-1)+initial_number_of_indirects+k];
			}
			write_blocks(inodes[inode_index].pointers[INODE_POINTERS-1],1,buffer);
		}
		else
		{
			//find a free block for indirect pointer
			int free_block=findfreeblock();
			inodes[inode_index].pointers[INODE_POINTERS-1]=free_block;
			markfreeblock(free_block); //mark it as occupied
 			//write inodes pointer into that block
			int j;
			for(j=0;j<(num_pointers-INODE_POINTERS+1)-1;j++){
				buffer[j]=array_blockptr[(INODE_POINTERS-1)+j];
			}	
			write_blocks(free_block,1,buffer);
		}
	}
}


int sfs_fwrite(int fileID, const char *buf, int length){

	int inode_index = dir_table[fileID].inode_number;
	int num_bytes_to_be_written=0;
	
	//index of rw pointers which points to the current block of inode
	int rw_index = file_des_t[fileID].rw_pointer/BLOCKSIZE+1; 
	//rw pointer offset within its current block
	int rw_offset = file_des_t[fileID].rw_pointer%BLOCKSIZE; 
	//calculate number of blocks to be written
	int num_blocks;
	if(length<BLOCKSIZE-rw_offset){
		num_blocks=1;
	}
	else{
		num_blocks=(length-(BLOCKSIZE-rw_offset))/BLOCKSIZE+2; //+2 for the first & last partial blocks
	}

	//writing to an empty file.
	if(inodes[inode_index].pointers[0]==0){
	//If file is empty all inode pointers will be zero 
	int num_bytes_to_be_written = 0;
	int inode_index=dir_table[fileID].inode_number;
	int num_blocks=length/(BLOCKSIZE)+1;
	
		if(num_blocks<=(INODE_POINTERS-1)){ 
			//don't need the indirect pointer
			int j;
			char write_block_buffer[BLOCKSIZE];
			for(j=0;j<num_blocks-1;j++){
				//reset buffer
				memset(write_block_buffer,0,BLOCKSIZE); 
				memcpy(write_block_buffer,buf,BLOCKSIZE); 
				inodes[inode_index].pointers[j]=findfreeblock();
				
				int flag = write_blocks(inodes[inode_index].pointers[j],1,write_block_buffer);
				if(flag){ 
				//if write into disk successfully,count the number of bytes written
					num_bytes_to_be_written = num_bytes_to_be_written + BLOCKSIZE;
				}
				//upodate the bitmap
				markfreeblock(inodes[inode_index].pointers[j]);
			}
			
			//last block could be a partial block
			char direct_pointer_buf_last[BLOCKSIZE];
			memset(direct_pointer_buf_last,0,BLOCKSIZE);
			memcpy(direct_pointer_buf_last, buf+(num_blocks-1)*BLOCKSIZE, length % BLOCKSIZE);
			//find a free block for the last direct pointer
			int last_direct_pointer = findfreeblock();
			markfreeblock(last_direct_pointer);
			inodes[inode_index].pointers[num_blocks-1] = last_direct_pointer;
			//if write into disk successfully then count the number of bytes
			int flag = write_blocks(last_direct_pointer,1,direct_pointer_buf_last);
			if(flag){
				num_bytes_to_be_written = num_bytes_to_be_written + length%BLOCKSIZE;
			}
			//update the information in file descriptor table
			file_des_t[fileID].rw_pointer = file_des_t[fileID].rw_pointer + length;
			inodes[inode_index].size = inodes[inode_index].size + num_bytes_to_be_written;
			
			//update info on the disk
			int inode_number = inode_index;
			int block_of_inode=inode_number/8+1; 
			char inode_update_buf[BLOCKSIZE];
			memset(inode_update_buf,0, BLOCKSIZE);
			read_blocks(block_of_inode, 1, inode_update_buf);
			memcpy((void *)inode_update_buf+(inode_number%8)*sizeof(INODE),(const void *) &inodes[inode_number], sizeof(INODE));
			write_blocks(block_of_inode, 1, inode_update_buf);	
			write_blocks(TOTAL_NUM_BLOCKS-1,1,free_bitmap);
			return num_bytes_to_be_written;
		}
		else{	
				//else need the indirect pointer
				int m;
				char write_block_buffer[BLOCKSIZE];
				//handle all the direct pointer firstly,similar as the above 
				for(m=0;m<INODE_POINTERS-1;m++){
				memset(write_block_buffer,0,BLOCKSIZE); 
				memcpy(write_block_buffer,buf,BLOCKSIZE); 
				inodes[inode_index].pointers[m]=findfreeblock();
				int flag=write_blocks(inodes[inode_index].pointers[m],1,write_block_buffer);
				if(flag){num_bytes_to_be_written+=BLOCKSIZE;}
				markfreeblock(inodes[inode_index].pointers[m]);
			}
			//now let's handle the indirect pointer
			int num_indirect_pointers = num_blocks-(INODE_POINTERS-1);
			//get free blocks for indirect pointer
			int freeblock_indirect = findfreeblock(); 
			markfreeblock(freeblock_indirect);
			int arr_of_indirectptr[num_indirect_pointers];
			int k;
			//write the remaining data to the indirect blocks
			char indirect_pointer_buf[BLOCKSIZE];
			for(k=0;k<num_indirect_pointers-1;k++){
				arr_of_indirectptr[k] = findfreeblock();
				memset(indirect_pointer_buf,0,BLOCKSIZE);
				memcpy(indirect_pointer_buf, buf+(INODE_POINTERS-1)*BLOCKSIZE + k*BLOCKSIZE, BLOCKSIZE);
				int flag=write_blocks(arr_of_indirectptr[k],1,indirect_pointer_buf);
				if(flag){num_bytes_to_be_written+=BLOCKSIZE;}
				markfreeblock(arr_of_indirectptr[k]);
			}
			//handle partial indirect block
			int last_indirect_pointers = findfreeblock();
			arr_of_indirectptr[num_indirect_pointers-1] = last_indirect_pointers;
			markfreeblock(last_indirect_pointers);
			char last_indirect_pointer_buf[BLOCKSIZE];
			memset(last_indirect_pointer_buf,0,BLOCKSIZE);
			memcpy(last_indirect_pointer_buf,buf+(INODE_POINTERS-1)*BLOCKSIZE+(num_indirect_pointers-1)*BLOCKSIZE,length%BLOCKSIZE);
			int flag = write_blocks(arr_of_indirectptr[num_indirect_pointers-1],1,last_indirect_pointer_buf);
			if(flag){
				num_bytes_to_be_written = num_bytes_to_be_written + length%BLOCKSIZE;
			}
			
			//Write the indirect pointers to the indirect block
			char indirect_buffer[BLOCKSIZE];
			memset(indirect_buffer,0,BLOCKSIZE);
			memcpy(indirect_buffer,arr_of_indirectptr,num_indirect_pointers*sizeof(int));
			write_blocks(freeblock_indirect,1,indirect_buffer); 
			inodes[inode_index].pointers[INODE_POINTERS-1]=freeblock_indirect;
			file_des_t[fileID].rw_pointer = file_des_t[fileID].rw_pointer + length;
			inodes[inode_index].size = inodes[inode_index].size + num_bytes_to_be_written;
			
			//update all the info into the disk
			int inode_number = inode_index;
			int block_of_inode = inode_number/8+1; 
			char inode_update_buf[BLOCKSIZE];
			memset(inode_update_buf,0, BLOCKSIZE);
			read_blocks(block_of_inode, 1, inode_update_buf);
			memcpy((void *)inode_update_buf+(inode_number%8)*sizeof(INODE),(const void *) &inodes[inode_number], sizeof(INODE));
			write_blocks(block_of_inode, 1, inode_update_buf);	
			write_blocks(TOTAL_NUM_BLOCKS-1,1,free_bitmap);
			return num_bytes_to_be_written;
			}	
		}

	//file not empty
	int number_pointers;
	if(get_num_pointers_inode(inode_index)>INODE_POINTERS-1){
		//first ignore the indrect pointers if it is used
		number_pointers = get_num_pointers_inode(inode_index)-1; 
	}
	else{
		number_pointers = get_num_pointers_inode(inode_index);
	}
	
	int array_blockptr[number_pointers];
	get_pointers(inode_index,array_blockptr,number_pointers);

	int num_unchanged_blocks = rw_index;
	//Update all those blocks	
	int updated_array_blockptr[num_blocks + num_unchanged_blocks];
	memset(updated_array_blockptr,0,num_blocks*sizeof(int));
	//Copy the unchanged blocks
	int p;
	for(p = 0; p < rw_index-1; p++){		
		updated_array_blockptr[p] = array_blockptr[p];
	}

	int s;
	for(s=rw_index-1; s<((num_blocks+num_unchanged_blocks)-1); s++){
		//Allocate additional free blocks when s>total intial blocks
		if(s>number_pointers-1){
			//free block is not enough, find more
				if(s==(num_blocks+num_unchanged_blocks-2)){
				//last block need to do a partial write
				int free_block = findfreeblock();
				updated_array_blockptr[s] = free_block;
				markfreeblock(free_block);
				if(num_blocks==1){
					update_block(updated_array_blockptr[s],(char*)(buf+(BLOCKSIZE-rw_offset)+BLOCKSIZE*(s-rw_index-1)),0,length);
					num_bytes_to_be_written=num_bytes_to_be_written+length;
				}
				else{
					if((BLOCKSIZE-rw_offset)+BLOCKSIZE*(s-rw_index)<0){
						return -1;//write in last block
					}
					update_block(updated_array_blockptr[s],(char*)(buf+(BLOCKSIZE-rw_offset)+BLOCKSIZE*(s-rw_index)),0,(length-(BLOCKSIZE-rw_offset))%BLOCKSIZE);	
					num_bytes_to_be_written = num_bytes_to_be_written + (length-(BLOCKSIZE-rw_offset)) % BLOCKSIZE;
				}			
			}
			else{				
				int free_block = findfreeblock();
				updated_array_blockptr[s] = free_block;
				markfreeblock(free_block);
				update_block(updated_array_blockptr[s],(char*)(buf+(BLOCKSIZE-rw_offset)+BLOCKSIZE*(s-rw_index)),0,BLOCKSIZE);	
				num_bytes_to_be_written = num_bytes_to_be_written + BLOCKSIZE;
			}	
		}
		else{
			markfreeblock(array_blockptr[s]);
			updated_array_blockptr[s] = array_blockptr[s];	
			if(s==rw_index-1){
				//Overwriting the first block of write
				if(num_blocks==1){	
					//if only updating one block - will not be filling the first block of the write.
					update_block(array_blockptr[s],(char*)buf,rw_offset,length);
					num_bytes_to_be_written = num_bytes_to_be_written + length;
				}
				else{
					update_block(array_blockptr[s],(char*)buf,rw_offset,BLOCKSIZE-rw_offset);
					num_bytes_to_be_written = num_bytes_to_be_written + BLOCKSIZE-rw_offset;
				}
			}
			else if(s==(num_blocks + num_unchanged_blocks-2)){
				//last block may be partial
				if(num_blocks==1){
					//special case for one block write.
					update_block(updated_array_blockptr[s],(char*)(buf+(BLOCKSIZE-rw_offset)+BLOCKSIZE*(s-rw_index-1)),0,length);
					num_bytes_to_be_written = num_bytes_to_be_written+length;
				}
				else{
					update_block(updated_array_blockptr[s],(char*)(buf+(BLOCKSIZE-rw_offset)+BLOCKSIZE*(s-rw_index-1)),0,(length-(BLOCKSIZE-rw_offset))%BLOCKSIZE);
					num_bytes_to_be_written = num_bytes_to_be_written+(length-(BLOCKSIZE-rw_offset))%BLOCKSIZE;
				}
			}
			else{
				//overwriting a full block
				update_block(array_blockptr[s],(char *)(buf+(BLOCKSIZE-rw_offset)+BLOCKSIZE*(s-rw_index-1)),0,BLOCKSIZE);
				num_bytes_to_be_written = num_bytes_to_be_written+BLOCKSIZE;
			}
		}
	}


	//storing the block pointers and put updated_array_blockptr into inode
	update_inode_pointers(inode_index,updated_array_blockptr,num_blocks+num_unchanged_blocks);
	int f_num_pointers = get_num_pointers_inode(inode_index);
	if(f_num_pointers > INODE_POINTERS-1){
		//exclude superblock if it is used
		f_num_pointers--;
	}
	int final_inode_pointers[f_num_pointers];
	get_pointers(inode_index,final_inode_pointers,f_num_pointers);
	file_des_t[fileID].rw_pointer+=length;
	inodes[inode_index].size = return_filesize(inode_index);
	
	//writing into disk
	int inode_number = inode_index;
	int block_of_inode=inode_number/8+1; 
	char inode_update_buf[BLOCKSIZE];
	memset(inode_update_buf,0, BLOCKSIZE);
	read_blocks(block_of_inode, 1, inode_update_buf);
	memcpy((void *)inode_update_buf+(inode_number%8)*sizeof(INODE),(const void *) &inodes[inode_number], sizeof(INODE));
	write_blocks(block_of_inode, 1, inode_update_buf);	
	write_blocks(TOTAL_NUM_BLOCKS-1,1,free_bitmap);
	return num_bytes_to_be_written;
}
	

int sfs_fseek(int fileID, int loc){
	if(loc<0){
		return -1;
	}
	file_des_t[fileID].rw_pointer=loc;
	return 0;
}

int sfs_fread(int fileID, char *buf, int length){

	int num_bytes = 0;
	int num_pointers = 0;
	int inode_index = dir_table[fileID].inode_number;
	
	if(file_des_t[fileID].open==0){ 	 //check if file is opened
		return -1;
	}
	
	if(length>inodes[inode_index].size){
		length=inodes[inode_index].size; //attempt to read more data than the file has
	}

	num_pointers = get_num_pointers_inode(inode_index);
	
	if(num_pointers>INODE_POINTERS-1){
		num_pointers--;
	}
	int array_blockptr[num_pointers];
	get_pointers(inode_index,array_blockptr,num_pointers);//get pointers in inode into array_blockptr
	
	//find the number of blocks to read
	int rw_offset = file_des_t[fileID].rw_pointer % BLOCKSIZE;
	int rw_index  = file_des_t[fileID].rw_pointer / BLOCKSIZE + 1; 

	int num_blocks;
	if(length<(BLOCKSIZE-rw_offset)){ 
	    //read within one block
		char read_buf[BLOCKSIZE];
		memset(read_buf,0,BLOCKSIZE);
		read_blocks(array_blockptr[rw_index-1],1,read_buf);
		memcpy(buf,read_buf+rw_offset,length);
		file_des_t[fileID].rw_pointer+=length;
		return length;
	}
	else{
		num_blocks=(length-(BLOCKSIZE-rw_offset))/BLOCKSIZE+2; //+2 for the first and last partial block
	}
	
	int k;
	char read_buf[BLOCKSIZE];
	for(k=rw_index-1;k<rw_index+num_blocks-1;k++){
		memset(read_buf,0,BLOCKSIZE);
		read_blocks(array_blockptr[k],1,read_buf);
		if(k==rw_index-1){
			//first is partial block
			memcpy(buf,read_buf+rw_offset,BLOCKSIZE-rw_offset); //read only the remaining data
			num_bytes = num_bytes + (BLOCKSIZE-rw_offset);
		}
		else if(k==rw_index+num_blocks-2){
			//last block is partial block
			memcpy(buf+num_bytes, read_buf, (length-(BLOCKSIZE-rw_offset))%BLOCKSIZE);
			num_bytes = num_bytes + (length-(BLOCKSIZE-rw_offset))%BLOCKSIZE;
		}
		else{
			//full block
			memcpy(buf+num_bytes, read_buf, BLOCKSIZE);
			num_bytes = num_bytes + BLOCKSIZE;
		}
		
	}
	
	file_des_t[fileID].rw_pointer = file_des_t[fileID].rw_pointer + num_bytes;
	return num_bytes;
}



//received a block num and set it as free
void set_freeblock(int block_to_be_free){
	int i=(block_to_be_free-1)/8; //each byte has 8 bits which means 8 blocks in the bitmap
	char bit_to_be_updated=block_to_be_free%8; 
	if(bit_to_be_updated==1){free_bitmap[i]=free_bitmap[i]&127;} 
	else if(bit_to_be_updated==2){free_bitmap[i]=free_bitmap[i]&191;} //bitwise and with 10111111 
	else if(bit_to_be_updated==3){free_bitmap[i]=free_bitmap[i]&223;} 
	else if(bit_to_be_updated==4){free_bitmap[i]=(free_bitmap[i])&239;}
	else if(bit_to_be_updated==5){free_bitmap[i]=free_bitmap[i]&247;}
	else if(bit_to_be_updated==6){free_bitmap[i]=free_bitmap[i]&251;}
	else if(bit_to_be_updated==7){free_bitmap[i]=free_bitmap[i]&253;}
	else if(bit_to_be_updated==0){free_bitmap[i]=free_bitmap[i]&254;}	
}


int sfs_remove(char *file){
	int i;
	for(i=0;i<TOTAL_NUM_FILES;i++){
		if(strncmp(dir_table[i].filename, file, MAXFILENAME)==0){//find the file to be removed in directory table 
			//clear data
			int inode_index = dir_table[i].inode_number;
			int num_pointers_in_inode = get_num_pointers_inode(inode_index);
			int indirect_pointer = 0;
			if(num_pointers_in_inode>=INODE_POINTERS){
				//get the indirect block address
				indirect_pointer = inodes[inode_index].pointers[INODE_POINTERS-1];
				//and remove it from number of pointers
				num_pointers_in_inode--;
			}
			int array_blockptr[num_pointers_in_inode];
			get_pointers(inode_index, array_blockptr,num_pointers_in_inode);//get all pointers in array_blockptr
			//remove indirect pointer
			if(indirect_pointer){
				char buffer[BLOCKSIZE];//write a empty buffer into disk to overwrite indirect_pointer 
				memset(buffer,0,BLOCKSIZE);
				write_blocks(indirect_pointer,1,buffer);
				set_freeblock(indirect_pointer);//set it free again
			}
			
			//handle other pointers 
			//reset the inode
			INODE free_inode={
				.mode=0, 
				.link_cnt=0, 
				.uid=0, 
				.gid=0, 
				.size=0,
				.pointers={0,0,0,0,0,0,0,0,0,0,0,0,0} //set all the pointers to 0
			};

			int n;
			char empty_block_buf[BLOCKSIZE];
			for(n=0; n < num_pointers_in_inode; n++){
				memset(empty_block_buf,0,BLOCKSIZE);
				write_blocks(array_blockptr[n],1,empty_block_buf);
				set_freeblock(array_blockptr[n]);
			}
 			
			inodes[inode_index] = free_inode;

	        write_blocks(TOTAL_NUM_BLOCKS-1,1,free_bitmap); //update the free_bitmap

			//write the updated inode in disk
			int inode_number = inode_index;
			int block_of_inode=inode_number/8+1; 
			char update_inode_buffer[BLOCKSIZE];
			memset(update_inode_buffer,0, BLOCKSIZE);
			read_blocks(block_of_inode, 1, update_inode_buffer);
			memcpy((void *)update_inode_buffer+(inode_number%8)*sizeof(INODE),(const void *) &inodes[inode_number], sizeof(INODE));
			write_blocks(block_of_inode, 1, update_inode_buffer);	

			//set information in directory table to 0
			memset(dir_table[i].filename,0,MAXFILENAME);
			dir_table[i].inode_number=0;
			
			//update this information into disk
			char dir_entry_buffer[BLOCKSIZE];
			int block_dir_entry=i/NUM_ENTRIES_PERBLOCK+8+1;
			memset(dir_entry_buffer,0, BLOCKSIZE);
			read_blocks(block_dir_entry, 1, dir_entry_buffer);
			memcpy((void *)dir_entry_buffer+(i%21)*sizeof(dir_entry),(const void *) &dir_table[i], sizeof(dir_entry));
			write_blocks(block_dir_entry,1,dir_entry_buffer);

			//clear file descriptor table
			file_des_t[inode_index-1].open=0;
			file_des_t[inode_index-1].rw_pointer=0;
			return 0;
		}
	}
	//files does not exist, return -1
	return -1; 
}

