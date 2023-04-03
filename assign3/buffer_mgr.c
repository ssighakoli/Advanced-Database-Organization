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
int numberOfWrites = 0;

//This variable keeps track of the number of reads from the disk.
int numberOfReads = 0;

//locking system to make the method thread safe
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct PageFrame
{
    SM_PageHandle pageContent; //Contains actual data
    PageNumber pageNum;
    int dirtyBit; // Will be 1 if dirty, 0 by default.
    int fixCount;
    int hitForLRU; //Will be incremented for every hit.
} PageFrame;

int hitCount=0;

//This method returns true if a particular index in a page frame is marked dirty.
bool isDirty(PageFrame *pageFrame, int index)
{
    bool isDirty = false;
     if(pageFrame[index].dirtyBit == 1)
        {
             isDirty = true;;   
        }
    return isDirty;
} 

//This method copies the content of one page frame to another 
void updatePageFrame(PageFrame *pageFrame1, PageFrame *pageFrame2,int index)
{
    pageFrame1[index].pageContent = pageFrame2->pageContent;
    pageFrame1[index].fixCount = pageFrame2->fixCount;
    pageFrame1[index].pageNum = pageFrame2->pageNum;
    pageFrame1[index].dirtyBit = pageFrame2 ->dirtyBit;
    pageFrame1[index].hitForLRU = pageFrame2 ->hitForLRU;
    
} 

//Set the initial contents of the bufferpool
void initiateBufferPool(PageFrame *initialPageFrame,int index)
{
        initialPageFrame[index].pageContent=NULL;
        initialPageFrame[index].dirtyBit=0;
        initialPageFrame[index].fixCount = 0;
        initialPageFrame[index].pageNum= -1;
        initialPageFrame[index].hitForLRU = 0;

}
//Replacement strategies

//FIFO Strategy
extern void FIFO_PageReplacementStrategy(BM_BufferPool *const bm, PageFrame *page)
{
    PageFrame *fifoFrame = (PageFrame *) bm->mgmtData;
    
    int frontIndexOfQueue,k,lastIndexOfQueue;
    lastIndexOfQueue = numberOfReads;

    frontIndexOfQueue = lastIndexOfQueue % currentBufferSize;

    for(k = 0; k < currentBufferSize; k++)
	{
		if(fifoFrame[frontIndexOfQueue].fixCount == 0)
		{
                   if(isDirty(fifoFrame,frontIndexOfQueue))
                   {
		              SM_FileHandle fileHandler;	   
                      openPageFile(bm->pageFile, &fileHandler);
                     // printf("In FIFO page replacement strategy..Found dirty bit, front index %d\n",frontIndexOfQueue);
                      //printf("In FIFO page replacement strategy..Found dirty bit, writing to disk: %s\n",fifoFrame[frontIndexOfQueue].pageContent);
		              writeBlock(fifoFrame[frontIndexOfQueue].pageNum, &fileHandler, fifoFrame[frontIndexOfQueue].pageContent);
                      numberOfWrites++;
                   }
                updatePageFrame(fifoFrame,page,frontIndexOfQueue);
			    break;
		}
            else{
                frontIndexOfQueue++;
                if(frontIndexOfQueue % currentBufferSize == 0)
                {
                    frontIndexOfQueue =0;
                }
            }
        }
   // printf("In FIFO page replacement strategy..Found dirty bit, front index %d\n",frontIndexOfQueue);    
}

//LRU Strategy
extern void LRU_PageReplacementStrategy(BM_BufferPool *const bm, PageFrame *page)
{
    PageFrame *lruFrame = (PageFrame *) bm->mgmtData;
    SM_FileHandle fileHandler;

	int k, indexOfLeastHitPage=0, pageWithLeaseHitNum;

	// Interating through frames
	for(k = 0; k < currentBufferSize; k++)
	{
		///If fix count of a an lru frame is 0.
		if(lruFrame[k].fixCount == 0)
		{
			indexOfLeastHitPage = k;
			pageWithLeaseHitNum = lruFrame[k].hitForLRU;
			break;
		}
	}	

	///Finding the framing with minimum lru hit to replace
	for(k = indexOfLeastHitPage + 1; k < currentBufferSize; k++)
	{
		if(lruFrame[k].hitForLRU < pageWithLeaseHitNum)
		{
			indexOfLeastHitPage = k;
			pageWithLeaseHitNum = lruFrame[k].hitForLRU;
		}
	}

	// If dirty bit is 1 for that page.
	if(isDirty(lruFrame,indexOfLeastHitPage))
	{
		openPageFile(bm->pageFile, &fileHandler);
		writeBlock(lruFrame[indexOfLeastHitPage].pageNum, &fileHandler, lruFrame[indexOfLeastHitPage].pageContent);
		//as we have written block, we increment writes by 1.
		numberOfWrites++;
	}
	
	updatePageFrame(lruFrame,page,indexOfLeastHitPage);

}


