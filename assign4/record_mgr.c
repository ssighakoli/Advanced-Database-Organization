#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"
#include <time.h>


typedef struct RecordMaster
{
	RID recID;
	TID tid;
	PageNumber firstFreePage;
	int rowsCount;
	int numRecScanned;
	BM_PageHandle pageHandle;	
	BM_BufferPool bufferPool;
	Expr *scanCondition;
} RecordMaster;


//necessary global variables declared
bool isRecManagerOpen = false;
const int ATTR_CAPACITY = 15;
const int SCHEMA_OVERMAX = ((PAGE_SIZE-16) / 80 ) + 1;
const int SCHEMA_TYPELENGTH_OVERMAX = 4084;
const int NO_PAGE_VALUE = -1;
const int NO_SLOT_VALUE = -1;
const int CURRENT_TRANSACTION_ID = 0;

RecordMaster *recMgr;


 //TID Helpers
TID generate_tid(int transactionID, int page, int slot) {
	TID tid;
	tid.transactionID = transactionID;
	tid.page = page;
	tid.slot = slot;
	return tid;
}

void attach_tid(TID tid, Record* record) {
	record->tid = tid;
}

extern int generateRandomTransactionId()
{
    int transaction_id = 0;
    while (transaction_id < 100000 || transaction_id > 999999) {
        transaction_id = rand(); 
        transaction_id = transaction_id % 1000000;  
    }
    return transaction_id;
}

// This is a helper method to findout a freeslot based on data and recordSize
int getAvailableSlot(char *data, int recordSize)
{
    int t = 0, noOfSlots = PAGE_SIZE / recordSize;
    char *ptr = data;
    while (t < noOfSlots) {
        if (*ptr != '+') {
            return t;
        }
        ptr += recordSize;
        t++;
    }
    return -1;
}


//This helper function is used to store the information onto storage handler
extern RC storeInformationToStorageHandler(char *name, char data[])
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

//This helper function is used to mark a page dirty and unpin it.
extern RC markDiryAndUnpin(RecordMaster *recMgr)
{
   int RC_State;
   if((RC_State =  markDirty(&recMgr->bufferPool, &recMgr->pageHandle)) != RC_OK)
	{
		return RC_State;
	}
   if((RC_State =  unpinPage(&recMgr->bufferPool, &recMgr->pageHandle)) != RC_OK)
	{
		return RC_State;
	}
   return RC_OK;
}


// RECORD MANAGER FUNCTIONS

// This method is used to initialize the record manager.
extern RC initRecordManager (void *mgmtData)
{
	FILE *pointerToTable;
	pointerToTable = NULL;
	if(pointerToTable != NULL)
	{
	   printf("initRecordManager(): Record Manager did not initialize properly!");
	   return RC_ERROR;
	}
	else
	{
		isRecManagerOpen = true;
	}
	return RC_OK;
}

// This method is used to shutdown the record manager.
extern RC shutdownRecordManager ()
{
	recMgr = NULL;
	free(recMgr);
	if(recMgr == NULL)//Errror handling checks before shutting down the record manager
	{
	   isRecManagerOpen = false;
	}
	else
	{
		printf("shutdownRecordManager(): Record Manager did not shutdown properly.");
		return RC_ERROR;// return error with the above statement
	}
	return RC_OK;
}


// This method is used to create a table in the given schema with a given name
extern RC createTable (char *name, Schema *schema)
{
	char data[PAGE_SIZE];
	ReplacementStrategy strategy = RS_LRU;//calling the strategy implemented in storage manager to initiate bufferpool
	recMgr = (RecordMaster*) malloc(sizeof(RecordMaster));

	initBufferPool(&recMgr->bufferPool, name, 100, strategy, NULL);//Buffer manager is used to initiate bufferpool

	if(schema->numAttr > SCHEMA_OVERMAX)//verifying the schema size with the given fixed size
	{
		return RC_SCHEMA_TOO_BIG;
	}
	else
	{
		char *dataHandle = data;
		const int values[] = {0, 1, schema->numAttr, schema->keySize};
		const int numValues = sizeof(values) / sizeof(int);
		for (int m = 0; m < numValues; m++) {
			*(int*)dataHandle = values[m];
			dataHandle += sizeof(int);
		}
		for(int n = 0; n < schema->numAttr; n++)
    	{
			int schemaTypeLength = (int) schema->typeLength[n];
			if(schemaTypeLength >= SCHEMA_TYPELENGTH_OVERMAX)
			{
			    return RC_SCHEMA_TYPELENGTH_TOO_BIG;
			}
			else
			{
				memcpy(dataHandle, schema->attrNames[n], ATTR_CAPACITY);
				dataHandle = dataHandle + ATTR_CAPACITY;
				*(int*)dataHandle = (int)schema->dataTypes[n];
				dataHandle = dataHandle + sizeof(int);
                *(int*)dataHandle = schemaTypeLength;
			}
	       	dataHandle = dataHandle + sizeof(int);
    	}
	}
	storeInformationToStorageHandler(name, data);
	return RC_OK;
}

