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
    SM_PageHandle newPageHandle;
    
    // Opens the filePage in read mode & write mode
    filePointer=fopen(fileName, "w+"); 
	
	//Using calloc function for memory allocation   
	newPageHandle = (SM_PageHandle)calloc(PAGE_SIZE,sizeof(char));
    
    //To check if the file is opened successfully
    if (filePointer!= NULL){
        //memset(memory,'\0',PAGE_SIZE); // using memset() fun fill the memeory allocated in previuos step with '\0' bytes
        int write=fwrite(newPageHandle,sizeof(char),PAGE_SIZE,filePointer);
        
        // checks if file write operation is successful or not
        if (write < PAGE_SIZE){
            printf("\n Page limit exceeded\n");
        }
        else{
            printf("\n Writing to a file successful!\n");
        }

        //Closing the file pointer
        fclose(filePointer);  
        return RC_OK;  
    }
    else{
        //RC_message = "File not found!";
        return RC_FILE_NOT_FOUND;
    }
}

//This method opens an existing file page.
extern RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{
    // Opening the file in read and write mode
    filePointer = fopen(fileName,"r+"); 
	int size_of_the_file, pages_in_Total;
    
    // Checking whether the file is opened or not
    if (filePointer!= 0)
    {
    	printf("Setting the pointer to the end of the file");
    	// Moving the pointer to the end of the file
        fseek(filePointer,0,SEEK_END); 

        // ftell() returns the size of the file after moving the pointer to the end of the file      
        size_of_the_file = ftell(filePointer);

        // Assigning the opened file name to the file handle parameters       
        fHandle->fileName = fileName;          
        
        // Calculating total number of pages in the file
		pages_in_Total = size_of_the_file / PAGE_SIZE; 


        //Initializing rest of the parameters of the file handle

        fHandle->totalNumPages = pages_in_Total;
        
        //Setting current page position to zero
        fHandle->curPagePos = 0;
        fHandle->mgmtInfo = filePointer;

        // Set the file pointer to the beginning of the file
        fseek(filePointer,0,SEEK_SET); 
        return RC_OK;
    }
    //if the file is not found
    else{
       
        return RC_FILE_NOT_FOUND;
    }

}

//This method closes an open file page.
extern RC closePageFile (SM_FileHandle *fHandle)
{
     if(filePointer!=NULL){
        fclose(fHandle->mgmtInfo);
        return RC_OK;
    }
    else {
        RC_message = "File not found";
        return RC_FILE_NOT_FOUND;
    }

}

//This method destroys the file page.
extern RC destroyPageFile (char *fileName)
{
    //Using fopen to check whether file exists or not
    filePointer=fopen(fileName,"r+"); 

    if(filePointer==NULL)  {
        RC_message = "File not found";
        return RC_FILE_NOT_FOUND;
    }
    else{
        //Using  remove function to delete a specified page file
    	remove(fileName); 
        return RC_OK;
    }

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
        RC_message = "File cannot be opened/does not exsit";
        return RC_FILE_NOT_FOUND;
    }

   //Checking if file handler is initialized. 
    if (fHandle == NULL){
        RC_message = "File handler is not initialized";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    
    //If the file has less than pageNum pages, the method should return RC_READ_NON_EXISTING_PAGE.
    if (fHandle->totalNumPages < pageNum) {
        RC_message = "Requested page does not exist.";
        return RC_READ_NON_EXISTING_PAGE;
    }

    //fseek is to set the file position of the stream to the given offset.
    int seekSuccess = fseek(fHandle->mgmtInfo, (PAGE_SIZE * sizeof(char) * pageNum), SEEK_SET);

    if(seekSuccess != 0){ 
        perror("Seek Failed!");
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
        RC_message = "File handler is not initialized.";
        return RC_FILE_HANDLE_NOT_INIT;
	}
	else{
        filePointer = fopen(fHandle->fileName, "r");
        // Check whether file is opened sucessfully.
        if(filePointer == NULL){
            RC_message = "File cannot be opened/does not exsit.";
            return RC_FILE_NOT_FOUND;
        }
        else{
            getBlockPosition = fHandle->curPagePos;
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
    
    //Checking if file handler is initialized
    if (fHandle == NULL || fHandle->fileName == NULL) {
        return RC_FILE_HANDLE_NOT_INIT;
    }
    //Check the existance of the pagenum if it is within the range.
    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        return RC_READ_NON_EXISTING_PAGE;
    }
    //Openening the file in WRITE mode with roles->r+b
    filePointer = fopen(fHandle->fileName, "r+b");
    if (filePointer == NULL) {
        return RC_FILE_NOT_FOUND;
    }

    //fseek to position
    fseek(filePointer, pageNum * PAGE_SIZE, SEEK_SET);

    // Write the content/data to the desired file 
    int seekSuccess = fwrite(memPage, sizeof(char), PAGE_SIZE, filePointer);
    if (seekSuccess != PAGE_SIZE) {
        fclose(filePointer);
        return RC_WRITE_FAILED;
    }

    //update the current page information 
    fHandle->curPagePos = pageNum;

    // Close the file after writing your information
    fclose(filePointer);

    return RC_OK;

}

//This method writes page to disk using current position.
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
   
    //Using the defined writeblock method to locate current page and to write the information
    if (fHandle == NULL) {
        RC_message = "File handler is not initialized.";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    
    int currentPosition = fHandle->curPagePos / PAGE_SIZE;
    return writeBlock(currentPosition, fHandle, memPage);
}

//This method increases number of pages in file by one by filling the newly created page with zero bytes.
extern RC appendEmptyBlock (SM_FileHandle *fHandle){

    filePointer = fopen(fHandle->fileName, "a");
    //Using calloc memory allocation funtion to create an empty block
    SM_PageHandle newEmptyPage = (char*)calloc(PAGE_SIZE, sizeof(char));
    
    //Checks and ensures file situation by checking if its null and return memory not allocated.
    if (newEmptyPage == NULL) {
       perror("Memory allocation failed.");
    }
    
    //This will help append an empty block if its empty. 
    fwrite(newEmptyPage, sizeof(char), PAGE_SIZE, filePointer);
    fHandle->totalNumPages++;
    free(newEmptyPage);
    fclose(filePointer);
    return RC_OK;

}


//This method ensures the capacity of the file. If the file has less than numberOfPages mentioned, then it increases the size to numberOfPages.
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){
    
    //Making sure more pages are added if insufficient
    int requiredPagesAdded = numberOfPages - (fHandle->totalNumPages);
    if (requiredPagesAdded > 0) {
        for (int i = 0; i < requiredPagesAdded; i++){
            appendEmptyBlock(fHandle);
        }
    }
    return RC_OK;

}
