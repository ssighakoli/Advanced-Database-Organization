#include "dberror.h"
#include "btree_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "tables.h"
#include "btree_insertionHelper.h"
#include "btree_deletionHelper.h"
#include "math.h"


BTreeMaster * btreeMaster = NULL;

const int maxOrder = PAGE_SIZE / sizeof(BTreeMaster);

//This method is used to initialize the indexmanager
RC initIndexManager(void *mgmtData) {
	FILE *pointerToTree;
	pointerToTree = NULL;
	if(pointerToTree != NULL)
	{
	   printf("initIndexManager(): BTree Master did not initialize properly!\n");
	   return RC_ERROR;
	}
	else
	{
		printf("initIndexManager(): BTree Master initialized properly!\n");
	}
	return RC_OK;
}

//This method is used to shutdown the indexmanager
RC shutdownIndexManager() {
	btreeMaster = NULL;
	free(btreeMaster);

	if(btreeMaster == NULL)
	{   
	   printf("shutdownRecordManager(): Shutdown success\n");
	}
	else
	{
		printf("shutdownRecordManager(): BTree Master did not shutdown properly\n");
		return RC_ERROR;
	}
	return RC_OK;
}

//This helper function is used to store the information onto storage handler
extern RC storeInformationToStorageHandle(char *name, char data[])
{
	SM_FileHandle fHandle;
	int RC_State;
    //The below functions are being called from storage manager based on the page file state
	if((RC_State = createPageFile(name)) == RC_FILE_NOT_FOUND)//creates page file
	{
		return RC_State;
	}
	if((RC_State = openPageFile(name, &fHandle)) != RC_OK)//Opens page file
	{
		return RC_State;
	}	
	if((RC_State = writeBlock(0, &fHandle, data)) == RC_WRITE_FAILED)//writes pagefile
	{
		return RC_State;
	}
	if((RC_State = closePageFile(&fHandle)) != RC_OK)//closes page file
	{
		return RC_State;
	}
   return RC_OK;
}

//This method is used to create a btree
RC createBtree(char *idxId, DataType keyType, int n) {
	int returnCode;
	printf("CREATE B TREE STARTS..\n");

	if(n >= maxOrder)
    {
        printf("createBtree(): Order too high!Cannot create B tree.\n");
        return RC_ERROR;
    }

	if(keyType == NULL)
    {
        printf("createBtree(): Required input parameters are NULL.\n");
    }

	btreeMaster = (BTreeMaster *) malloc(sizeof(BTreeMaster));

	
    if (!btreeMaster)
    {
        printf("createBtree(): Memory allocation for master node failed\n");
        return RC_ERROR;
    }


	btreeMaster->treeOrder = n + 2;		
	btreeMaster->totalNodes = 0;		
	btreeMaster->totalEntries = 0;	
	btreeMaster->baseNode = NULL;		
	
	btreeMaster->keyType = keyType;	

	BM_BufferPool * bm = (BM_BufferPool *) malloc(sizeof(BM_BufferPool));
	btreeMaster->bufferPool = *bm;

	char data[PAGE_SIZE];

	returnCode = storeInformationToStorageHandle(idxId,data);

    if(returnCode != RC_OK)
    {
        //free(btreeMaster);
        return RC_ERROR;
    }
    else
    {
        printf("createBtree(): Information stored to Storage manager successfully\n");
    }

	printf("\n createBtree SUCCESS\n");
	return (RC_OK);
}

//This method is used to open a btree
RC openBtree(BTreeHandle **tree, char *idxId) {

    int returnCode;

	// Check if the input parameters are valid
    if (idxId == NULL || tree == NULL)
    {
        printf("openBtree(): File name/Tree is NULL\n");
        return RC_NULL_ARGUMENT;
    }

	*tree = (BTreeHandle *) calloc(1, sizeof(BTreeHandle));
	(*tree)->mgmtData = btreeMaster;
	

	returnCode = initBufferPool(&btreeMaster->bufferPool, idxId, 1000, RS_FIFO, NULL);

	if (returnCode != RC_OK)
	{
		printf("\n Buffer pool not initialized successfully\n");
		//free(btreeMaster);
		return returnCode;
	}

	printf("\n Buffer pool initialized successfully\n");
	
	return RC_OK;
}