Schema* setSchemaAttributes(SM_PageHandle smHandle, int totalAttributes)
{
    Schema* newSchema = (Schema*)malloc(sizeof(Schema));
    newSchema->numAttr = totalAttributes;// Attribute names, typelength and datatypes are set using this function
    newSchema->attrNames = (char**)malloc(sizeof(char*) * totalAttributes);
    newSchema->dataTypes = (DataType*)malloc(sizeof(DataType) * totalAttributes);
    newSchema->typeLength = (int*)malloc(sizeof(int) * totalAttributes);

	int totalSize = sizeof(char) * totalAttributes * ATTR_CAPACITY;
    char* attributesBuffer = (char*)malloc(totalSize);

	for (int n = 0; n < totalAttributes; n++)
    {
        newSchema->attrNames[n] = attributesBuffer + n * ATTR_CAPACITY;
        memcpy(newSchema->attrNames[n], smHandle, ATTR_CAPACITY);
        smHandle += ATTR_CAPACITY;

	    newSchema->dataTypes[n] = *(DataType*)smHandle;
		smHandle += sizeof(int);
		*(int*)(newSchema->typeLength + n) = *(int*)smHandle;
		smHandle += sizeof(int);

    }
    return newSchema;
}


// Below method opens the table when the name of the table is given
extern RC openTable (RM_TableData *rel, char *name)
{
	SM_PageHandle smHandle = NULL;    
	int totalAttributes;
	Schema *newSchema;

	rel->mgmtData = recMgr;
	rel->name = name;
    
	pinPage(&recMgr->bufferPool, &recMgr->pageHandle, 0);//Buffer manager pinpage used to pin the page initially to 0
	smHandle = (char*) recMgr->pageHandle.data;

	memcpy(&recMgr->rowsCount, smHandle, sizeof(int));
	memcpy(&recMgr->firstFreePage, smHandle += sizeof(int), sizeof(int));
	memcpy(&totalAttributes, smHandle += sizeof(int), sizeof(int));

	newSchema = setSchemaAttributes(smHandle,totalAttributes);//this containes the new values set using setSchemaAttributes.
	rel->schema = newSchema;//returns the newly added schema

	markDiryAndUnpin(recMgr);//marks dirty and unpins the page as a best practice
	return RC_OK;
} 

 
// This method is used to close the table when the pointer to the table is given
extern RC closeTable (RM_TableData *rel)
{
	recMgr = rel->mgmtData;
	forceFlushPool(&recMgr->bufferPool);//flushes the buffer pool to make it empty 
	isRecManagerOpen = false;
	return RC_OK;
}