// BUFFER POOL FUNCTIONS

//This method is used to create and initialize the buffer pool.
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
    //printf("-----------------InitBufferPool: Initializing buffer pool---------------------\n");
    pthread_mutex_lock(&mutex);  //Acquiring lock
    int k=0;
    int size_bm=0;
	if(bm==NULL) // checking if the buffer manager bm exists or not
		printf("Buffer pool is null");
	if(pageFileName==NULL)
		return RC_FILE_NOT_FOUND;
	//Initialising the buffer manager's variables
    bm->strategy = strategy;
	bm->pageFile = (char *)pageFileName;
    bm->numPages = numPages;
	int size_pageFrame=sizeof(PageFrame);
    //allocating memory required = (size of one page)* (number of pages)
	PageFrame *initialPageFrame = malloc(numPages * size_pageFrame); 

	//Setting values for  buffer pool variables
	size_bm = numPages;	
	numberOfWrites = 0;
    currentBufferSize = numPages;
    maxBufferSize = numPages;
	// Check if all pages fields are equal to NULL 
    for(k = 0; k < size_bm; k++)
	{
        initiateBufferPool(initialPageFrame,k);
    }
	//Page frame is stored in the bufferpool
	bm->mgmtData = initialPageFrame;

	pthread_mutex_unlock(&mutex); //Release lock
    //printf("---------------------InitBufferPool: end-----------------------------------\n");
	
	return RC_OK;		
}


//This method is used to destroy the buffer pool.
RC shutdownBufferPool(BM_BufferPool *const bm)
{
   // printf("--------------ShutdownBufferPool: Start------------------\n");
    pthread_mutex_unlock(&mutex);	
    pthread_mutex_lock(&mutex); //Acquire lock
    int k=0;
	if(bm->mgmtData==NULL)
		printf("BM is null");
	PageFrame *pageFrameToShutDown;
	pageFrameToShutDown = (PageFrame *)bm->mgmtData;
	// All the modified pages are written back to disk
	forceFlushPool(bm);
	while(k < currentBufferSize)
	{
		if(pageFrameToShutDown[k].fixCount != 0)
		{
                   printf("Pinned pages exist");
		}
		k++;
	}
	free(pageFrameToShutDown); // releasing the memory to avoid unneccessary leaks
	bm->mgmtData = NULL;
	pthread_mutex_unlock(&mutex); //Release lock
    //printf("--------------ShutdownBufferPool: End----------------------\n");
	return RC_OK;


}

//This method is used to write all dirty pages from buffer pool to disk.
RC forceFlushPool(BM_BufferPool *const bm)
{
    //printf("--------------Force flush pool: Start-----------------------\n");
	pthread_mutex_unlock(&mutex);
	pthread_mutex_lock(&mutex);//Acquire lock
	
	PageFrame *pageFrameToFlush;
	pageFrameToFlush = (PageFrame *)bm->mgmtData;
	if(bm->mgmtData==NULL)
		printf("BM is invalid");
        int k;
    for(k = 0; k < currentBufferSize; k++)
	{
        if(pageFrameToFlush[k].fixCount==0 && isDirty(pageFrameToFlush,k))
        {
	        SM_FileHandle fileHandler;
            // Writing modified pages to disk
            openPageFile(bm->pageFile, &fileHandler);
            //ensureCapacity(pFrame[k].pageNum, &fhandle);
	         printf("forceFlushPool: Writing modified pages to disk: Data: %s\n",pageFrameToFlush[k].pageContent);	
            writeBlock(pageFrameToFlush[k].pageNum, &fileHandler, pageFrameToFlush[k].pageContent);
            pageFrameToFlush[k].dirtyBit=0;
            numberOfWrites++;
        }
    }
    pthread_mutex_unlock(&mutex); //Release lock
	//printf("--------------Force flush pool: End------------------------\n");
	return RC_OK;
}


// PAGE MANAGEMENT FUNCTIONS

