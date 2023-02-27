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
//This index stores page count which are read from disk
int count = 0;
int cPointer = 0;
int lfu=0;
int hit=0;

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
    int hitNum;
    int refNum;
} PageFrame;

//Replacement strategies


//first in first out
extern void FIFO(BM_BufferPool* const bm, PageFrame* page)
{
    pthread_mutex_lock(&mutex);//lock initiated

    PageFrame* pageFrame = (PageFrame*)bm->mgmtData;

    int frameIndex = count % maxBufferSize;

    // Check if the page frame is in use
    if (pageFrame[frameIndex].fixCount > 0) {
        // Find the next free page frame
        do {
            frameIndex++;
            frameIndex %= maxBufferSize;
        } while (pageFrame[frameIndex].fixCount > 0);
    }

    // If page in memory modified (dirtyBit = 1), then write page to disk
    if (pageFrame[frameIndex].dirtyBit == 1)
    {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[frameIndex].pageNum, &fh, pageFrame[frameIndex].data);

        // Increase the writeCount which records the number of writes done by the buffer manager.
        writeCount++;
    }

    // updation of pageframes to new content
    pageFrame[frameIndex].data = page->data;
    pageFrame[frameIndex].pageNum = page->pageNum;
    pageFrame[frameIndex].dirtyBit = page->dirtyBit;
    pageFrame[frameIndex].fixCount = page->fixCount;

    // Move the count further
    count++;

    pthread_mutex_unlock(&mutex); //release lock
}

//  Least Recently Used strategy
extern void LRU(BM_BufferPool* const bm, PageFrame* page)
{
    pthread_mutex_lock(&mutex);

    PageFrame* pageFrame = (PageFrame*)bm->mgmtData;
    int leastRecIndex = 0;
    int i;

    // locate the pageframe with least hit count
    for (int i = 1; i < maxBufferSize; i++)
    {
        if (pageFrame[i].hitNum < pageFrame[leastRecIndex].hitNum)
        {
            leastRecIndex = i;
        }
    }

    // If page in memory has been modified (dirtyBit = 1), then write page to disk
    if (pageFrame[leastRecIndex].dirtyBit == 1)
    {
        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[leastRecIndex].pageNum, &fh, pageFrame[leastRecIndex].data);

        // Increase the writeCount which records the number of writes done by the buffer manager.
        writeCount++;
    }

    // Setting page frame's content to new page's content
    pageFrame[leastRecIndex].data = page->data;
    pageFrame[leastRecIndex].pageNum = page->pageNum;
    pageFrame[leastRecIndex].dirtyBit = page->dirtyBit;
    pageFrame[leastRecIndex].fixCount = page->fixCount;
    pageFrame[leastRecIndex].hitNum = 0;

    // Increment hit count for all other page frames
    for (int i = 0; i < maxBufferSize; i++)
    {
        if (i != leastRecIndex)
        {
            pageFrame[i].hitNum++;
        }
    }

    pthread_mutex_unlock(&mutex);// release lock
}

// Function to find the least frequently used page frame
int locateLeastFrequentlyUsedPageFrame(PageFrame* pageFrame, int firstIndex) {
    int leastFreIndex = firstIndex;
    int leastFreUsedFrame = pageFrame[leastFreIndex].refNum;

    for (int i = 0; i < maxBufferSize; i++) {
        if (pageFrame[firstIndex].refNum < leastFreUsedFrame && pageFrame[firstIndex].fixCount == 0) {
            leastFreIndex = firstIndex;
            leastFreUsedFrame = pageFrame[firstIndex].refNum;
        }
        firstIndex = (firstIndex + 1) % maxBufferSize;
    }

    return leastFreIndex;
    pthread_mutex_unlock(&mutex);//release lock
}

// Least Frequently Used
extern void LFU(BM_BufferPool* const bm, PageFrame* page)
{

    pthread_mutex_lock(&mutex);
    SM_FileHandle fh;

    PageFrame* pageFrame = (PageFrame*)bm->mgmtData;

    int leastFreIndex = lfu;
    leastFreIndex = (leastFreIndex + 1) % maxBufferSize;

    // locate the pageframe which is least frequently used
    leastFreIndex = locateLeastFrequentlyUsedPageFrame(pageFrame, leastFreIndex);

    // If page in memory has been modified (dirtyBit == 1), then write page to disk
    if (pageFrame[leastFreIndex].dirtyBit == 1) {
        openPageFile(bm->pageFile, &fh);
        writeBlock(pageFrame[leastFreIndex].pageNum, &fh, pageFrame[leastFreIndex].data);
        writeCount++;
    }

    // Setting page frame's content to new page's content
    pageFrame[leastFreIndex].data = page->data;
    pageFrame[leastFreIndex].pageNum = page->pageNum;
    pageFrame[leastFreIndex].dirtyBit = page->dirtyBit;
    pageFrame[leastFreIndex].fixCount = page->fixCount;
    lfu = leastFreIndex + 1;
}