// This method is used to delete the table with the given name
extern RC deleteTable (char *name)
{
	FILE *tableToDelete;
	tableToDelete = fopen(name, "r");
	if(tableToDelete == NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	remove(name);// the table named "name" would be removed as to make delete table successful
	isRecManagerOpen = false;
	return RC_OK;
}

// This method is used to get the number of tuples when pointer to table is given
extern int getNumTuples (RM_TableData *rel)
{
	recMgr = rel->mgmtData;
    int numberOfTuples = recMgr->rowsCount;
	return numberOfTuples;// returns the total count of tuples in the table
}


// RECORD FUNCTIONS



extern void setRecordParams(RM_TableData *rel, Record *record, int sizeOfRecord)
{
    RecordMaster *recMgr = rel->mgmtData;
    RID *recID = &record->id;
    recID->page = recMgr->firstFreePage;// identifies the first available page
	pinPage(&recMgr->bufferPool, &recMgr->pageHandle, recID->page);//pins that page using buffermanager pinpage method
	recID->slot = getAvailableSlot(recMgr->pageHandle.data, sizeOfRecord);
} 


//The purpose of below method is to add a new record to the table that is referred to by the variable "rel".
//After successfully inserting the new record, the Record ID of the newly inserted record is stored in the 'record' parameter.
extern RC insertRecord(RM_TableData *rel, Record *record) {

	RID *recID = &record->id;
	char *recordData, *slotAddr;
	int transactionID = 0;

	int sizeOfRecord = getRecordSize(rel->schema);
	setRecordParams(rel, record,sizeOfRecord);

	recordData = recMgr->pageHandle.data;
	
    for (; recID->slot == -1; recID->page++) {
        unpinPage(&recMgr->bufferPool, &recMgr->pageHandle);
        pinPage(&recMgr->bufferPool, &recMgr->pageHandle, recID->page + 1);
        recordData = recMgr->pageHandle.data;
        recID->slot = getAvailableSlot(recordData, sizeOfRecord);
    }

	slotAddr = recordData + (recID->slot * sizeOfRecord);

	//TID's
	transactionID = generateRandomTransactionId();
	TID tid = generate_tid(transactionID, recID->page, recID->slot);
	attach_tid(tid, record);

	printf("TID and Tombstone Implentation successfull in INSERT record\n");

	memmove(slotAddr, "+", sizeof(char));

    for (int i = 0; i < sizeOfRecord - 1; i++) {
        *(slotAddr + i + 1) = *(record->data + i + 1);
    }

    markDiryAndUnpin(recMgr);
    recMgr->rowsCount++;

    pinPage(&recMgr->bufferPool, &recMgr->pageHandle, 0);
    return RC_OK;
}


//Below method removes a specific record identified by the "id" from the table referred to by the "rel".
extern RC deleteRecord (RM_TableData *rel, RID id)
{
	int transactionID = 0;
    // Retrieving our meta data stored in the table
	RecordMaster *recMgr = rel->mgmtData;
	BM_BufferPool bufferpool = recMgr->bufferPool;
	BM_PageHandle pageHandle = recMgr->pageHandle;
	PageNumber pageNumber = id.page;
	
    // Pinning the page which needs to be  deleted
	pinPage(&bufferpool, &pageHandle, pageNumber);

    // Updating free page
	recMgr->firstFreePage = pageNumber;
	
	char *recordData = recMgr->pageHandle.data + id.slot * getRecordSize(rel->schema);

	transactionID = generateRandomTransactionId();
	TID tid = generate_tid(transactionID, id.page, id.slot);
	attach_tid(tid, (Record*)recordData);


	printf("TID and Tombstone Implentation successfull in DELETE record\n");
	
    // Implementing tombstone mechanism using '-'. Here, '-' implies that  record is deleted
	*recordData = '-';
		
    markDiryAndUnpin(recMgr);
    
    //return SUCCESS
	return RC_OK;
}

void setTomstoneValue(char* recordData, int offset, char tombstone) {
    recordData[offset] = tombstone;
}


//Below method modifies a record identified by the "record" in the table referred to by the "rel".
extern RC updateRecord (RM_TableData *rel, Record *record)
{	
    // Retrieving our meta data stored in the table
	RecordMaster *recMgr = rel->mgmtData;
	int transactionID = 0;
  
	pinPage(&recMgr->bufferPool, &recMgr->pageHandle, record->id.page); // Pinning the page which needs to be  updated

	char *recordData;
	int sizeOfRecord = getRecordSize(rel->schema);     // Getting the size of the record using getRecordSize()

    // Setting id
	RID id = record->id;
	
	recordData = recMgr->pageHandle.data + (id.slot * sizeOfRecord);   // reading record's memory location   
	setTomstoneValue(recordData, 0, '+');  // Implementing tombstone mechanism using '+'. Here, '+' implies that  record is not empty

	transactionID = generateRandomTransactionId();
	TID tid = generate_tid(transactionID, id.page, id.slot);
	attach_tid(tid, record);

	printf("TID and Tombstone Implentation successfull in UPDATE record\n");
	
    // Using memmove() for Copying the updated record data to the exisitng one;
	memmove(recordData + 1, record->data + 1, sizeOfRecord - 1);
	
    markDiryAndUnpin(recMgr);
	
    //return SUCCESS
	return RC_OK;	
}



//Below method retrieves a specific record identified by the "id" from the table referred to by the "rel".
extern RC getRecord (RM_TableData *rel, RID id, Record *record)
{
	// Getting record maager's  meta data from the table
	RecordMaster *recMgr = rel->mgmtData;
	int transactionID = 0;
	if(recMgr!=NULL)
	{
	
	char *recordData,*dataPtr;
    // Using pinPage() to pin the page with record we need to get/retreive
	pinPage(&recMgr->bufferPool, &recMgr->pageHandle, id.page);

	int sizeOfRecord = getRecordSize(rel->schema);    // Calculate record size
	char *dataAddr = recMgr->pageHandle.data + (id.slot * sizeOfRecord);
	if(*dataAddr == '+')
	{
        //Set record's id
        record->id = id;
		transactionID = generateRandomTransactionId();
		TID tid = generate_tid(transactionID, id.page, id.slot);
		attach_tid(tid, record);
		printf("TID and Tombstone Implentation successfull in GET record\n");
        recordData = record->data;
        dataPtr = dataAddr + 1;
        for (int n = 1; n < sizeOfRecord; n++) {
			recordData[n] = *dataPtr;
			dataPtr++;
		}
        
	}
	else
	{
        // Returns error message implying no matching record is found in the table for Record->id.
		printf("getRecord(): Get record failed as there was no matching record is found in the table for Record->id\n");
        return RC_RM_NO_TUPLE_WITH_GIVEN_RID;
        
	}
	markDiryAndUnpin(recMgr);
	return RC_OK; //return SUCCESS
	}
	else
	{
		printf("getRecord(): Record manager is NULL\n");
		return RC_ERROR;
	}
} 


// ******** SCAN FUNCTIONS ******** //

void scanMgrInitialiser(RecordMaster* scanMgr, Expr* cond) {
    RID recID = { .page = 1, .slot = 0 };
    scanMgr->scanCondition = cond;
    scanMgr->numRecScanned = 0;
    scanMgr->recID = recID;
}


// This function scans all the records using the condition
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
    RecordMaster *recMgrForScan;
	RecordMaster *recMgr = rel->mgmtData;  
	if (cond != NULL)  // Check for scan condition
	{
   
	openTable(rel, "Table_To_Scan"); // Opening the table in memory

	recMgrForScan = (RecordMaster*) malloc(sizeof(RecordMaster)); // Allocating required memory for the scanManager
	scan->mgmtData = recMgrForScan;     // Setting the scan manager's meta data values with our meta data
    	
    scanMgrInitialiser(recMgrForScan, cond);     // Set scan manager variables using custom helper function

    recMgr->rowsCount = ATTR_CAPACITY;  // Setting the table manager's meta data values with our meta data

    scan->rel= rel; // Setting the scan's table (table that needs to be searched based on a specific condition)
    return RC_OK;	//return SUCCESS
	}
	else
	{
		printf("startScan(): Scanning the table failed! \n");
		return RC_SCAN_CONDITION_NOT_FOUND;
	}
}


