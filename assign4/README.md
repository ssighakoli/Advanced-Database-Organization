# Assignment 4: B+ TREE (INDEX MANAGER)

- The goal of this assignment is to implement a B tree Index. <br>
- The index should be backed up by a page file and pages of the index should be accessed through your buffer manager. <br>
- A B+-tree stores pointer to records index by a keys of a given datatype. Pointers to intermediate nodes should be represented by the page number of the page the node is stored in. <br>
- The insertion rules are as below:

#### Leaf Split: 
In case a leaf node need to be split during insertion and n is even, the left node should get the extra key. For odd values of n we can always evenly split the keys between the two nodes. In both cases the value inserted into the parent is the smallest value of the right node.

#### Non-Leaf Split: 
In case a non-leaf node needs to be split and n is odd, we cannot split the node evenly (one of the new nodes will have one more key). In this case the "middle" value inserted into the parent should be taken from the right node.

#### Leaf Underflow: 
In case of a leaf underflow your implementation should first try to redistribute values from a sibling and only if this fails merge the node with one of its siblings. Both approaches should prefer the left sibling. 

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
1. Allowing different datatypes to be used as keys. Initially the assignment was to support only INT datatype, we have accommodated FLOAT, STRING and BOOLEAN datatypes and added their comparision using helpers accordingly.

## SOLUTION DESCRIPTION:

### INITIALIZATION AND SHUTDOWN:

#### 1.initIndexManager():
- We initialize btree master by using a file pointer.
- initIndexManager() is successfully initialized to start btree manager functions.

#### 2.shutdownIndexManager():
- Once the functionalties are performed, we shutdown and free the btree master.
- We also set the btree master to null before freeing it.

### BTREE OPERATIONS:

#### 1.createBtree():
- This method is used to create a new btree. 
- We allocate the memory to custom BTree master node and set the required parameters such as keytype and number of nodes.
- Additionally, we also have a condition to check the max nodes. 
- We store all this information about this BTree into the storage manager using custom storeInformationToStorageHandle method.

#### 2.openBtree():
- This method is used to open an already existing BTree.
- It takes the pointer to the tree and the pointer to the file containing BTree as input. 
- Initally, a null check is performed on the input parameters and a custom error code 'RC_NULL_ARGUMENT' is returned if any of the input parameters are NULL.
- We allocate the memory to new tree handle using calloc function and set the handle's mgmtData to the existing custom master node.
- openPageFile method from storage handler is used to open the file containing BTree by taking in file_name and filehandler pointer as input. 

#### 3.closeBtree():
- This method is used to close the BTree.
- It takes the pointer to BTreeHandle as input. 
- Initally, a null check is performed on the BTree pointer and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- closePageFile method from fileHandler is used to close the BTree. It takes the fileHandler containing BTree as input.
- If closePage is successful, then the custom created BTree master and current nodes are freed.

#### 4.deleteBtree():
- This method is used to delete the BTree.
- It takes the file name pointer as the input.
- Initally, a null check is performed on the input file name and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- The file is then deleted using remove() method.
- This is not same as deleteKey() method, here we delete the full tree and not a single key. 


### ACCESSING INFORMATION ABOUT B -TREE:

#### 1.getNumNodes():
- This method is used to return the number of nodes in the BTree.
- It takes the pointer to BTree handle and the pointer to the result as input.
- Initally, a null check is performed on the input BTree pointer and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- In our custom BTree master struct, we have a parameter to track the number of nodes in the BTree at any point. So, on calling this method, numberOfNodes in BTree master struct is returned.

#### 2.getNumEntries():
- This method is used to return the number of entries in the BTree.
- It takes the pointer to BTree handle and the pointer to the result as input.
- Initally, a null check is performed on the input BTree pointer and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- In our custom BTree master struct, we have a parameter to track the number of entries in the BTree at any point. So, on calling this method, numberOfEntries in BTree master struct is returned.

#### 3.getKeyType():
- This method is used to return the datatype of keys in the BTree.
- It takes the pointer to BTree handle and the pointer to the result as input.
- Initally, a null check is performed on the input BTree pointer and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- In our custom BTree master struct, we have a parameter to store the datatype of BTree at any point. So, on calling this method, dataType in BTree master struct is returned.
- This was stored in the struct during createBtree() method where we received keytype as the input. 

### ACCESSING B-TREE INDEX:

#### 1.findKey():
- This method is used to find a particular key in the BTree and return RC_OK if the key is found, along with the pointer to RID corresponding to this key.
- It takes pointer to BTree and the key to be found as the input.
- Initally, a null check is performed on the input BTree pointer and the key to be found and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- The key is later found using custom created binarySearch() method. 
- If the key is not found, a custom error code RC_IM_KEY_NOT_FOUND is returned.

#### 2.insertKey():
- This method is used to insert an element into the BTree.
- It takes pointer to BTree, the key to be inserted, and the RID corresponding to this key as the input.
- Initally, a null check is performed on the input BTree pointer and the key to be inserted and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- Initially, we check if the key already exists in the BTree using the above findkey() method.
- If the key does not exist, a new node for this new key is created using custom createNode() method and the pointers are incremented to point to appropriate parent node and the next sibling.
- If the key to be inserted finds place in leaf, it is inserted directly and there will be no change to pointers. 
- If the key to be inserted needs to manipulate the non leaf node or root node, a custom splitNode() method is used to take care of these manipulations. 

#### 3.deleteKey():
- This method is used to delete a key from the BTree.
- It takes pointer to BTree, the key to be deleted as the input.
- Initally, a null check is performed on the input BTree pointer and the key to be deleted and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- Initially, we check if the key to be deleted exists in the BTree using the above findkey() method.
- If the key exists() and if the key is in leaf node, it is deleted directly by checking if the balance of the key is maintained post deletion.
- If the key to be deleted is in non leaf node or parent node, node redistribution is done using the custom redistributeNodes() method.

#### 4.openTreeScan():
- This method is used to open the tree scan handler.
- It takes pointer to BTree and the scan handle as the input.
- Initally, a null check is performed on the input BTree pointer and the scan handle and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- Allocate memory to the scan handler using malloc.
- Three custom functions bubbleSort(), updateBtree() and populateArrays() methods are used to populate the scan handle from our custom BTree structure.

#### 5.nextEntry():
- This method is used to return the next entry of the key. 
- In B-Tree, we usually maintain amongst the leaf node, this method tests that functionality.
- It takes BTree scan handle and the pointer to RID (the result to be returned) as the input.
- Initally, a null check is performed on the input scan handle and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- Check if the node to be returned is the last node using custom isLastNode() method, if it returns true, we know that we have no more entries and a custom error code RC_IM_NO_MORE_ENTRIES is returned.
- Next condition is to check if the key is the last entry in the non-last node using isLastKeyInNode() method, if it returns true, we need to increment the pointer to point this to next sibling. 
- Using findKey() and getNextNode() methods, the pointer to RID of the node found is returned.

#### 6.closeTreeScan():
- This method is used to close the BTree scan handle.
- It takes pointer to scan handle as the input.
- Initally, a null check is performed on the  scan handle and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- If it is not null, we free the handle and its mgmtData.

### DEBUGGING FUNCTIONS

#### printTree():
- This method is used to know the status of BTree at any point.
- It takes the pointer to BTree as input.
- Initally, a null check is performed on the input BTree pointer and a custom error code 'RC_NULL_ARGUMENT' is returned if it is NULL.
- A custom printTreeNode() is used which takes in the custom Btree master and the depth as the input. 
- Initially the depth would be 0 (to start printing from root node) and will go on till node's next node is NULL.