//  CLOCK 
extern void CLOCK(BM_BufferPool* const bm, PageFrame* page)
{
    pthread_mutex_lock(&mutex);
    PageFrame* pageFrame = (PageFrame*)bm->mgmtData;
    int i;

    for (int i = 0; i < maxBufferSize; i++)
    {
        cPointer = (cPointer % maxBufferSize == 0) ? 0 : cPointer;

        if (pageFrame[cPointer].hitNum == 0)
        {
            // If page in memory has been modified (dirtyBit = 1), then write page to disk
            if (pageFrame[cPointer].dirtyBit == 1)
            {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                writeBlock(pageFrame[cPointer].pageNum, &fh, pageFrame[cPointer].data);
                writeCount++;
            }

            // updating page frame's content to newly added page content
            pageFrame[cPointer].data = page->data;
            pageFrame[cPointer].pageNum = page->pageNum;
            pageFrame[cPointer].dirtyBit = page->dirtyBit;
            pageFrame[cPointer].fixCount = page->fixCount;
            pageFrame[cPointer].hitNum = page->hitNum;
            cPointer++;
            break;
        }
        else
        {
            //locate further frames
            pageFrame[cPointer++].hitNum = 0;
        }
    }
    pthread_mutex_unlock(&mutex);
}



// BUFFER POOL FUNCTIONS

//This method is used to create and initialize the buffer pool.
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{

    printf(" initBufferPool: initializing buffer pool");
    int k=0;
    int size_bm=0;
    int clockVar=0;
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
	PageFrame *pg = malloc(numPages*size_pageFrame ); 

	//Setting values for  buffer pool variables
	size_bm = numPages;	
	writeCount = 0;
	clockVar = 0;
	// Check if all pages fields are equal to NULL 
    while(k<size_bm)
    {
        pg[k].data=NULL;
        pg[k].dirtyBit=0;
        pg[k].fixCount = 0;
        pg[k].pinStatus=0;
        pg[k].pageNum=-1;
        pg[k].freeStat=0;
        k=k+1;
    }
	//Page frame is stored in the bufferpool
	bm->mgmtData = pg;
     printf(" initBufferPool: end");
	return RC_OK;
		
}

//This method is used to destroy the buffer pool.
RC shutdownBufferPool(BM_BufferPool *const bm)
{
     printf("shutdownBufferPool: Start");
    int k=0;
	if(bm->mgmtData==NULL)
		printf("BM is null");
	PageFrame *pFrame;
	pFrame = (PageFrame *)bm->mgmtData;
	// All the modified pages are written back to disk
	forceFlushPool(bm);

	while(k < bm->numPages)
	{
		if(pFrame[k].fixCount != 0)
		{
           // return RC_PINNED_PAGES_IN_BUFFER;
           printf("Pinned pages exist");
		}
		else
		{
            printf("Fix Count is now equal to zero \n");	
		}
		k++;
	}
	forceFlushPool(bm);
	free(pFrame); // releasing the memory to avoid unneccessary leaks
	bm->mgmtData = NULL;
    printf("shutdownBufferPool: end");
	return RC_OK;


}

//This method is used to write all dirty pages from buffer pool to disk.
RC forceFlushPool(BM_BufferPool *const bm)
{
    printf("shutdownBufferPool: Start");

	SM_FileHandle fhandle;
	PageFrame *pFrame;
	pFrame = (PageFrame *)bm->mgmtData;
	int i;
	if(bm->mgmtData==NULL)
		printf("BM is invalid");
    int k=0;
    while (k<bm->numPages)
    {
        if(pFrame[k].fixCount==0 && pFrame[k].dirtyBit==1  )
        {
            // Writing modified pages to disk
            openPageFile(bm->pageFile, &fhandle);
            ensureCapacity(pFrame[k].pageNum, &fhandle);
            writeBlock(pFrame[k].pageNum, &fhandle, pFrame[k].data);
            pFrame[k].dirtyBit=0;
            writeCount++;

        }
        else{
            printf("");
        }
        k++;
    }
    
	printf("shutdownBufferPool: End");
	return RC_OK;


}


// PAGE MANAGEMENT FUNCTIONS

//This method is used to mark a page frame as dirty.
extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    printf("markDirty: Start");
    int k=0;
	PageFrame *pFrame;
	pFrame = (PageFrame *)bm->mgmtData;

    while(k<bm->numPages)
    {
        if(pFrame[k].pageNum == page->pageNum)
        {
            pFrame[k].dirtyBit=TRUE;
            return RC_OK;
        }
        k++;

    }

    //return RC_BM_PAGE_FRAME_NOT_FOUND;
	
    printf("BM page frame not found\n");	
    
    printf("markDirty: End");
    return RC_OK;


}