// helper function to increment the record ID by one slot
void incrementRecID(RID* recID, int totalSlots) {
    recID->slot++;
    if (recID->slot >= totalSlots) {
        recID->slot = 0;
        recID->page++;
    }
}

// helper function to find the total number of slots available in a page
int getTotalSlots(Schema* schema) {
    int sizeOfRec = getRecordSize(schema);
    return PAGE_SIZE / sizeOfRec;
}

// helper function to check if there are more tuples to scan
bool hasMoreTuplesToScan(RecordMaster *recMgrForScan, RecordMaster *recMgrForTable) {
    int numOfScans = recMgrForScan->numRecScanned;
    int numOfRows = recMgrForTable->rowsCount;
    return (numOfScans < numOfRows);
}

// helper function to read a record from a slot
void readRecordFromSlot(RecordMaster *recMgrForTable, RecordMaster *recMgrForScan, Record *record, char *recordData, int sizeOfRec) {
    char* dataAddr = record->data;
    *dataAddr = '-';
	memmove(++dataAddr, recordData + 1, sizeOfRec - 1);
	record->id = recMgrForScan->recID;
    recMgrForScan->numRecScanned++;
}

// helper function to evaluate the scan condition
bool evaluateScanCondition(Record *record, Schema *schema, Expr *scanCondition) {
    Value* result = (Value*)malloc(sizeof(Value));
    evalExpr(record, schema, scanCondition, &result);
    bool res = result->v.boolV;
    freeVal(result);
    return res;
}

