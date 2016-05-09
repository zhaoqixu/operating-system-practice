Data structure used in this design:

-------------------------------------------------
|T	|FREE	|			|T	|
|A	|LIST	|			|A	|
|G	|NODE	|	   DATA		|G	|
|	|	|			|	|
-------------------------------------------------
^	^		^		^
1	2		3	        4
	

I used double tags scheme for each data segmentation(block).
The same metadata for one segmentation is stored in these two tags.
Each tag has 4 bytes, in which the last bit of the 4 bytes represents whether
the block is free or not, the rest represent the size of the block.

Thus when we want to retrieve the size information of one block,
we use bitwise opertion ">>" to get the size information, and 
when we want to know whether it is free or not,we use &0b1 or &0b0.

1,4 is the start pointer for tag
2 is the pointer in the free list 
3 is the start pointer for data stored in the segmentation.

If the my_malloc(size) where size < 1 or my_mallopt(policy)
where policy != 1 or 2, my_malloc will return Segmentation fault.
As the assignment does not mention how to handle this, I just leave it
there.Also I think this is reasonable because My_malloc return NULL pointer.

I tested my code by allocating different size of memory and then free.
I printed out the changes everytime after I my_malloc or my_free and the results
turned to reasonbale according to how I allocated and freed.

For testing: 1   make
	     2   ./test 