//This method is used to unpin the page.
extern RC unpinPage(BM_BufferPool* const bm, BM_PageHandle* const page)
{
      printf("unpinPage: start");
    PageFrame* pageFrame = (PageFrame*)bm->mgmtData;
    pthread_mutex_lock(&mutex);//Acquire lock
    int i;
    for (i = 0; i < currentBufferSize; i++) //Iterating through the buffer to find the current page.
    {
        if (pageFrame[i].pageNum == page->pageNum) //If the given page's pageNum matches with that of current page
        {
            pageFrame[i].pinStatus = 0; //Set pinStatus to 0.
            if (pageFrame[i].fixCount > 0) {
                pageFrame[i].fixCount--; //Decrease the fixCount
            }
            else {
                pageFrame[i].fixCount = 0; //Set fixCount to 0.
            }
            pageFrame[1].pinStatus = 0; //Set pinStatus to 0.
            break;
        }
    }

    pthread_mutex_unlock(&mutex); //Release lock
    printf("unpinPage: end");
    return RC_OK;
}


//This method is used to pin the page with pagenumber pageNum.
extern RC pinPage(BM_BufferPool* const bm, BM_PageHandle* const page, const PageNumber pageNum)
{
      printf("pinPage: start");
    PageFrame* pageFrame = (PageFrame*)bm->mgmtData;

    // identifying if buffer pool is empty
    if (pageFrame[0].pageNum == -1)
    {

        SM_FileHandle fh;
        openPageFile(bm->pageFile, &fh);
        ensureCapacity(pageNum, &fh);
        pageFrame[0].data = (SM_PageHandle)malloc(PAGE_SIZE);
        readBlock(pageNum, &fh, pageFrame[0].data);
        pageFrame[0].pageNum = pageNum;
        pageFrame[0].fixCount++;
        count = 0;
        hit = 0;
        pageFrame[0].hitNum = hit;
        pageFrame[0].refNum = 0;
        page->pageNum = pageNum;
        page->data = pageFrame[0].data;

        return RC_OK;
    }
    else
    {
        int i;
        bool isBufferFull = true;

        for (i = 0; i < maxBufferSize; i++)
        {
            if (pageFrame[i].pageNum != -1)
            {
                // Checking the range of page if its in memory
                if (pageFrame[i].pageNum == pageNum)
                {

                    pageFrame[i].fixCount++;
                    isBufferFull = false;
                    hit++;

                    if (bm->strategy == RS_LRU)
                        // The Value of hit is used from the function LRU identify the least recent used page
                        pageFrame[i].hitNum = hit;
                    else if (bm->strategy == RS_CLOCK)
                        pageFrame[i].hitNum = 1;
                    else if (bm->strategy == RS_LFU)
                        pageFrame[i].refNum++;

                    page->pageNum = pageNum;
                    page->data = pageFrame[i].data;

                    cPointer++;
                    break;
                }
            }
            else {
                SM_FileHandle fh;
                openPageFile(bm->pageFile, &fh);
                pageFrame[i].data = (SM_PageHandle)malloc(PAGE_SIZE);
                readBlock(pageNum, &fh, pageFrame[i].data);
                pageFrame[i].pageNum = pageNum;
                pageFrame[i].fixCount = 1;
                pageFrame[i].refNum = 0;
                count++;
                hit++;

                if (bm->strategy == RS_LRU)
                    pageFrame[i].hitNum = hit;
                else if (bm->strategy == RS_CLOCK)
                    pageFrame[i].hitNum = 1;

                page->pageNum = pageNum;
                page->data = pageFrame[i].data;
                isBufferFull = false;
                break;
            }
        }
        if (isBufferFull == true)
        {

            PageFrame* updatedPage = (PageFrame*)malloc(sizeof(PageFrame));
            SM_FileHandle fh;
            openPageFile(bm->pageFile, &fh);
            updatedPage->data = (SM_PageHandle)malloc(PAGE_SIZE);
            readBlock(pageNum, &fh, updatedPage->data);
            updatedPage->pageNum = pageNum;
            updatedPage->dirtyBit = 0;
            updatedPage->fixCount = 1;
            updatedPage->refNum = 0;
            count++;
            hit++;

            if (bm->strategy == RS_LRU)

                updatedPage->hitNum = hit;
            else if (bm->strategy == RS_CLOCK)

                updatedPage->hitNum = 1;

            page->pageNum = pageNum;
            page->data = updatedPage->data;

            switch (bm->strategy)
            {
            case RS_FIFO: // Using FIFO strategy
                FIFO(bm, updatedPage);
                break;

            case RS_LRU: // Using LRU strategy
                LRU(bm, updatedPage);
                break;

            case RS_CLOCK: // Using CLOCK strategy
                CLOCK(bm, updatedPage);
                break;

            case RS_LFU: // Using LFU strategy
                LFU(bm, updatedPage);
                break;

            default:
                printf("\nNo replacement strategy is required or implemented");
                break;
            }

        }
         printf("pinPage: end");
        return RC_OK;
    }
}



