//This is the implementation class for storage_mgr.h
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "storage_mgr.h"

FILE *filePointer;


/* =====================================================================
-- MANIPULATING PAGE FILES 
========================================================================*/

//This method is used to initialize the Storage manager.
extern void initStorageManager (void){
    filePointer = NULL;
    printf("-----STORAGE MANAGER INITIALIZED-----------\n");
}

//This method creates a new page file fileName with one page filled with '\0' bytes
extern RC createPageFile (char *fileName){

}

//This method opens an existing file page.
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle){

}

//This method closes an open file page.
extern RC closePageFile (SM_FileHandle *fHandle){

}

//This method destroys the file page.
extern RC destroyPageFile (char *fileName){

}


/* =====================================================================
-- READING BLOCKS FROM DISC
========================================================================*/

//This method reads the block at given position pageNum from a file and stores its content in the memory pointed to by the memPage page handle.
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    //Opening the file in READ mode
    filePointer = fopen(fHandle->fileName, "r");
	
    //To check if the file is opened successfully
    if(filePointer == NULL){
        RC_message = "File cannot be opened/does not exsit."
        return RC_FILE_NOT_FOUND;
    }

   //Checking if file handler is initialized. 
    if (fHandle == NULL){
        RC_message = "File handler is not initialized."
        return RC_FILE_HANDLE_NOT_INIT;
    }
    
    //If the file has less than pageNum pages, the method should return RC_READ_NON_EXISTING_PAGE.
    if (fHandle->totalNumPages < pageNum) {
        RC_message = "Requested page does not exist."
        return RC_READ_NON_EXISTING_PAGE;
    }

    //fseek is to set the file position of the stream to the given offset.
    int seekSuccess = fseek(fHandle->mgmtInfo, (PAGE_SIZE * sizeof(char) * pageNum), SEEK_SET);

    if(seekSuccess != 0){ 
        RC_message = "Seek failed!"
        return RC_ERROR;
    }
    else{
        //Read the content and store it in the location pointed out by memPage page handle.
        fread(memPage, sizeof(char), PAGE_SIZE, filePointer);
        // Set the current page position to the pointer
	      fHandle->curPagePos = pageNum;
    }

	// Closing the file stream    	
	fclose(filePointer);	
  return RC_OK;
		
}

//This method returns the current page position in a file.
extern int getBlockPos (SM_FileHandle *fHandle)
{
    int getBlockPosition;
    //Checking if file handler is initialized. 
    if(fHandle == NULL){
        RC_message = "File handler is not initialized."
        return RC_FILE_HANDLE_NOT_INIT;
	}
	else{
        filePointer = fopen(fHandle->fileName, "r");
        // Check whether file is opened sucessfully.
        if(filePointer == NULL){
            RC_message = "File cannot be opened/does not exsit."
            return RC_FILE_NOT_FOUND;
        }
        else{
            getBlockPosition = fHandle->curPagePos
            return getBlockPosition;
        }
    }

}

//This method reads the first page in a file.
extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    return  readBlock(0,fHandle,memPage);
}

//This method reads the previous page relative to the current page position of the file. 
//The curPagePos should be moved to the page that was read. 
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{ 
    int currentBlockPosition = fHandle->curPagePos / PAGE_SIZE;
    return readBlock(currentBlockPosition-1,fHandle,memPage);

}

//This method reads the current page relative to the current page position of the file. 
//The curPagePos should be moved to the page that was read. 
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int currentBlockPosition = fHandle->curPagePos / PAGE_SIZE;
    return  readBlock(currentBlockPosition,fHandle,memPage);

}

//This method reads the next page relative to the current page position of the file. 
//The curPagePos should be moved to the page that was read. 
extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int currentBlockPosition = fHandle->curPagePos / PAGE_SIZE;
    return  readBlock(currentBlockPosition+1,fHandle,memPage);

}

//This method reads the last page in a file.
extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int lastBlockPosition = fHandle->totalNumPages - 1;
    return readBlock(lastBlockPosition, fHandle, memPage);

}


/* =======================================================================
-- WRITING BLOCKS TO DISC
==========================================================================*/

//This method writes page to disk using absolute position.
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){

}

//This method writes page to disk using current position.
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){

}

//This method increases number of pages in file by one by filling the newly created page with zero bytes.
extern RC appendEmptyBlock (SM_FileHandle *fHandle){

}

//This method ensures the capacity of the file. If the file has less than numberOfPages mentioned, then it increases the size to numberOfPages.
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){

}