extern RC next(RM_ScanHandle* scan, Record* record) {
    RecordMaster* recMgrForScan = scan->mgmtData;
    RecordMaster* recMgrForTable = scan->rel->mgmtData;
    Schema* schema = scan->rel->schema;

    if (recMgrForScan->scanCondition == NULL) {
        return RC_SCAN_CONDITION_NOT_FOUND;
    }

    if (!hasMoreTuplesToScan(recMgrForScan, recMgrForTable)) {
        return RC_RM_NO_MORE_TUPLES;
    }

    int totalSlots = getTotalSlots(schema);

    while (hasMoreTuplesToScan(recMgrForScan, recMgrForTable)) {
        if (recMgrForScan->numRecScanned == 0) {
            recMgrForScan->recID = (RID) {1, 0};
            pinPage(&recMgrForTable->bufferPool, &recMgrForScan->pageHandle, recMgrForScan->recID.page);
        }
        else {
            if (recMgrForScan->recID.slot >= totalSlots - 1) {
                unpinPage(&recMgrForTable->bufferPool, &recMgrForScan->pageHandle);
                recMgrForScan->recID = (RID) {recMgrForScan->recID.page + 1, 0};
                pinPage(&recMgrForTable->bufferPool, &recMgrForScan->pageHandle, recMgrForScan->recID.page);
            }
            else {
                incrementRecID(&recMgrForScan->recID, totalSlots);
            }
        }

        char* recordData = recMgrForScan->pageHandle.data + (recMgrForScan->recID.slot * getRecordSize(schema));
        readRecordFromSlot(recMgrForTable, recMgrForScan, record, recordData, getRecordSize(schema));

        if (evaluateScanCondition(record, schema, recMgrForScan->scanCondition)) {
            unpinPage(&recMgrForTable->bufferPool, &recMgrForScan->pageHandle);
            return RC_OK;
        }
    }

    unpinPage(&recMgrForTable->bufferPool, &recMgrForScan->pageHandle);

	recMgrForScan->recID = (RID) {1, 0};
	memset(&recMgrForScan->numRecScanned, 0, sizeof(int));
	
    // This return message implies there are no more tuples left to scan
	return RC_RM_NO_MORE_TUPLES;
}






// This function closes the scan operation.
extern RC closeScan (RM_ScanHandle *scan)
{
	RecordMaster *recMgr = scan->mgmtData;
	recMgr = NULL;
    free(recMgr);
	if(recMgr != NULL)
	{
		printf("closeScan(): Close scan failed\n");
		return RC_ERROR;
	}
    return RC_OK;
}

// SCHEMA FUNCTIONS

//This method is used to calculate the recordsize of the schema when a pointer to schema is given
extern int getRecordSize(Schema *schema)
{
    int recordSize=0;
    for (int j = 0; j < schema->numAttr; j++) {
        DataType type = schema->dataTypes[j];
        if (type == DT_INT) {
            recordSize += sizeof(int);
        } else if (type == DT_FLOAT) {
            recordSize += sizeof(float);
        } else if (type == DT_BOOL) {
            recordSize += sizeof(bool);
        } else if (type == DT_STRING) {
            recordSize += schema->typeLength[j];
        } else {
            printf("Datatype is not INT/FLOAT/BOOL/STRING\n");
        }
    }
    return ++recordSize;
}

// This method is used to create a new schema when all the required items of Schema struct are given
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	//Using calloc function to allocate memory 
	Schema *newSchema = (Schema *) calloc(1, sizeof(Schema)); 
	
	//Assigning values to the new schema
	*newSchema = (Schema){
		.numAttr = numAttr,
		.attrNames = attrNames,
		.dataTypes = dataTypes,
		.typeLength = typeLength,
		.keySize = keySize,
		.keyAttrs = keys
	};

	return newSchema; 
}


// This method is used to free the schema when a pointer to the schema is given
extern RC freeSchema(Schema *schema) {
    if (schema == NULL) {
		printf("freeSchema(): Schema free failed!\n");
        return RC_ERROR;
    }

    free(schema);
    return RC_OK;
}


// ATTRIBUTE FUNCTIONS

// This function creates a new record in the schema referenced by "schema"
extern RC createRecord(Record **record, Schema *schema) {

    if (!record || !schema) {
		printf("createRecord(): Record/Schema is NULL\n");
        return RC_ERROR;
    }

    // Using calloc function to allocate the memory
    Record *createdRecord = (Record*) calloc(1, sizeof(Record));
    if (!createdRecord) {
		printf("createRecord(): New record created is NULL\n");
        return RC_MEMORY_ALLOCATION_FAILED;
    }

    int sizeOfRecord = getRecordSize(schema); //Calculating size of record using getRecordSize() method

    createdRecord->data = (char*) malloc(sizeOfRecord);
    if (!createdRecord->data) {
		printf("createRecord(): New record's data created is NULL\n");
        free(createdRecord);
        return RC_MEMORY_ALLOCATION_FAILED;
    }

    createdRecord->id.page = NO_PAGE_VALUE;
    createdRecord->id.slot = NO_SLOT_VALUE;
    memset(createdRecord->data, '-', 1); // Using tombstone value
    memset(createdRecord->data + 1, '\0', 1);

    *record = createdRecord; 

    return RC_OK;
}


