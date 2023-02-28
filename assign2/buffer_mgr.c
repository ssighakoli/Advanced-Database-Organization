#include<stdio.h>
#include<stdlib.h>
#include <pthread.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>

pthread_mutex_t lock; //mutex lock to make the methods thread safe

//This variable indicates the maximum buffer size.
int maxBufferSize = 0;

//This variable indicates the current buffer size.
int currentBufferSize = 0;

//This varaible keeps track of the number of writes to the disk.
int writeCount = 0;

//This variable keeps track of the number of reads from the disk.
int readCount = 0;

//locking system to make the method thread safe
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct PageFrame
{
    SM_PageHandle data; //Contains actual data
    PageNumber pageNum;
    int dirtyBit; // Will be 1 if dirty, 0 by default.
    int fixCount;
    int pinStatus; //Will be 1 if pinned, 0 if unpinned.
    int freeStat;
    int hitCount; //Will be incremented for every hit.
} PageFrame;


int rearIndexOfQueue=0;

//Returns the index of the frame to be evicted using CLOCK strategy.
 
//Replacement strategies

//FIFO Strategy
extern void FIFO_PageReplacementStrategy(BM_BufferPool *const bm, PageFrame *page)
{
    printf("In FIFO page replacement strategy..START\n");
    PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
    
    int frontIndexOfQueue,i;

    frontIndexOfQueue = rearIndexOfQueue % currentBufferSize;

    for(i = 0; i < currentBufferSize; i++)
	{
		if(pageFrame[frontIndexOfQueue].fixCount == 0)
		{
                   if(pageFrame[frontIndexOfQueue].dirtyBit == 1)
                   {
                      printf("In FIFO page replacement strategy..Found dirty bit..Current buffer size: %d\n",currentBufferSize);
                      
		              SM_FileHandle fileHandler;	   
                      openPageFile(bm->pageFile, &fileHandler);
                      printf("In FIFO page replacement strategy..Found dirty bit, front index %d\n",frontIndexOfQueue);
                      printf("In FIFO page replacement strategy..Found dirty bit, writing to disk: %s\n",pageFrame[frontIndexOfQueue].data);
		              writeBlock(pageFrame[frontIndexOfQueue].pageNum, &fileHandler, pageFrame[frontIndexOfQueue].data);
                      writeCount++;
                   }
			    pageFrame[frontIndexOfQueue].data = page->data;
			    pageFrame[frontIndexOfQueue].pageNum = page->pageNum;
			    pageFrame[frontIndexOfQueue].dirtyBit = page->dirtyBit;
			    pageFrame[frontIndexOfQueue].fixCount = page->fixCount;
			    break;
		}
            else{
                frontIndexOfQueue++;
			    frontIndexOfQueue = (frontIndexOfQueue% currentBufferSize == 0) ? 0 : frontIndexOfQueue;
            }
        }
    printf("In FIFO page replacement strategy..Found dirty bit, front index %d\n",frontIndexOfQueue);    
    printf("In FIFO page replacement strategy..END\n");
}

//LRU Strategy
extern void LRU_PageReplacementStrategy(BM_BufferPool *const bm, PageFrame *page)
{
    printf("In LRU page replacement strategy..START\n");
    PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
    SM_FileHandle fileHandler;

	int i, leastHitIndex, leastHitNum;

	// Interating through all the page frames in the buffer pool.
	for(i = 0; i < currentBufferSize; i++)
	{
		// Finding page frame whose fixCount = 0 i.e. no client is using that page frame.
		if(pageFrame[i].fixCount == 0)
		{
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitCount;
			break;
		}
	}	

	// Finding the page frame having minimum hitNum (i.e. it is the least recently used) page frame
	for(i = leastHitIndex + 1; i < currentBufferSize; i++)
	{
		if(pageFrame[i].hitCount < leastHitNum)
		{
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitCount;
		}
	}

	// If page in memory has been modified (dirtyBit = 1), then write page to disk
	if(pageFrame[leastHitIndex].dirtyBit == 1)
	{
		openPageFile(bm->pageFile, &fileHandler);
		writeBlock(pageFrame[leastHitIndex].pageNum, &fileHandler, pageFrame[leastHitIndex].data);
		
		// Increase the writeCount which records the number of writes done by the buffer manager.
		writeCount++;
	}
	
	// Setting page frame's content to new page's content
	pageFrame[leastHitIndex].data = page->data;
	pageFrame[leastHitIndex].pageNum = page->pageNum;
	pageFrame[leastHitIndex].dirtyBit = page->dirtyBit;
	pageFrame[leastHitIndex].fixCount = page->fixCount;
	pageFrame[leastHitIndex].hitCount = page->hitCount;

    printf("In LRU page replacement strategy..END\n");

}


