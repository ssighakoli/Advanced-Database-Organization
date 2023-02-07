//This file is to include additional test cases.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void additionalTestCases(void);

/* main function running all tests */
int main (void)
{
	testName = "";

	initStorageManager();
	additionalTestCases();

	return 0;
}

//This tests readFirstBlock, readNextBlock, readPreviousBlock, writeBlock, writeCurrentBlock, ensureCapacity methods
void 
additionalTestCases(void)
{
    SM_FileHandle fh;
	SM_PageHandle ph;
    int i;

    ph = (SM_PageHandle) malloc(PAGE_SIZE);

    testName = "Testing additional methods";

    // create a new page file
    TEST_CHECK(createPageFile (TESTPF));
    TEST_CHECK(openPageFile (TESTPF, &fh));
    printf("created and opened file\n");

    // read first page into handle
    TEST_CHECK(readFirstBlock (&fh, ph));

    // the page should be empty (zero bytes)
    for (i=0; i < PAGE_SIZE; i++){
        ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
    }
    printf("First page is empty!\n");

    // change ph to be a string and write that one to disk
    for (i=0; i < PAGE_SIZE; i++){
    ph[i] = (i % 10) + '0';
    }
    TEST_CHECK(writeBlock (0, &fh, ph));
    printf("Writing block-1..\n");
    
    //Testing write current block method
    TEST_CHECK(writeCurrentBlock(&fh,ph));
    printf("Writing current block-2..\n");

    //Testing write current block method again
    TEST_CHECK(writeCurrentBlock(&fh,ph));
    printf("Writing current block-3..\n");


    //Testing read block method
    TEST_CHECK(readBlock(0,&fh,ph));
    for (i=0; i < PAGE_SIZE; i++){
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
    }
    printf("Reading first block..\n");

    //Testing read next block method
    TEST_CHECK(writeCurrentBlock(&fh,ph));
    for (i=0; i < PAGE_SIZE; i++){
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
    }
    printf("Reading next block..\n");

    //Testing read previous block method
    TEST_CHECK(readPreviousBlock(&fh,ph));
    for (i=0; i < PAGE_SIZE; i++){
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
    }
    printf("Reading previous block..\n");

    for(i=0;i<PAGE_SIZE;i++){
    ph[i] = (i % 10) + '1';
    }

    TEST_CHECK(writeCurrentBlock(&fh,ph));
    printf("Writing current block-4..\n");

    //Testing read current block method
    TEST_CHECK(readCurrentBlock(&fh,ph));
    for (i=0; i < PAGE_SIZE; i++){
    ASSERT_TRUE((ph[i] == (i % 10) + '1'), "character in page read from disk is the one we expected.");
    }
    printf("Reading current block.\n");

    //Testing ensure capacity method
    TEST_CHECK(ensureCapacity(8,&fh));
    printf("Capacity ensured..\n");

    // destroy new page file
    TEST_CHECK(closePageFile (&fh));
    printf("Pagefile Closed..\n");
    
    TEST_CHECK(destroyPageFile (TESTPF));  
    printf("Pagefile Destroyed..\n");

    //free page memory
    free(ph);
  
    TEST_DONE();
    printf("TEST 2 RUN SUCCESSFULL\n");

}