//This method is used to mark a page frame as dirty.
extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    pthread_mutex_unlock(&mutex);	
    pthread_mutex_lock(&mutex);//Acquire lock
    int k=0;
	PageFrame *pageToBeMarkedDirty;
	pageToBeMarkedDirty = (PageFrame *)bm->mgmtData;

    while(k<currentBufferSize)
    {
        if(pageToBeMarkedDirty[k].pageNum == page->pageNum)
        {
            pageToBeMarkedDirty[k].dirtyBit=1;
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
    PageFrame* pageToUnpin = (PageFrame*)bm->mgmtData;
    pthread_mutex_unlock(&mutex); 
	pthread_mutex_lock(&mutex);//Acquire lock
    int k;
    for (k = 0; k < currentBufferSize; k++) //Iterating through the buffer to find the current page.
    {
        if (pageToUnpin[k].pageNum == page->pageNum) //If the given page's pageNum matches with that of current page
        {
            //printf("unpinPage: page found to unpin with page number: %d\n",page->pageNum);
            pageToUnpin[k].fixCount--;
	    break;		
        }
    }
    pthread_mutex_unlock(&mutex); //Release lock
    return RC_OK;
}


//This method is used to pin the page.
extern RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) 
{
    PageFrame *pageFrameToPin;
    pageFrameToPin= (PageFrame *)bm->mgmtData;
    pthread_mutex_unlock(&mutex); 
	pthread_mutex_lock(&mutex);//Acquire lock
    //checking the page to be written is the first page.
    if(pageFrameToPin[0].pageNum == -1)
	{
        SM_FileHandle fileHandler;
        openPageFile(bm->pageFile, &fileHandler); //Method from SM_FileHandle of assignment-1
		pageFrameToPin[0].pageContent = (SM_PageHandle) malloc(PAGE_SIZE); //Method from SM_FileHandle of assignment-1
		ensureCapacity(pageNum,&fileHandler); //Method from SM_FileHandle of assignment-1
		readBlock(pageNum, &fileHandler, pageFrameToPin[0].pageContent); //Method from SM_FileHandle of assignment-1
		pageFrameToPin[0].pageNum = pageNum;
		pageFrameToPin[0].fixCount++;
		numberOfReads = 0; 
		hitCount = 0;	//Setting hitCount to 0 as it was not a hit.
        pageFrameToPin[0].hitForLRU = hitCount;
		page->pageNum = pageNum;
		page->data = pageFrameToPin[0].pageContent;
       // printf("In first if block: page number is: %d\n", page->pageNum); 
       // printf("In first if block: page data is: %s\n", page->data); 
	   // printf("In first if block: page frame data is: %s\n", pageFrameToPin[0].pageContent);    
       // printf("If block exited!\n");
		return RC_OK;		
    }
    else
    {
        bool isCurrentBufferFull = true;
        int k;
        for(k = 0; k < currentBufferSize; k++)
		{
		if(pageFrameToPin[k].pageNum != -1)
		  {
            if(pageFrameToPin[k].pageNum == pageNum) //Meaning, its a hit
			{
                //printf("In Else - if block!\n");
                pageFrameToPin[k].fixCount++;
			    isCurrentBufferFull = false;
                hitCount = hitCount + 1; 
                pageFrameToPin[k].hitForLRU=hitCount;
                page->pageNum = pageNum;
		        page->data = pageFrameToPin[k].pageContent;
               // printf("In Else - if block: page data is: %s\n", page->data); 
                break;
            }
		 }else //Meaning, it is not a hit and page needs to be read from disk
            {
                SM_FileHandle fileHandler;
                //printf("In Else - else block!\n");
				openPageFile(bm->pageFile, &fileHandler);
				pageFrameToPin[k].pageContent = (SM_PageHandle) malloc (PAGE_SIZE);
                //printf("In else - else block: page frame data before readBlock is: %s\n", pageFrameToPin[k].pageContent);
                //printf("In else - else block: page pageNum before readBlock is: %d\n", pageNum);
				readBlock(pageNum, &fileHandler, pageFrameToPin[k].pageContent);
				//printf("In else - else block: page frame data after readBlock is: %s\n", pageFrameToPin[k].pageContent);
				pageFrameToPin[k].pageNum = pageNum;
				pageFrameToPin[k].fixCount = 1;
				hitCount =hitCount + 1;
				numberOfReads++;	
				page->pageNum = pageNum;
				page->data = pageFrameToPin[k].pageContent;
                pageFrameToPin[k].hitForLRU=hitCount;
				isCurrentBufferFull = false;
               // printf("In else - else block: page frame data is: %s\n", pageFrameToPin[k].pageContent);
               // printf("In else - else block: page number is: %d\n", page->pageNum);  
               // printf("In else - else block: page data is: %s\n", page->data); 
				break;
			}

        }

        if(isCurrentBufferFull == true) //means that the current buffer is full 
	{
        printf("In isCurrentBufferFull block!\n");
		int sizeOfFrame = sizeof(PageFrame);
           PageFrame *newPageToBeWritten = (PageFrame *) malloc(sizeOfFrame);		
		   SM_FileHandle fileHandler;
			openPageFile(bm->pageFile, &fileHandler);
			newPageToBeWritten->pageContent = (SM_PageHandle) malloc(PAGE_SIZE);
			readBlock(pageNum, &fileHandler, newPageToBeWritten->pageContent);
			newPageToBeWritten->pageNum = pageNum;
			newPageToBeWritten->dirtyBit = 0;		
			newPageToBeWritten->fixCount = 1;
			numberOfReads++;
			hitCount ++;
            newPageToBeWritten->hitForLRU = hitCount;
			page->pageNum = pageNum;
			page->data = newPageToBeWritten->pageContent;	
           		

			// Call appropriate page replacement alg
		       ReplacementStrategy strategy = bm->strategy;
			switch(strategy)
			{			
				case RS_FIFO: 
					FIFO_PageReplacementStrategy(bm, newPageToBeWritten);
					break;
				
				case RS_LRU: 
					LRU_PageReplacementStrategy(bm, newPageToBeWritten);
					break;
				
				default:
					printf("Strategy's implementation not available!\n");
					break; 

            }
        }

             
    }
    pthread_mutex_unlock(&mutex);  //Release lock
    return RC_OK;
    
}