// BUFFER POOL FUNCTIONS

//This method is used to create and initialize the buffer pool.
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    printf("InitBufferPool: initializing buffer pool\n");
    pthread_mutex_lock(&mutex);//lock initiated
    int k=0;
    int size_bm=0;
	if(bm==NULL) // checking if the buffer manager bm exists or not
		printf("BM is null");
	if(pageFileName==NULL)
		return RC_FILE_NOT_FOUND;
	//Initialising the buffer manager's variables
    bm->strategy = strategy;
	bm->pageFile = (char *)pageFileName;
    bm->numPages = numPages;
	int size_pageFrame=sizeof(PageFrame);
    //allocating memory required = (size of one page)* (number of pages)
	PageFrame *pg = malloc(numPages * size_pageFrame); 

	//Setting values for  buffer pool variables
	size_bm = numPages;	
	writeCount = 0;
    currentBufferSize = numPages;
    maxBufferSize = numPages;
	// Check if all pages fields are equal to NULL 
    for(k = 0; k < currentBufferSize; k++)
	{
        pg[k].data=NULL;
        pg[k].dirtyBit=0;
        pg[k].fixCount = 0;
        pg[k].pinStatus=0;
        pg[k].pageNum= -1;
        pg[k].freeStat=0;
    }
	//Page frame is stored in the bufferpool
	bm->mgmtData = pg;



	pthread_mutex_unlock(&mutex);
    printf(" initBufferPool: end\n");
	
	return RC_OK;		
}


//This method is used to destroy the buffer pool.
RC shutdownBufferPool(BM_BufferPool *const bm)
{
    printf("shutdownBufferPool: Start\n");
    pthread_mutex_unlock(&mutex);	
    pthread_mutex_lock(&mutex);//Acquire lock
    int k=0;
	if(bm->mgmtData==NULL)
		printf("BM is null");
	PageFrame *pFrame;
	pFrame = (PageFrame *)bm->mgmtData;
	// All the modified pages are written back to disk
	forceFlushPool(bm);

	while(k < currentBufferSize)
	{
		if(pFrame[k].fixCount != 0)
		{
                  // return RC_PINNED_PAGES_IN_BUFFER;
                   printf("Pinned pages exist");
		}
		k++;
	}
	//forceFlushPool(bm);
	free(pFrame); // releasing the memory to avoid unneccessary leaks
	bm->mgmtData = NULL;
	pthread_mutex_unlock(&mutex);
    printf("shutdownBufferPool: end\n");
	return RC_OK;


}

//This method is used to write all dirty pages from buffer pool to disk.
RC forceFlushPool(BM_BufferPool *const bm)
{
    printf("forceFlushPool: Start\n");
	pthread_mutex_unlock(&mutex);
	pthread_mutex_lock(&mutex);//Acquire lock
	


	PageFrame *pFrame;
	pFrame = (PageFrame *)bm->mgmtData;
	if(bm->mgmtData==NULL)
		printf("BM is invalid");
        int k;
        for(k = 0; k < currentBufferSize; k++)
	{
        if(pFrame[k].fixCount==0 && pFrame[k].dirtyBit==1)
        {
	    printf("forceFlushPool: Writing modified pages to disk:\n");	
	    SM_FileHandle fileHandler;
            // Writing modified pages to disk
            openPageFile(bm->pageFile, &fileHandler);
            //ensureCapacity(pFrame[k].pageNum, &fhandle);
	    printf("forceFlushPool: Writing modified pages to disk: Data: %s\n",pFrame[k].data);	
            writeBlock(pFrame[k].pageNum, &fileHandler, pFrame[k].data);
            pFrame[k].dirtyBit=0;
            writeCount++;
        }
    }
        pthread_mutex_unlock(&mutex);
	printf("forceFlushPool: End\n");
	return RC_OK;


}


// PAGE MANAGEMENT FUNCTIONS

//This method is used to mark a page frame as dirty.
extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    pthread_mutex_unlock(&mutex);	
    pthread_mutex_lock(&mutex);//Acquire lock
    int k=0;
	PageFrame *pFrame;
	pFrame = (PageFrame *)bm->mgmtData;

    while(k<currentBufferSize)
    {
        if(pFrame[k].pageNum == page->pageNum)
        {
            pFrame[k].dirtyBit=1;
            return RC_OK;
        }
        k++;

    }

    //return RC_BM_PAGE_FRAME_NOT_FOUND;
	
    pthread_mutex_unlock(&mutex);
    return RC_OK;


}

