//This is the implementation class for storage_mgr.h
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
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
	
    //To check if the file is opened successfully
    if (filePointer!= NULL){
        //memset(memory,'\0',PAGE_SIZE); // using memset() fun fill the memeory allocated in previuos step with '\0' bytes
        newPageHandle = (SM_PageHandle)calloc(PAGE_SIZE,sizeof(char));
        int write=fwrite(newPageHandle,sizeof(char),PAGE_SIZE,filePointer);
        printf("Create page file: %d\n",write);
        printf("PAGE SIZE: %d\n",PAGE_SIZE);
        // checks if file write operation is successful or not
        if (write < PAGE_SIZE){
            printf("\n Page limit exceeded\n");
        }
        else{
            printf("\n Writing to a file successful!\n");
        }

        //Closing the file pointer
        fclose(filePointer);  
	free(newPageHandle);
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
    filePointer = fopen(fileName,"r"); 
	int size_of_the_file, pages_in_Total;
    
    // Checking whether the file is opened or not
    if (filePointer!= NULL)
    {
        //printf("openPageFile: Inside if: file found: %s\n",fileName);
    	//printf("Setting the pointer to the end of the file");
    	// Moving the pointer to the end of the file
        //fseek(filePointer,0,SEEK_END); 

        // ftell() returns the size of the file after moving the pointer to the end of the file      
        size_of_the_file = ftell(filePointer);

        // Assigning the opened file name to the file handle parameters       
        fHandle->fileName = fileName;          
        
        // Calculating total file pages
	pages_in_Total = size_of_the_file / PAGE_SIZE; 


        //Initializing rest of the parameters of the file handle

       // fHandle->totalNumPages = pages_in_Total;

        struct stat fileInfo;
		if(fstat(fileno(filePointer), &fileInfo) < 0)    
			printf("error!");
		fHandle->totalNumPages = fileInfo.st_size/ PAGE_SIZE;
        
        //Setting current page position of fhandle to zero
        fHandle->curPagePos = 0;
        //fHandle->mgmtInfo = filePointer;
       // printf("openPageFile: fHandle->totalNumPages: %d \n",fHandle->totalNumPages);
        fHandle->mgmtInfo = filePointer;
        // Set the file pointer to the beginning of the file
        //fseek(filePointer,0,SEEK_SET);
        fclose(filePointer);
        return RC_OK;
    }
    //if the file is not found
    else{
         printf("openPageFile: file not found: %s\n",fileName);
         printf("openPageFile: fHandle->totalNumPages: %d \n",fHandle->totalNumPages);
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
    printf("-------In destroyPageFile------\n");
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

//This method reads the block at given position pageNum and stores it to memory pointed by mempage
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

    // Opening file with read access.	
	filePointer = fopen(fHandle->fileName, "r");
    
    
    if (pageNum < 0 || pageNum > fHandle->totalNumPages)
        	return RC_READ_NON_EXISTING_PAGE;


	if(filePointer == NULL)
    {
        printf("File is not found!!\n");
		return RC_FILE_NOT_FOUND;
    }
	int sizeOfPage = pageNum * PAGE_SIZE;
	int isSeekSuccess = fseek(filePointer,sizeOfPage, SEEK_SET);
	if(isSeekSuccess == 0) {
        int readStatus = fread(memPage, sizeof(char), PAGE_SIZE, filePointer);
		if(readStatus > PAGE_SIZE)
        {
            printf("Required page does not exist!\n");
			return RC_READ_NON_EXISTING_PAGE;
        }
	} else {
        printf("Error occured while reading the file!\n");
		return RC_ERROR; 
	}
    	
    // Using ftell(), updating the current position.
	fHandle->curPagePos = ftell(filePointer); 
	
   	//Close the file stream
	fclose(filePointer);
    return RC_OK;
}


//This method is used to return the current page position in a file.
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

//This method reads the previous page of the file. 
extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{ 
    int currentBlockPosition = fHandle->curPagePos / PAGE_SIZE;
    return readBlock(currentBlockPosition-1,fHandle,memPage);

}

//This method reads the current page of the file. 
extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    int currentBlockPosition = fHandle->curPagePos / PAGE_SIZE;
    return  readBlock(currentBlockPosition,fHandle,memPage);

}

//This method reads the next page  of the file. 
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

extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
    //Checking page range
	if (pageNum < 0 || pageNum > fHandle->totalNumPages)
    {
       printf("Writing failed!!\n");
    }
	
	// Opening file stream with read and write access.	
	filePointer = fopen(fHandle->fileName, "r+");
	
	if(filePointer == NULL)
    {
        printf("File not found!!\n");
		return RC_FILE_NOT_FOUND;
    }

	int beginningOfFile = pageNum * PAGE_SIZE;

	if(pageNum != 0) { 
        // Writing to first page
		fHandle->curPagePos = beginningOfFile;
        //closing the filePointer
		fclose(filePointer);
        //writing the currentblock on to disk.
		writeCurrentBlock(fHandle, memPage);
			
	} else {	
        //Writing on a random page(not first)
		fseek(filePointer, beginningOfFile, SEEK_SET);	
		int k;
		for(k = 0; k < PAGE_SIZE; k++) 
		{
			if(feof(filePointer)) //checking if end of file is reached
            {
				 appendEmptyBlock(fHandle);	
            }		
			fputc(memPage[k], filePointer); //used to write the character and update the filePointer position
		}

		// Using ftell to set current position of page.
		fHandle->curPagePos = ftell(filePointer); 

		// Closing file
		fclose(filePointer);
	}
	return RC_OK;
}

//This method writes current page to disk.
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
   
    //Opening file with read and write access
    filePointer = fopen(fHandle->fileName, "r+");

	if(filePointer == NULL)
    {
        printf("File not found!!\n");
		return RC_FILE_NOT_FOUND;
    }
	
    //Appending block
	appendEmptyBlock(fHandle);
    
    //Setting filePointer to current page position
	fseek(filePointer, fHandle->curPagePos, SEEK_SET);
	
    int memLength = strlen(memPage);
    //Writing contents on to disk of length memLength
	fwrite(memPage, sizeof(char),memLength,filePointer);
	
    //Using ftell to set current position of page.
	fHandle->curPagePos = ftell(filePointer);

	// Closing file   	
	fclose(filePointer);
	return RC_OK;	
}

//This method increases number of pages in file by one by filling the newly created page with zero bytes.
extern RC appendEmptyBlock (SM_FileHandle *fHandle){

    //filePointer = fopen(fHandle->fileName, "a");
    //Using calloc memory allocation funtion to create an empty block
    SM_PageHandle newEmptyPage = (char*)calloc(PAGE_SIZE, sizeof(char));
    
    //Checks and ensures file situation by checking if its null and return memory not allocated.
    if (newEmptyPage == NULL) {
       perror("Memory allocation failed.");
    }
    
    int isSeekSuccess = fseek(filePointer, 0, SEEK_END);
    //This will help append an empty block if its empty. 
    if(isSeekSuccess != 0 ) {
          printf("Writing on to disk failed!!\n");
          free(newEmptyPage);  //free the empty page created
    }
    else{
        fwrite(newEmptyPage, sizeof(char), PAGE_SIZE, filePointer);   //writing newEmptyPage on to disk
    }
    free(newEmptyPage); //free the empty page created
    fHandle->totalNumPages++;
    printf("In appendEmptyBlock method: File Handle total Number of pages: %d \n",fHandle->totalNumPages);	
    return RC_OK;

}


//This method ensures the capacity of the file.
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){
    printf("In ensureCapacity method:\n");
    printf("In ensureCapacity method: Number of pages: %d \n",numberOfPages);
    printf("In ensureCapacity method: File Handle total Number of pages: %d \n",fHandle->totalNumPages);	
    //Making sure more pages are added if insufficient
    //int requiredPagesAdded = numberOfPages - (fHandle->totalNumPages);
	//Opening file with append access
    filePointer = fopen(fHandle->fileName, "a");
	
	if(filePointer == NULL)
    {
        printf("File not found!!\n");
		return RC_FILE_NOT_FOUND;
    }
	
    int requiredPagesAdded = numberOfPages - (fHandle->totalNumPages);
    /* while(numberOfPages > fHandle->totalNumPages ) 
    {
            appendEmptyBlock(fHandle);
    } */

    for(int k=0; k<requiredPagesAdded;k++)
    {
        appendEmptyBlock(fHandle);
    }   
    fclose(filePointer);//closing the file.
    printf("In ensureCapacity method: END\n");
    return RC_OK;

}
