# Assignment 4: B+ TREE (INDEX MANAGER)

- The goal of this assignment is to implement a B tree Index. <br>
- The index should be backed up by a page file and pages of the index should be accessed through your buffer manager. <br>
- A B+-tree stores pointer to records index by a keys of a given datatype. Pointers to intermediate nodes should be represented by the page number of the page the node is stored in. <br>

## Steps to run the script:

1. Connect to fourier server using the command: ssh username@fourier.cs.iit.edu
2. Clone to the git repository using the commnd: git clone https://github.com/IITTeaching/cs525-s23-group-6.git
3. Change to 'cs525-s23-group-6' using the command: cd cs525-s23-group-6
4. List the files in the directory using the command: ls
5. Change to 'assign2' folder using the command: cd assign4
6. List the files in the directory using the command: ls
7. Use the command 'make clean' to delete old .o files of test1 and additional test.
8. Use the command 'make' to compile all files in the directory.
9. Use the command 'make run_test_1' to run test1.
10. Use the command 'make test_additional' to combine the additional test.
11. Use the command 'make run_test_additional' to run the additional test case.

## Additional functionality included for extra credit:
1. 

## SOLUTION DESCRIPTION:

### INITIALIZATION AND SHUTDOWN:

#### 1.initIndexManager():
- We initialize record manager with the help of storage manager.
- initIndexManager() is successfully initialized to start btree manager functions.

#### 2.shutdownIndexManager():
- Once the functionalties are performed, we shutdown and free the record manager.
- We also set the record manager to null before freeing it.

### BTREE OPERATIONS:

#### 1.createBtree():

#### 2.openBtree():

#### 3.closeBtree():

#### 4.deleteBtree():


### ACCESSING INFORMATION ABOUT B -TREE:

#### 1.getNumNodes():

#### 2.getNumEntries():

#### 3.getNumEntries():


### ACCESSING B-TREE INDEX:

#### 1.findKey():

#### 2.insertKey():

#### 3.deleteKey():

#### 4.openTreeScan():

#### 5.nextEntry():

#### 6.closeTreeScan():

### DEBUGGING FUNCTIONS


#### printTree():