//This is a helper method used to calculate the attributeOffset
RC calculateAttributeOffset(Schema* schema, int attrNum, int* result) {
    *result = 1;
    for (int j = 0; j < attrNum; j++) {
        DataType dataType = schema->dataTypes[j];
        *result += (dataType == DT_INT) ? sizeof(int)
            : (dataType == DT_FLOAT) ? sizeof(float)
            : (dataType == DT_BOOL) ? sizeof(bool)
            : (dataType == DT_STRING) ? schema->typeLength[j]
            : (printf("Datatype is not INT/FLOAT/BOOL/STRING\n"), RC_RM_UNKOWN_DATATYPE);
    }
    return RC_OK;
}


// This method is used to free a record whena  pointer to the record is given
extern RC freeRecord (Record *record)
{
	if(record==NULL)
	{
		printf("freeRecord(): Record free failed! \n");
		return RC_ERROR;
	}
	else
	{
		free(record);
		return RC_OK;
	}
}

//This method is used to get an attribute when an attributeNumber is given from a given record and schema
extern RC getAttr(Record *record, Schema *schema, int attrNum, Value **value)
{
    int attributeOffset = 0;
    calculateAttributeOffset(schema, attrNum, &attributeOffset); // calculating attribute_offset
    Value *attrValue = calloc(1, sizeof(*attrValue));
	if (attrValue == NULL) {
		printf("getAttr(): Memory allocation failed for attribute value.\n");
		return RC_ERROR;
	}
    char *dataAddr = record->data;
    dataAddr = dataAddr + attributeOffset;
	if (attrNum != 1)
	{
		schema->dataTypes[attrNum] = schema->dataTypes[attrNum];
	}
	else {
		schema->dataTypes[attrNum] = 1;
	}

	schema->dataTypes[attrNum] == DT_INT
    ? (memcpy(&attrValue->v.intV, dataAddr, sizeof(int)), attrValue->dt = DT_INT)
    : schema->dataTypes[attrNum] == DT_FLOAT
        ? (memcpy(&attrValue->v.floatV, dataAddr, sizeof(float)), attrValue->dt = DT_FLOAT)
        : schema->dataTypes[attrNum] == DT_BOOL
            ? (memcpy(&attrValue->v.boolV, dataAddr, sizeof(bool)), attrValue->dt = DT_BOOL)
            : schema->dataTypes[attrNum] == DT_STRING
                ? (attrValue->v.stringV = (char*)malloc(schema->typeLength[attrNum] + 1),
                    strncpy(attrValue->v.stringV, dataAddr, schema->typeLength[attrNum]),
                    attrValue->v.stringV[schema->typeLength[attrNum]] = '\0',
                    attrValue->dt = DT_STRING)
                : (printf("Datatype is not INT/FLOAT/BOOL/STRING\n"), RC_RM_UNKOWN_DATATYPE);

    *value = attrValue;
    return RC_OK;
}


//This method is used to set an attribute in the given record and schema
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value) {
    int attributeOffset = 0;
    calculateAttributeOffset(schema, attrNum, &attributeOffset);
    char *dataAddr = record->data;
    dataAddr += attributeOffset;

    int dataType = schema->dataTypes[attrNum];
    int sizeOfDatatype=0;

    dataType == DT_INT ? (*(int *) dataAddr = value->v.intV, sizeOfDatatype = sizeof(int))
        : dataType == DT_FLOAT ? (*(float *) dataAddr = value->v.floatV, sizeOfDatatype = sizeof(float))
        : dataType == DT_BOOL ? (*(bool *) dataAddr = value->v.boolV, sizeOfDatatype = sizeof(bool))
        : dataType == DT_STRING ? (strncpy(dataAddr, value->v.stringV, schema->typeLength[attrNum]), sizeOfDatatype = schema->typeLength[attrNum])
        : (printf("Datatype is not INT/FLOAT/BOOL/STRING\n"), RC_RM_UNKOWN_DATATYPE);

    dataAddr += sizeOfDatatype;
    return RC_OK;
}