//This method is used to close a btree
RC closeBtree(BTreeHandle *tree) {

    // Check if the input parameters are valid
    if (tree == NULL)
    {
        return RC_NULL_ARGUMENT;
    }

	BTreeMaster * btreeMaster = (BTreeMaster*) tree->mgmtData;

	RC returnCode = shutdownBufferPool(&btreeMaster->bufferPool);
    if (returnCode != RC_OK)
    {
        printf("closeBtree(): Failed to close page file for B-tree\n");
        return RC_ERROR;
    }
    else
    {
        printf("closeBtree(): BTree closed successfully!\n");
        //free(btreeMaster);
	 //free(tree);
    }
	return RC_OK;
}

//This method is used to delete a btree
RC deleteBtree (char *idxId)
{
    // Check if the input parameters are valid
    if (idxId == NULL)
    {
        printf("deleteBtree(): B Tree File name is NULL\n");
        return RC_ERROR;
    }

    FILE *treeToDelete;
	treeToDelete = fopen(idxId, "r");
	if(treeToDelete == NULL)
	{
        printf("deleteBtree(): File not found!\n");
		return RC_FILE_NOT_FOUND;
	}
	remove(idxId);
    printf("deleteBtree(): Delete Successfull!\n");
	return RC_OK;
}

//This method is used to print key information
void printinformationArray(int arr[][3], int numKeys, int numCols) {
	printf("Printing Information array...........\n");
    for (int i = 0; i < numKeys; i++) {
        for (int j = 0; j < numCols; j++) {
            printf("%d ", arr[i][j]);
        }
    }
}

//This method is used to cretae a new record to store page and slot values
RecordInfo * makeRecord(RID * recordId) {
	RecordInfo * record = (RecordInfo *) malloc(sizeof(RecordInfo));
	if (record == NULL) {
		perror("RecordInfo creation.");
		exit(RC_ERROR);
	} else {
		record->recordId.page = recordId->page;
		record->recordId.slot = recordId->slot;
	}
	printf("\n Created new node record for rid.page = %d and rid.slot = %d\n", record->recordId.page, record->recordId.slot);
	return record;
}

//This method is used to insert into leaf node
Node * pushToLeaf(BTreeMaster * treeManager, Node * leaf, Value * key, RecordInfo * pointer) {


	int i, insertAt;
	treeManager->totalEntries++;

	insertAt = 0;

	while (insertAt < leaf->keyCount )
	{
		insertAt++;
	}

	for (i = leaf->keyCount; i > insertAt; i--)
	{
		leaf->keyPointers[i] = leaf->keyPointers[i - 1];
		leaf->nodePointers[i] = leaf->nodePointers[i - 1];
	}

	leaf->keyPointers[insertAt] = key->v.intV;

	leaf->nodePointers[insertAt] = pointer;
	leaf->keyCount++;
	return leaf;
}

//This helper is used to store information to the info array
void insertIntoInformationArray(BTreeMaster *btreeMaster, Value *key, int page, int slot,int numEntries) {
    btreeMaster->informationArray[numEntries][0] = key->v.intV;
    btreeMaster->informationArray[numEntries][1] = page;
    btreeMaster->informationArray[numEntries][2] = slot;
}

//This helper is used to check if the leaf node is full
bool checkIfLeafFull(BTreeMaster *btreeMaster, Node *leaf) {
    int bTreeOrder = btreeMaster->treeOrder;
    return leaf->keyCount == bTreeOrder - 1;
}

