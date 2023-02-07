
# Assignment 1: STORAGE MANAGER

- The goal of this assignment is to implement a simple storage manager - a module that is capable of reading blocks from a file on disk into memory and writing blocks from memory to a file on disk.<br>
- The storage manager deals with pages (blocks) of fixed size (PAGE_SIZE). <br>
- In addition to reading and writing pages from a file, it provides methods for creating, opening, and closing files. <br>
- The storage manager has to maintain several types of information for an open file: The number of total pages in the file, the current page position (for reading and writing), the file name, and a POSIX file descriptor or FILE pointer. <br>

## Steps to run the script:

1. 


## SOLUTION DESCRIPTION:

### MANIPULATING FILE RELATED METHODS:

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
- checks and ensures file situation by checking if its null and return memory not allocated. 
- This will help append an empty block if its empty.

#### ensureCapacity()
- Making sure more pages are added if insufficient.

