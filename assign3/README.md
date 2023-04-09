# Assignment 3: RECORD MANAGER

- The goal of this assignment is to implement a simple record manager that handles tables with fixed size schema. <br>
- Clients can insert records, delete records, update records, and scan through the records in a table. <br>
- A scan is associated with a search condition and only returns records that match the search condition. <br>
- Each table should be stored in a separate page file and your record manager should access the pages of the file through the buffer manager implemented in the prevoius assignment.


## Steps to run the script:

1. Connect to fourier server using the command: ssh username@fourier.cs.iit.edu
2. Clone to the git repository using the commnd: git clone https://github.com/IITTeaching/cs525-s23-group-6.git
3. Change to 'cs525-s23-group-6' using the command: cd cs525-s23-group-6
4. List the files in the directory using the command: ls
5. Change to 'assign2' folder using the command: cd assign3
6. List the files in the directory using the command: ls
7. Use the command 'make clean' to delete old .o files of test1 and additional test.
8. Use the command 'make' to compile all files in the directory.
9. Use the command 'make run_test_1' to run test1.
10. Use the command 'make test_additional' to combine the additional test.
11. Use the command 'make run_test_additional' to run the additional test case.

## Additional functionality included for extra credit:
1. TID and Tombstone mechanism to manage records and have a replica of them in the database. This implementation is performed by using a struct TID and adding it to the existing Record struct along with RID.
2. Adding a condition to check if schema size is larger than page size.
3. Adding a condition to check if record size is larger than data size.

## SOLUTION DESCRIPTION:

### TABLE AND RECORD MANAGER FUNCTIONS:

#### 1.initRecordManager():
- We initialize record manager with the help of storage manager.
- initStorageManager() is successfully initialized to start Record manager functions

#### 2.shutdownRecordManager():
- Once the functionalties are performed, we shutdown and free the record manager.
- We also set the record manager to null before freeing it.

#### 3.createTable():
- This method is creates a table in the declared schema with table name as "name".
- Buffer pool is initialized using LRU Replacement strategy with a total number of pages 80.
- Setup schema attributes, key size, attribute capacity to 15 and datatype and its lenghts.
- Further we store information to StorageHandler responsible to open, write,and close in the created page file.

#### 4.openTable():
- This method Opens the table that was created earlier.
- we pin the page to 0 initially using PinPage from bufferpool
- Allocate memory to schema
- set name,keysize,datatype
- unpin the page and forcepage

#### 5.closeTable():
- In this method, we close the table by closing the bufferbool.
- call shutdownBufferPool()

#### 6.deleteTable():
- This method opens the table in read mode.
- Attempts to delete the table.
- Error handling is implemented as a best practce

#### 7.getNumTuples():
- In this method we read the rows count and return them to record manager
### HANDLING RECORDS FUNCTIONS:

#### 1.insertRecord():
- This method is used to add a new record to the table that is referred to by the variable "rel". After successfully inserting the new record, the Record ID of the newly inserted record is stored in the 'record' parameter.

#### 2.deleteRecord():
- This method removes a specific record identified by the "id" from the table referred to by the "rel".
-Retrieve meta data from table, pin the page that is to be deleted, update free page with current pinned page and return success.
- This method implements tombstone mechanism using '-'.  indicating record for that entry has been deleted.

#### 3.updateRecord():
-This method modifies a record identified by the "record" in the table referred to by the "rel".
-Get meta data from the table, pin the page that is to be updated, use memmove() to copy the updated data to the existing record. Return Success.
- This method implements tombstone mechanism using '+'.  indicating record for that entry has been updated.

#### 4.getRecord():
- This method retrieves a specific record identified by the "id" from the table referred to by the "rel".
### SCAN FUNCTIONS:

#### 1.startScan():
- This method scans all the records using the condition.
- Check for scan condition, open the table, allocat memory for scan manager and set values for scan manager, scan for records that satisfy given condition and return success.

#### 2.next():
- This method examines every entry in the table and saves the matching entry (entry that meets the specified criteria) in the memory location indicated by the 'record' pointer.

#### 3.closeScan():
- Thie method closes the scan opereation if opened previously.

### SCHEMA FUNCTIONS:
These helper functions are used to return the size in bytes of records for a given schema and create a new schema.

#### 1.getRecordSize():
- This method is used to return the record size when schema is given.

##### This method is implemented as below:<br>
- We get the number of attributes from the given schema.
- For each attribute, we compute the size based on the datatype using sizeof function.

#### 2.createSchema():
- This method is used to create a schema when all its required attributes are given.

##### This method is implemented as below:<br>
- We need to allocate size to the new schema using malloc function.
- After that we set the values in the newly created schema namely number of attributes, attribute names, datatypes, type length, keySize and keys.

#### 3.freeSchema():
- This method is used to free the already existing schema when the schema name is given.
##### This method is implemented as below:<br>
- Check if given schema exists.
- If it exists, free the schema using free() method. 

### ATTRIBUTE FUNCTIONS:

#### 1.createRecord():
- This method is used to create a record in the given schema.

##### This method is implemented as below:<br>
- We allocate the memory to the new record using malloc function.
- We allocate the memory to the data part of the record using sizeof and malloc functions.
- By making use of tombstone mechanism, we set the pointer to '-'.

#### 2.freeRecord():
- This method is used to free the record when record is given.

##### This method is implemented as below:<br>
- Check if given record exists.
- If it exists, free the record using free() method. 

#### 3.getAttr():
- This method is used to get an attribute when record, schema and attribute number are given.

##### This method is implemented as below:<br>
- Calculate the attribute offset and set the datapointer to the offset position.
- Allocate the memory to the resultant attribute value using malloc function.
- Based on the datatype, we calculate the size using sizeof method.
- We copy the default values of the datatype using memcpy function for int, float and bool and strncpy function for char.

#### 4.setAttr():
- This method is used to set an attribute when record, schema, attribute number and value are given.

##### This method is implemented as below:<br>
- Calculate the attribute offset and set the datapointer to the offset position.
- Check the datatype of resultant attribute using the given attribute number.
- Based on the datatype, calculate the size and move the offset by size value.
- The pointer to the resultant attribute is given by 'value'.