//This method is used to unpin the page.
extern RC unpinPage(BM_BufferPool* const bm, BM_PageHandle* const page)
{
    PageFrame* pageFrame = (PageFrame*)bm->mgmtData;
        pthread_mutex_unlock(&mutex); 
	pthread_mutex_lock(&mutex);//Acquire lock
    int i;
    for (i = 0; i < currentBufferSize; i++) //Iterating through the buffer to find the current page.
    {
        if (pageFrame[i].pageNum == page->pageNum) //If the given page's pageNum matches with that of current page
        {
            printf("unpinPage: page found to unpin with page number: %d\n",page->pageNum);
            pageFrame[i].pinStatus = 0;
            pageFrame[i].fixCount--;
	    break;		
        }
    }

    pthread_mutex_unlock(&mutex); //Release lock
    return RC_OK;
}


//This method is used to pin the page with pagenumber pageNum.
extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) 
{
    printf("In pin page method for Page number: %d\n",pageNum);
   
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;

    //Check if the buffer is empty and if this is the first page to be added to the buffer pool.
    if(pageFrame[0].pageNum == -1)
	{
        SM_FileHandle fileHandler;
        openPageFile(bm->pageFile, &fileHandler); //Method from SM_FileHandle of assignment-1
		pageFrame[0].data = (SM_PageHandle) malloc(PAGE_SIZE); //Method from SM_FileHandle of assignment-1
		ensureCapacity(pageNum,&fileHandler); //Method from SM_FileHandle of assignment-1
		readBlock(pageNum, &fileHandler, pageFrame[0].data); //Method from SM_FileHandle of assignment-1
		pageFrame[0].pageNum = pageNum;
		pageFrame[0].fixCount++;
		rearIndexOfQueue = 0; 
		pageFrame[0].hitCount = 0;	//Setting hitCount to 0 as it was not a hit.
		page->pageNum = pageNum;
		page->data = pageFrame[0].data;
        printf("In first if block: page number is: %d\n", page->pageNum); 
        printf("In first if block: page data is: %s\n", page->data); 
	    printf("In first if block: page frame data is: %s\n", pageFrame[0].data);    
        printf("If block exited!\n");
		return RC_OK;		
    }
    else
    {
        printf("In firse else block!\n");
        int i;
        bool isCurrentBufferFull = true;
        for(i = 0; i < currentBufferSize; i++)
		{
		if(pageFrame[i].pageNum != -1)
		  {
            if(pageFrame[i].pageNum == pageNum) //Meaning, its a hit
			{
                printf("In Else - if block!\n");
                pageFrame[i].fixCount++;
			    isCurrentBufferFull = false;
                pageFrame[i].hitCount = pageFrame[i].hitCount + 1; 
                page->pageNum = pageNum;
		        page->data = pageFrame[i].data;
                printf("In Else - if block: page data is: %s\n", page->data); 
                break;
            }
		 }else //Meaning, it is not a hit and page needs to be read from disk
            {
                SM_FileHandle fileHandler;
                printf("In Else - else block!\n");
				openPageFile(bm->pageFile, &fileHandler);
				pageFrame[i].data = (SM_PageHandle) malloc (PAGE_SIZE);
                printf("In else - else block: page frame data before readBlock is: %s\n", pageFrame[i].data);
                printf("In else - else block: page pageNum before readBlock is: %d\n", pageNum);
				readBlock(pageNum, &fileHandler, pageFrame[i].data);
				printf("In else - else block: page frame data after readBlock is: %s\n", pageFrame[i].data);
				pageFrame[i].pageNum = pageNum;
				pageFrame[i].fixCount = 1;
				pageFrame[i].hitCount = pageFrame[i].hitCount + 1;
				rearIndexOfQueue++;	
				page->pageNum = pageNum;
				page->data = pageFrame[i].data;
				isCurrentBufferFull = false;
                printf ("i value is: %d\n",i);
                printf("In else - else block: page frame data is: %s\n", pageFrame[i].data);
                printf("In else - else block: page number is: %d\n", page->pageNum);  
                printf("In else - else block: page data is: %s\n", page->data); 
				break;
			}

        }

        if(isCurrentBufferFull == true) //means that the current buffer is full and we must replace any existing page using any of the page replacement strategy
        {
        printf("In isCurrentBufferFull block!\n");
           PageFrame *newPageToBeWritten = (PageFrame *) malloc(sizeof(PageFrame));		
		   SM_FileHandle fileHandler;
			openPageFile(bm->pageFile, &fileHandler);
			newPageToBeWritten->data = (SM_PageHandle) malloc(PAGE_SIZE);
			readBlock(pageNum, &fileHandler, newPageToBeWritten->data);
			newPageToBeWritten->pageNum = pageNum;
			newPageToBeWritten->dirtyBit = 0;		
			newPageToBeWritten->fixCount = 1;
			rearIndexOfQueue++;
			newPageToBeWritten-> hitCount ++;
			page->pageNum = pageNum;
			page->data = newPageToBeWritten->data;			

			// Call appropriate algorithm's function depending on the page replacement strategy selected (passed through parameters)
			switch(bm->strategy)
			{			
				case RS_FIFO: 
					FIFO_PageReplacementStrategy(bm, newPageToBeWritten);
					break;
				
				case RS_LRU: 
					LRU_PageReplacementStrategy(bm, newPageToBeWritten);
					break;
				
				case RS_CLOCK:
                    printf("\n Clock algorithm not implemented");
					break;
  				
				case RS_LFU: 
                    printf("\n LFU algorithm not implemented");
					break;
  				
				case RS_LRU_K:
					printf("\n LRU-k algorithm not implemented");
					break;
				
				default:
					printf("\nAlgorithm Not Implemented\n");
					break; 

            }
        }

             
    }
    return RC_OK;
    
}