//This helper is used to insert into leaf directly or split if required
Node* insertIntoLeaf(BTreeMaster *btreeMaster, Node *leaf, Value *key, RecordInfo *recordPointer, int page, int slot, int numEntries) {
    int bTreeOrder = btreeMaster->treeOrder;
    bool isLeafFull = (leaf->keyCount >= bTreeOrder - 1);
    printf("insertKey(): Space %s in the leaf, splitting %s.\n", isLeafFull ? "not available" : "available", isLeafFull ? "needed" : "not needed");
    btreeMaster->baseNode = isLeafFull ? divideLeafAndInsert(btreeMaster, leaf, key, recordPointer) : pushToLeaf(btreeMaster, leaf, key, recordPointer);
    insertIntoInformationArray(btreeMaster, key, page, slot, numEntries);
    return leaf;
}

//This method is used to insert a key into the btree
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {

	BTreeMaster *btreeMaster = (BTreeMaster *) tree->mgmtData;
	RecordInfo * recordPointer;
	Node * leaf;

	int bTreeOrder = btreeMaster->treeOrder;
	int page = rid.page;
	int slot = rid.slot;
	int numEntries = btreeMaster->totalEntries;

	recordPointer = makeRecord(&rid);

	if (btreeMaster->baseNode == NULL) {
		btreeMaster->baseNode = constructBTree(btreeMaster, key, recordPointer);
		insertIntoInformationArray(btreeMaster,key,page, slot, 0);
		return RC_OK;
	}

	leaf = searchForLeaf(btreeMaster->baseNode, key);
	leaf = insertIntoLeaf(btreeMaster, leaf, key, recordPointer, page, slot, numEntries);

    //printTree(tree);

	return RC_OK;
}

//This helper is used to find key from the info array
RecordInfo * findKeyInArray(int informationArray[][3], int numKeys, int keyToFind) {
	RecordInfo * record = (RecordInfo *) malloc(sizeof(RecordInfo));
    for (int k = 0; k < numKeys; k++) {
        if ((int)informationArray[k][0] == keyToFind) {
			record->recordId.page = informationArray[k][1];
		    record->recordId.slot = informationArray[k][2];
            return record;  
        }
    }
    return NULL; 
}

//This method is used to find a key in the given tree and result rid pointer corresponding to that key
extern RC findKey(BTreeHandle *tree, Value *key, RID *result) {

	BTreeMaster *btreeMaster = (BTreeMaster *) tree->mgmtData;
	int numEntries = btreeMaster->totalEntries;

	int (*informationArray)[3] = btreeMaster->informationArray;
	
	int keyToFind = key->v.intV;

	RecordInfo * r = findKeyInArray(informationArray,numEntries+1, keyToFind);
	
	if (r != NULL) {
		*result = r->recordId;
	     return RC_OK;
	}
	else
	{
		return RC_IM_KEY_NOT_FOUND;
	}
	
}

//This method is used to return the number of nodes present in the given btree
RC getNumNodes(BTreeHandle *tree, int *result) {

    if (tree == NULL || result == NULL)
    {
        printf("getNumNodes(): Cannot fetch number of nodes from BTreeMaster.\n");
        return RC_NULL_ARGUMENT;
    }

	BTreeMaster * btreeMaster = (BTreeMaster *) tree->mgmtData;

	if (btreeMaster == NULL)
    {
        printf("getNumNodes(): Cannot fetch number of nodes from BTreeMaster.\n");
        return RC_NULL_ARGUMENT;
    }

	int numberOfNodessInBtree = btreeMaster->totalNodes;

	*result = numberOfNodessInBtree;
	return RC_OK;
}

//This method is used to return the number of entries present in the given btree
RC getNumEntries(BTreeHandle *tree, int *result) {

    if (tree == NULL || result == NULL)
    {
        printf("getNumEntries(): Cannot fetch number of nodes from BTreeMaster.\n");
        return RC_NULL_ARGUMENT;
    }

	BTreeMaster * btreeMaster = (BTreeMaster *) tree->mgmtData;

	if (btreeMaster == NULL)
    {
        printf("getNumEntries(): Cannot fetch number of nodes from BTreeMaster.\n");
        return RC_NULL_ARGUMENT;
    }

	int numberOfEntriesInBtree = btreeMaster->totalEntries;

	*result = numberOfEntriesInBtree;
	return RC_OK;
}