//This method is used to write the current content of the page back to the page file on disk.
extern RC forcePage(BM_BufferPool* const bm, BM_PageHandle* const page)
{
     printf("forcePage: start");
    pthread_mutex_lock(&mutex); //Acquire lock
    PageFrame* pageFrame = (PageFrame*)bm->mgmtData;
    int i;

    for (i = 0; i < currentBufferSize; i++) //Iterating through the buffer to find the current page.
    {
        if (pageFrame[i].pageNum == page->pageNum) //If page is found
        {
            SM_FileHandle fileHandler;
            openPageFile(bm->pageFile, &fileHandler);
            //writeBlock method writes the current page on to the disk. This method was implemented in SM_FileHandle of assignment-1.
            writeBlock(pageFrame[i].pageNum, &fileHandler, pageFrame[i].data);
            writeCount++; //increase the writeCount
            pageFrame[i].dirtyBit = 0; //Set dirty bit to 0 as page is written back to disk.
            break;
        }
    }
    pthread_mutex_unlock(&mutex); //Release lock
    printf("forcePage: end");
    return RC_OK;
}


// STATISTICS FUNCTIONS

//This method returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame. 
//An empty page frame is represented using the constant NO_PAGE.
PageNumber* getFrameContents(BM_BufferPool* const bm)
{
    printf("getFrameContents: start");
    PageFrame* pageFrame;
    PageNumber* pageNumbers;
    pthread_mutex_lock(&mutex); // acquire lock
    pageFrame = (PageFrame*)bm->mgmtData;
    pageNumbers = malloc(sizeof(PageNumber) * currentBufferSize); //Allocating memory for pageNumbers
    for (int i = 0; i < maxBufferSize; i++) { //Iterating through the buffer.
        pageNumbers[i] = (pageFrame[i].pageNum != -1) ? pageFrame[i].pageNum : NO_PAGE;
    }
    pthread_mutex_unlock(&mutex); // release lock
    printf("getFrameContents: end");
    return pageNumbers;
}

//This method returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty. 
bool* getDirtyFlags(BM_BufferPool* const bm)
{
    printf("getDirtyFlags: start");
    bool* dirtyFlags = malloc(sizeof(bool) * currentBufferSize);
    PageFrame* pageFrame;
    pthread_mutex_lock(&mutex); // acquire lock
    pageFrame = (PageFrame*)bm->mgmtData;
    for (int i = 0; i < maxBufferSize; i++) { //Iterating through the buffer.
        dirtyFlags[i] = (pageFrame[i].dirtyBit == 1) ? true : false; //Finds if dirtyBit is set/not.
    }
    pthread_mutex_unlock(&mutex); // release lock
    printf("getDirtyFlags: end");
    return dirtyFlags;
    free(dirtyFlags); //will not be executed, so can be removed.
}


//This method returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame. 
//Returns 0 for empty page frames.
int* getFixCounts(BM_BufferPool* const bm)
{
    printf("getFixCounts: start");
    int* fixCounts = calloc(currentBufferSize, sizeof(int));
    PageFrame* pageFrame;
    pthread_mutex_lock(&mutex);  // acquire lock
    pageFrame = (PageFrame*)bm->mgmtData;
    for (int i = 0; i < maxBufferSize; i++) //Iterating through the buffer.
    {
        fixCounts[i] = (pageFrame[i].fixCount != -1) ? pageFrame[i].fixCount : 0; //finds if fixedCount is set/not.
    }

    pthread_mutex_unlock(&mutex);  // release lock
    printf("getFixCounts: end");
    return fixCounts;
    free(fixCounts); //will not be executed, so can be removed.
}

//This method returns the number of pages that have been read from disk since a buffer pool has been initialized.
int getNumReadIO(BM_BufferPool* const bm)
{
    printf("getNumReadIO: start");
    return readCount; //from the readCount we initialized in the beginning.
}

//This method returns the number of pages written from disk since a buffer pool has been initialized.
int getNumWriteIO(BM_BufferPool* const bm)
{
    printf("getNumWriteIO: start");
    return writeCount; //from the writeCount we initialized in the beginning.
}