//This method is used to write the current content of the page back to the page file on disk.
extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    pthread_mutex_unlock(&mutex);	
    pthread_mutex_lock(&mutex);//Acquire lock
    PageFrame *pageFrame = (PageFrame *)bm->mgmtData;
    int i;

    for(i = 0; i < currentBufferSize; i++) //Iterating through the buffer to find the current page.
    {
        if(pageFrame[i].pageNum == page->pageNum) //If page is found
        {
            SM_FileHandle fileHandler;
            openPageFile(bm->pageFile, &fileHandler);
            //writeBlock method writes the current page on to the disk. This method was implemented in SM_FileHandle of assignment-1.
            writeBlock(pageFrame[i].pageNum, &fileHandler, pageFrame[i].data); 
            writeCount++; //increase the writeCount
            pageFrame[i].dirtyBit= 0; //Set dirty bit to 0 as page is written back to disk.
            break;
        }
    }
    pthread_mutex_unlock(&mutex); //Release lock
    return RC_OK;
}

// STATISTICS FUNCTIONS

//This method returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame. 
//An empty page frame is represented using the constant NO_PAGE.
PageNumber* getFrameContents(BM_BufferPool* const bm)
{
    printf("getFrameContents: start\n");
    PageFrame* pageFrame;
    PageNumber* pageNumbers;
    pageFrame = (PageFrame*)bm->mgmtData;
    pageNumbers = malloc(sizeof(PageNumber) * currentBufferSize); //Allocating memory for pageNumbers
    for (int i = 0; i < maxBufferSize; i++) { //Iterating through the buffer.
        pageNumbers[i] = (pageFrame[i].pageNum != -1) ? pageFrame[i].pageNum : NO_PAGE;
    }
    printf("getFrameContents: end\n");
    return pageNumbers;
}

//This method returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty. 
bool* getDirtyFlags(BM_BufferPool* const bm)
{
    printf("getDirtyFlags: start\n");
    bool* dirtyFlags = malloc(sizeof(bool) * currentBufferSize);
    PageFrame* pageFrame;
    pageFrame = (PageFrame*)bm->mgmtData;
    for (int i = 0; i < maxBufferSize; i++) { //Iterating through the buffer.
        dirtyFlags[i] = (pageFrame[i].dirtyBit == 1) ? true : false; //Finds if dirtyBit is set/not.
    }
    printf("getDirtyFlags: end\n");
    return dirtyFlags;
    free(dirtyFlags); //will not be executed, so can be removed.
}


//This method returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame. 
//Returns 0 for empty page frames.
int* getFixCounts(BM_BufferPool* const bm)
{
    printf("getFixCounts: start\n");
    int* fixCounts = calloc(currentBufferSize, sizeof(int));
    PageFrame* pageFrame;
    pageFrame = (PageFrame*)bm->mgmtData;
    for (int i = 0; i < maxBufferSize; i++) //Iterating through the buffer.
    {
        fixCounts[i] = (pageFrame[i].fixCount != -1) ? pageFrame[i].fixCount : 0; //finds if fixedCount is set/not.
    }
    printf("getFixCounts: end\n");
    return fixCounts;
    free(fixCounts); //will not be executed, so can be removed.
}

//This method returns the number of pages that have been read from disk since a buffer pool has been initialized.
int getNumReadIO(BM_BufferPool* const bm)
{
    printf("getNumReadIO: start\n");
    return readCount; //from the readCount we initialized in the beginning.
}

//This method returns the number of pages written from disk since a buffer pool has been initialized.
int getNumWriteIO(BM_BufferPool* const bm)
{
    printf("getNumWriteIO: start\n");
    return writeCount; //from the writeCount we initialized in the beginning.
}
