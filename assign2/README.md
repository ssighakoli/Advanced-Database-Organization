

# Assignment 2: BUFFER MANAGER

- The goal of this assignment is to implement a simple buffer manager - which manages a fixed number of pages in memory that represent pages from a page file managed by the storage manager implemented in the previous assignment. <br>
- The Buffer manager should be able to handle more than one open buffer pool at the same time. <br>
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

- We made sure that all the buffer pool functions are thread safe using pthread_mutex_lock and pthread_mutex_unlock.

## SOLUTION DESCRIPTION:

### BUFFER POOL FUNCTIONS:

A new buffer pool is creatd for a page file that already exists on disk. Here, while the existing page file is on disk, the newly created buffer pool is formed in memory. In otrder to perform different operations on this page file on disk, we use Storage Manager from Assignment 1.

#### 1.initBufferPool():

- This method is used to create and initialise a new memory buffer pool.
- This  method takes following inputs:
- numPages: size of the buffer or number of page frames that the buffer can store.
- pageFileName: Holds the page file's name for which the pages are being created.
- strategy: Holds the page replacement techniques used by the buffer pool(FIFO, LRU, LFU or CLOCK).
- stratData: Holds parameters required along with page replacement technique.
- Initially, check if buffer manager already exists. Return RC_BM_INVALID  error if buffer manager does not exists.
- Check for page file name , return RC_FILE_NOT_FOUND if file is not found.
- Now, initialise the buffer manager's(bm's) variables strategy, pageFile and numPages.
- Allocate total  memory required i.e, number of pages times size of each page.
- Now, set the values for buffer pool variables.
- Store the page frame in buffer pool and return RC_OK message.

#### 2.shutdownBufferPool():
- This method is used to destroy the buffer pool.
- Function now releases the memory assigned to all the uffer pool resources. 
- Use  forceFlush() to flush the buffer pool of all dirty pages before destroying it. 
- If any page is currently being accessed by a client, RC_PINNED_PAGES_IN_BUFFER error will be generated.
#### 3.forceFlushPool():
- This function writes the dirty pages (modified pages with dirtyBit value as 1) to the disk.
- The following two connditions are checked,
- It checks for all the page frames in the buffer pool, if dirtyBit = 1 (indicating that the page frame content has been modified by some client) and fixCount = 0 (indicating no user is using that page Frame).
- When both above mentioned conditions are satisfied, the page frame is written to the page file on disk.


### PAGE MANAGEMENT FUNCTIONS:
These functions are used to pin pages, unpin pages, mark pages as dirty, and force a page back to disk.<br>

#### 1.markDirty():
- This function is used to set the specified page frame's dirtyBit to 1.
- By repeatedly checking each page in the buffer pool, it locates the page frame using pageNum.
- When found, it sets dirtyBit = 1 for that page.

#### 2.pinPage():
- With the pinPage function, a page is retrieved from the buffer pool and pinned in memory. As input, it takes a pointer to the buffer pool structure (BM_BufferPool), a pointer to a BM_PageHandle structure that will hold the page data, and the page number (pageNum).

- If the requested page has already been placed in the buffer pool, the function simply returns a pointer to the page frame containing the page data and increases its fix count by 1.

- If the page is not in the buffer pool, the function reads it from disk and finds a free page frame to store it. The replacement strategy used to choose a page frame depends on the implementation of the buffer manager. Once a page frame has been chosen, the page data is read from disk and stored in the page frame. The function then updates the BM_PageHandle structure to point to the page frame and sets the fix count of the page to 1.

- In the absence of free page frames in the buffer pool, the function evicts one of the existing frames to make room for the new page.

- After pinning the page in memory, the function returns RC_OK to indicate success. If an error occurs, the function returns an appropriate error code

#### 3.unpinPage():
- This function is used to unpin the page. 
- The field pageNum is used for this purpose and the pin status of this page will be changed to 0.
##### This method is implemented as below:<br>
- Initially a mutex lock is attained.
- We iterate through the buffer pool; if page is found: set the pinStatus of the page to 0 and decrease the fixCount. If the fix count is negative, set it to 0.
- The lock will now be released.
- RC_OK is returned.

#### 4.forcePage():
- This function is used to write the current content of the page back to the page file on disk.<br>
##### This method is implemented as below:<br>
- Initially a mutex lock is attained.
- We iterate through the buffer pool; if page is found: we open the page and write it back to the disk using 'openPageFile' and 'writeBlock'methods of SM_FileHandle we implemented as part of assignment-1.
- After writing the page back to disk, we set dirty bit to 0. 
- The lock will now be released.
- RC_OK is returned.





### STATISTICS FUNCTIONS:
These functions return statistics about a buffer pool and its contents.<br>

#### 1.getFrameContents():
- This function returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame. <br>
- An empty page frame is represented using the constant NO_PAGE. <br>
##### This method is implemented as below:<br>
- Initially a mutex lock is attained.
- We allocate the memory for pageNumbers array using malloc function.
- We iterate through the buffer pool; if the pageNum is not -1 we return the page umber; else we will return a constant NO_PAGE.
- The lock will now be released.
- Page numbers array is returned.

#### 2.getDirtyFlags():
- This function returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty. <br>
- Empty page frames are considered as clean. <br>
##### This method is implemented as below:<br>
- Initially a mutex lock is attained.
- We allocate the memory for dirtyBits array of boolean using malloc function.
- We iterate through the buffer pool; if the dirty bit is 1, we return 0; else we will return false.
- The lock will now be released.
- Dirty bits array is returned.

#### 3.getFixCounts():
- This function returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame. <br>
- Return 0 for empty page frames<br>
##### This method is implemented as below:<br>
- Initially a mutex lock is attained.
- We allocate the memory for fixCounts array of integers using calloc function.
- We iterate through the buffer pool; if the fix count is -1, we return 0; else we will return the fixCount.
- The lock will now be released.
- Fix counts array is returned.

#### 4.getNumReadIO():
- This function returns the number of pages that have been read from disk since a buffer pool has been initialized.<br>
##### This method is implemented as below:<br>
- We have maintained a variable readCount from the beginning. So we will return that 'readCount' here.

#### 5.getNumWriteIO():
- This function returns the number of pages written to the page file since the buffer pool has been initialized.<br>
##### This method is implemented as below:<br>
- We have maintained a variable writeCount from the beginning. So we will return that 'writeCount' here.



