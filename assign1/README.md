
# Assignment 1: STORAGE MANAGER

- The goal of this assignment is to implement a simple storage manager - a module that is capable of reading blocks from a file on disk into memory and writing blocks from memory to a file on disk.<br>
- The storage manager deals with pages (blocks) of fixed size (PAGE_SIZE). <br>
- In addition to reading and writing pages from a file, it provides methods for creating, opening, and closing files. <br>
- The storage manager has to maintain several types of information for an open file: The number of total pages in the file, the current page position (for reading and writing), the file name, and a POSIX file descriptor or FILE pointer. <br>

## Steps to run the script:

1. Connect to fourier server using the command: ssh username@fourier.cs.iit.edu
2. Clone to the git repository using the commnd: git clone https://github.com/IITTeaching/cs525-s23-group-6.git
3. Change to 'cs525-s23-group-6' using the command: cd cs525-s23-group-6
4. List the files in the directory using the command: ls
5. Change to 'assign1' folder using the command: cd assign1
6. List the files in the directory using the command: ls
7. Use the command 'make clean' to delete old .o files of test1 and additional test.
8. Use the command 'make' to compile all files in the directory.
9. Use the command 'make run_test_1' to run test1.
10. Use the command 'make test_additional' to combine the additional test.
11. Use the command 'make run_test_additional' to run the additional test case.


### Additional functionality included:
- We made sure that filePointer is closed whenever opened to avoid memory leaks.
- We have written an additional test case file to test readFirstBlock, readNextBlock, readPreviousBlock, writeBlock, writeCurrentBlock, ensureCapacity methods.


## SOLUTION DESCRIPTION:

### MANIPULATING FILE RELATED METHODS:

#### extern void initStorageManager (void)
Initialize the Storage Manager and assign filePointer to null.

#### extern RC createPageFile(char *fileName)

- Initialising a file pointer and Opening file stream in 'w+' mode (read & write mode). It creates an empty file with both reading and writing operations.
- Use calloc() function to allocate required block of memory.
- Validate if the file already exists. If yes, write to the file and close the file using fclose() function.
- Else, return RC_FILE_NOT_FOUND.

#### extern RC openPageFile(char *fileName, SM_FileHandle *fHandle)

- This method uses the previously created page file. Opens file stream in 'r+' mode (read & write mode). It creates an empty file with both reading and writing operations.
- Check if the file exists. If yes, File handler parameters are assigned,fileName is assigned with the name of the current file, curPagePos is set to zero to mark the beginning of the page, the file pointer is assigned to mgmtInfo.
- Move the pointer to EOF.
- Total number of pages is calculated by dividing the file size calculated by using ftell() by size of each page.
Now, set the calculated values to file handle parameters and use fseek() function to set the file pointer back to file beginning.
- Return RC_OK.
- Else, return RC_FILE_NOT_FOUND

#### extern RC closePageFile(SM_FileHandle *fHandle)

- Using one of the file pointer assigned before i.e., mgmtInfo check if file is null. If yes, return RC_FILE_NOT_FOUND. Else, close the file using fclose() and return RC_OK status.

#### extern RC destroyPageFile(char *fileName)
- Check if the file exists. If Yes, use remove() command to destroy the page .
- Else, return RC_FILE_NOT_FOUND.



### READING BLOCKS FROM DISC RELATED METHODS:

The read related methods are used to read blocks of data from the page file into the disk.

#### readBlock()
This method reads the block at position pageNum from a file and stores its content in the memory pointed to by the memPage page handle. 
Below are the steps followed inside readBlock:
- First we open the file in read mode.
- Check if the file handler is initialized or not.
- Check if the requested page exists or not, if it does not exist, then return RC_READ_NON_EXISTING_PAGE.
- Then check if the file is opened successfully.
- Use fseek is to set the file position of the stream to the given offset.
- Read the content and store it in the location pointed out by memPage page handle.
- Set the current page position to the pointer.
- Close the file stream.

#### getBlockPos()
This method returns the current page position in a file. 
Below are the steps followed inside getBlockPos:
- Check if the file handler is initialized or not.
- Check if the file is opened successfully.
- Get the current position of the block.

#### readFirstBlock()
This method reads the first page in a file. 
Below are the steps followed inside readFirstBlock:
- Use the above readBlock() method and set pageNum to 0.

#### readPreviousBlock()
This method reads the previous page relative to the current page position of the file.The curPagePos should be moved to the page that was read. 
Below are the steps followed inside readPreviousBlock: 
- Calculate the current block position.
- Use the above readBlock() method and set pageNum to (current block position - 1).

#### readCurrentBlock()
This method reads the current page relative to the current page position of the file.The curPagePos should be moved to the page that was read. 
Below are the steps followed inside readCurrentBlock:
- Calculate the current block position.
- Use the above readBlock() method and set pageNum to current block position.

#### readNextBlock()
This method reads the next page relative to the current page position of the file.The curPagePos should be moved to the page that was read. 
Below are the steps followed inside readNextBlock:
- Calculate the current block position.
- Use the above readBlock() method and set pageNum to (current block position + 1).

#### readLastBlock()
This method reads the last page in a file. 
Below are the steps followed inside readLastBlock:
- Calculate the last block position as (total pages - 1).
- Use the above readBlock() method and set pageNum to last block position.


### WRITING BLOCKS TO DISC RELATED METHODS:

The WriteBlock  methods are used to Write blocks of data from the page file into the disk.

#### writeBlock()

- Check the existance of the pagenum if it is within the range.
- Openening the file in WRITE mode with roles->r+b
- fseek to position
- Write the content/data to the desired file 
- Update the current page information 
- Close the file after writing your information

####  writeCurrentBlock()

- Using the defined writeblock method to locate current page and to write the information on the current positioned block

#### appendEmptyBlock()
This method uses the exist writeblock to update an empty block.
- checks and ensures file situation by checking if its null and return memory not allocated. 
- This will help append/update an empty block if its empty.

#### ensureCapacity()
- Making sure more pages are added if insufficient.
- Using this method, we can ensure the total number of pages is equal to the intended number of pages.

