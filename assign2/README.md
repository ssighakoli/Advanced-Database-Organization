# Assignment 2: BUFFER MANAGER

- The goal of this assignment is to implement a simple buffer manager - which manages a fixed number of pages in memory that represent pages from a page file managed by the storage manager implemented in the previous assignment. <br>
- The Buffer manager should be able to handle more than one open buffer pool at the same time. However, there can only be one buffer pool for each page file. <br>
- It uses one page replacement strategy whenever the buffer pool is initialized.

## What is a buffer pool?

A buffer pool is a region of memory in a computer system that is used to temporarily store data that has been read from or is about to be written to a storage device, such as a hard disk or solid-state drive. The purpose of a buffer pool is to improve the efficiency of I/O operations by reducing the number of accesses to the underlying storage device. 
In a database system, the buffer pool is used to cache frequently accessed data pages from the disk. By keeping frequently used data in memory, the database system can reduce the number of disk I/O operations required to retrieve data, which can significantly improve overall system performance.


## Steps to run the script:

1. Connect to fourier server using the command: ssh username@fourier.cs.iit.edu
2. Clone to the git repository using the commnd: git clone https://github.com/IITTeaching/cs525-s23-group-6.git
3. Change to 'cs525-s23-group-6' using the command: cd cs525-s23-group-6
4. List the files in the directory using the command: ls
5. Change to 'assign2' folder using the command: cd assign2
6. List the files in the directory using the command: ls
7. Use the command 'make clean' to delete old .o files of test1 and additional test.
8. Use the command 'make' to compile all files in the directory.
9. Use the command 'make run_test_1' to run test1.
10. Use the command 'make test_additional' to combine the additional test.
11. Use the command 'make run_test_additional' to run the additional test case.



## Additional functionality included for extra credit:

- We made sure that all the buffer pool functions are thread safe.
- In addition to the two minimum page replacement strategies FIFO and LRU, we have also implemented CLOCK, LFU and LFU_K.

## SOLUTION DESCRIPTION:

### Buffer pool functions:

#### initBufferPool():
#### shutdownBufferPool():
#### forceFlushPool():

### Page Management functions:
These functions are used pin pages, unpin pages, mark pages as dirty, and force a page back to disk.<br>

#### markDirty():

#### unpinPage():
- This function is used to unpin the page. The field pageNum is used for this purpose.<br>

This is implemented as below:<br>
-

#### forcePage():
- This function is used to write the current content of the page back to the page file on disk.<br>

This is implemented as below:<br>
-

#### pinPage():


### Statistics functions:
These functions return statistics about a buffer pool and its contents.<br>

#### getFrameContents():
- This function returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame. <br>
- An empty page frame is represented using the constant NO_PAGE. <br>

This is implemented as below: <br>
-

#### getDirtyFlags():
- This function returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty. <br>
- Empty page frames are considered as clean. <br>

This is implemented as below: <br>
-

#### getFixCounts():
- This function returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame. <br>
- Return 0 for empty page frames<br>

This is implemented as below:<br>
-

#### getNumReadIO():
- This function returns the number of pages that have been read from disk since a buffer pool has been initialized.<br>

This is implemented as below:<br>
-

#### getNumWriteIO():
- This function returns the number of pages written to the page file since the buffer pool has been initialized.<br>

This is implemented as below:<br>
-