//This method is used to return the datatype of the given btree
RC getKeyType(BTreeHandle *tree, DataType *result) {

    if (tree == NULL || result == NULL)
    {
        printf("getKeyType(): Cannot fetch keytype from BTreeMaster.\n");
        return RC_NULL_ARGUMENT;
    }

	BTreeMaster * btreeMaster = (BTreeMaster *) tree->mgmtData;

	if (btreeMaster == NULL)
    {
        printf("getNumNodes(): Cannot fetch keytype from BTreeMaster.\n");
        return RC_NULL_ARGUMENT;
    }

	DataType dataTypeOfKeys = btreeMaster->keyType;

	*result = dataTypeOfKeys;
	return RC_OK;
}

//This helper is used to delete a row
void deleteRow(int informationArray[][3], int numRows, int numCols, int keyToDelete) {
    int rowIndex = -1; // -1 means key not found
    
    // find the row with the key
    for (int i = 0; i < numRows; i++) {
        if (informationArray[i][0] == keyToDelete) {
            rowIndex = i;
            break;
        }
    }
    
    // if key not found, return
    if (rowIndex == -1) {
        return;
    }
    
    // shift all rows above the deleted row down by one
    for (int i = rowIndex; i < numRows - 1; i++) {
        for (int j = 0; j < numCols; j++) {
            informationArray[i][j] = informationArray[i + 1][j];
        }
    }
    
    // reduce the number of rows in the array by one
    numRows--;
    
    // clear the last row (which is now a duplicate of the second-last row)
    for (int j = 0; j < numCols; j++) {
        informationArray[numRows][j] = 0;
    }
}


//This method is used to delete a key from the tree
RC deleteKey(BTreeHandle *tree, Value *key) {

	BTreeMaster *btreeMaster = (BTreeMaster *) tree->mgmtData;
	int (*informationArray)[3] = btreeMaster->informationArray;
	int totalEntries = btreeMaster->totalEntries;
	int keyToDelete = key->v.intV;
	
	btreeMaster->baseNode = discardKey(btreeMaster, key);

	deleteRow(informationArray,totalEntries,3,keyToDelete);
	btreeMaster->totalEntries = totalEntries-1;

	return RC_OK;
}

//This method is used to initialize the btree scanner
BTreeScanner* initBTreeScanner(Node* node, int totalEntries, int order) {
    BTreeScanner* scanner = malloc(sizeof(BTreeScanner));
    scanner->node = node;
    scanner->searchKey = 0;
    return scanner;
}


//This method is used to open the btree scanner
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
	// Retrieve B+ Tree's metadata information.
	BTreeMaster *btreeMaster = (BTreeMaster *) tree->mgmtData;

	// Retrieve B+ Tree Scan's metadata information.
  	BTreeScanner *scannedData = malloc(sizeof(BTreeScanner));

	// Allocating some memory space.
	*handle = malloc(sizeof(BT_ScanHandle));

	Node * node = btreeMaster->baseNode;

	if (btreeMaster->baseNode == NULL) {
		return RC_NO_RECORDS_TO_SCAN;
	} else {
		while (!node->isLeafNode)
			node = node->nodePointers[0];

		scannedData->node = node;
		scannedData->btreeMaster = *btreeMaster;
		scannedData->searchKey = 0;
		(*handle)->mgmtData = scannedData;
		
	}
	return RC_OK;
}

//This method is used to return the lowest first index
int getLowestFirstIndex(int informationArray[][3], int rows, int searchKey) {
    // Retrieve the lowest first index from the informationArray.
    int lowestIndex = informationArray[searchKey][0];
    // Return the lowest index.
    return lowestIndex;
}