//This method is used to write the current content on disk.
extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    pthread_mutex_unlock(&mutex);	
    pthread_mutex_lock(&mutex);//Acquire lock
    PageFrame *forcePageFrame = (PageFrame *)bm->mgmtData;
    int k;

    for(k = 0; k < currentBufferSize; k++) //Iterating through the buffer to find the current page.
    {
        if(forcePageFrame[k].pageNum == page->pageNum) //If page is found
        {
            SM_FileHandle fileHandler;
            openPageFile(bm->pageFile, &fileHandler);
            //writeBlock method writes the current page on to the disk. This method was implemented in SM_FileHandle of assignment-1.
            writeBlock(forcePageFrame[k].pageNum, &fileHandler, forcePageFrame[k].pageContent); 
            numberOfWrites++; //increase the writeCount
            forcePageFrame[k].dirtyBit= 0; //Set dirty bit to 0 as page is written back to disk.
            break;
        }
    }
    pthread_mutex_unlock(&mutex); //Release lock
    return RC_OK;
}

// STATISTICS FUNCTIONS

//This method returns an array of frame contents
PageNumber* getFrameContents(BM_BufferPool* const bm)
{
    //printf("getFrameContents: start\n");
    PageFrame* pageFrameToGetContents;
    PageNumber* pageNumbers;
    pageFrameToGetContents = (PageFrame*)bm->mgmtData;
    pageNumbers = malloc(sizeof(PageNumber) * currentBufferSize); //Allocating memory for pageNumbers
    for (int k = 0; k < maxBufferSize; k++) { //Iterating through the buffer.
       if(pageFrameToGetContents[k].pageNum != -1)
       {
        pageNumbers[k] = pageFrameToGetContents[k].pageNum;
       }
       else{
         pageNumbers[k] = NO_PAGE;
       }
    }
    //printf("getFrameContents: end\n");
    return pageNumbers;
}

//This method returns an array of bools if dirty
bool* getDirtyFlags(BM_BufferPool* const bm)
{
    int sizeOfDirtyFrame = sizeof(bool) * currentBufferSize;	
    bool* dirtyFlags = malloc(sizeOfDirtyFrame);
    PageFrame* pageFrameToGetDirtyFlags;
    pageFrameToGetDirtyFlags = (PageFrame*)bm->mgmtData;
    for (int k = 0; k < maxBufferSize; k++) { //Iterating through the buffer.
      if(isDirty(pageFrameToGetDirtyFlags,k))
      {
        dirtyFlags[k] = true; //Finds if dirtyBit is set/not.
      }
      else{
        dirtyFlags[k] = false;
      }
    }
    return dirtyFlags;
    free(dirtyFlags); //will not be executed, so can be removed.
}


//This method returns an array of fixcount
int* getFixCounts(BM_BufferPool* const bm)
{
    int* fixCounts = calloc(currentBufferSize, sizeof(int));
    PageFrame* pageFrameToGetFixCounts;
    pageFrameToGetFixCounts = (PageFrame*)bm->mgmtData;
    for (int k = 0; k < maxBufferSize; k++) //Iterating through the buffer.
    {
        if(pageFrameToGetFixCounts[k].fixCount != -1)
        {
            fixCounts[k] = pageFrameToGetFixCounts[k].fixCount;
        }
        else{
            fixCounts[k]=0;
        }
    }
    return fixCounts;
    free(fixCounts); //will not be executed, so can be removed.
}

//This method returns the readcount
int getNumReadIO(BM_BufferPool* const bm)
{
    return numberOfReads+1; //from the readCount we initialized in the beginning.
}

//This method returns the writecount
int getNumWriteIO(BM_BufferPool* const bm)
{
    return numberOfWrites; //from the writeCount we initialized in the beginning.
}