//This method is used to sort the info list
void sortInformationList(int informationArray[][3], int rows) {
    // Loop through all rows except the last one.
    for (int i = 0; i < rows-1; i++) {
        // Loop through the remaining unsorted rows.
        for (int j = 0; j < rows-i-1; j++) {
            // Check if the current row's first element is greater than the next row's first element.
            if (informationArray[j][0] > informationArray[j+1][0]) {
                // Swap the rows.
                int temp[3];
                memcpy(temp, informationArray[j], sizeof(int)*3);
                memcpy(informationArray[j], informationArray[j+1], sizeof(int)*3);
                memcpy(informationArray[j+1], temp, sizeof(int)*3);
            }
        }
    }
}


//This method is used to find the next entry from the scan and return corresponding record page and slot
RC nextEntry(BT_ScanHandle *handle, RID *result) {


    BTreeScanner *scannedData = (BTreeScanner *) handle->mgmtData;
    BTreeMaster *btreeMaster = &scannedData->btreeMaster;
    int searchKey = scannedData->searchKey;

    // Get the number of entries and the information array.
    int numEntries = btreeMaster->totalEntries;
    int (*informationArray)[3] = btreeMaster->informationArray;

    sortInformationList(informationArray, numEntries);

    // Get the key to find.
    int keyToFind = getLowestFirstIndex(informationArray, numEntries, searchKey);
  
    RecordInfo *r = findKeyInArray(informationArray, numEntries, keyToFind);

    if (r == NULL) {
        return RC_IM_NO_MORE_ENTRIES;
    }

    // Set the result and update the search key.
    *result = r->recordId;
    scannedData->searchKey = searchKey + 1;

    return RC_OK;
}
 

//This method is used to close the btree scan
RC closeTreeScan(BT_ScanHandle *handle)
{
    // Check if handle is NULL.
    if (handle == NULL) {
        return RC_NULL_ARGUMENT;
    }
    BTreeScanner *scannedData = (BTreeScanner *) handle->mgmtData;
    free(scannedData);
    free(handle);
    handle = NULL;

    return RC_OK;
}

int path_to_root(Node *root, Node *node) {
    int length = 0;
    while (node != root) {
        node = node->parent;
        length++;
    }
    return length;
}


void print_subtree(BTreeMaster *treeManager, Node *node, int rank) {
    if (node == NULL) return;

    int new_rank = path_to_root(treeManager->baseNode, node);
    if (new_rank != rank) {
        rank = new_rank;
        printf("\n");
    }

    for (int i = 0; i < node->keyCount; i++) {
        switch (treeManager->keyType) {
        case DT_INT:
            printf("%d ", (*node->keyPointers[i]).v.intV);
            break;
        case DT_FLOAT:
            printf("%.02f ", (*node->keyPointers[i]).v.floatV);
            break;
        case DT_STRING:
            printf("%s ", (*node->keyPointers[i]).v.stringV);
            break;
        case DT_BOOL:
            printf("%d ", (*node->keyPointers[i]).v.boolV);
            break;
        }
        printf("(%d - %d) ", ((RecordInfo *)node->nodePointers[i])->recordId.page, ((RecordInfo *)node->nodePointers[i])->recordId.slot);
    }
    printf("| ");

    if (!node->isLeafNode) {
        for (int i = 0; i <= node->keyCount; i++) {
            print_subtree(treeManager, node->nodePointers[i], rank);
        }
    }
}

void printTreeHelper(BTreeMaster *btreeMaster, int rows, int numCols)
{
    for(int i=0;i<rows;i++)
    {
       printf("Key: %d, Page: %d, Slot = %d\n", btreeMaster->informationArray[i][0],btreeMaster->informationArray[i][1],btreeMaster->informationArray[i][2]);
    }

}

extern char *printTree(BTreeHandle *tree) {

    BTreeMaster *btreeMaster = (BTreeMaster *) tree->mgmtData;
    printf("------------------------PRINTING TREE-----------------------\n");

    if (btreeMaster->baseNode == NULL) {
        printf("Empty tree.\n");
        return '\0';
    }

    int numEntries = btreeMaster->totalEntries;

    printTreeHelper(btreeMaster, numEntries, 3);
    printf("\n");

    return '\0';
}




