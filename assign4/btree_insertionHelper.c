#include "btree_insertionHelper.h"
#include "dt.h"
#include "string.h"

//This is a helper method to print a node
extern char* printNode(Node *n)
{
    int i;
	for (i = 0; i < n->keyCount; i++) {
			switch (DT_INT) {
			case DT_INT:
				printf("%d ", (*n->keyPointers[i]).v.intV);
				break;
			case DT_FLOAT:
				printf("%.02f ", (*n->keyPointers[i]).v.floatV);
				break;
			case DT_STRING:
				printf("%s ", (*n->keyPointers[i]).v.stringV);
				break;
			case DT_BOOL:
				printf("%d ", (*n->keyPointers[i]).v.boolV);
				break;
			}
			printf("(%d - %d) ", ((RecordInfo *) n->nodePointers[i])->recordId.page, ((RecordInfo *) n->nodePointers[i])->recordId.slot);
		}
}

//This is a helper method to print the node info
void printNodeInfo(Node *node) {
    printf("Node information:\n");
    printf("isLeafNode: %d\n", node->isLeafNode);
    printf("keyCount: %d\n", node->keyCount);
 
    
   printf("keys: ");
   for(int i=0;i<node->keyCount; i++)
   {
	   printf("%d\n", node->keyPointers[i]);

   }
    
    printf("parent: %p\n", node->parent);
}

//This is a helper method to sort keys present in the node
void sortKeys(Node * node) {
    int i, j;
    Value * temp_key;
    void * temp_pointer;

    for (i = 0; i < node->keyCount; i++) {
        for (j = i+1; j < node->keyCount; j++) {
            if (node->keyPointers[i]->v.intV > node->keyPointers[j]->v.intV) {
                // swap the keys
                temp_key = node->keyPointers[i];
                node->keyPointers[i] = node->keyPointers[j];
                node->keyPointers[j] = temp_key;

                // swap the pointers
                temp_pointer = node->nodePointers[i+1];
                node->nodePointers[i+1] = node->nodePointers[j+1];
                node->nodePointers[j+1] = temp_pointer;
            }
        }
    }
}


// Set the initial key-value pair and rightmost pointer of the given node.
void setInitialKeyValuePair(Node *node, int bTreeOrder, int key, RecordInfo *pointer) {
    node->keyPointers[0] = key;
    node->nodePointers[0] = pointer;
    node->nodePointers[bTreeOrder - 1] = NULL;
}

// Increment the number of keys in the node and the total number of entries in the B-tree.
void incrementKeyAndEntryCount(Node *node, BTreeMaster *treeManager) {
    node->keyCount++;
    treeManager->totalEntries++;
}

// This function initializes a B-tree with a single key-value pair.
Node *constructBTree(BTreeMaster *treeManager, Value *key, RecordInfo *pointer) {

    // Create a new leaf node that will act as the root node of the B-tree.
    Node *root = buildNode(treeManager);
	root->isLeafNode = true;
    root->treeLevel ++;

    // Initialize the root node with the given key-value pair.
    setInitialKeyValuePair(root, treeManager->treeOrder, key->v.intV, pointer);

    // Set the parent of the root node to NULL, as it is the top node in the tree.
    root->parent = NULL;
    root->isRootNode = true;

    // Update the root node's key count and the B-tree's total entry count.
    incrementKeyAndEntryCount(root, treeManager);

    // Return the newly created root node.
    return root;
}



// Helper function to find the insertion index
int findInsertionIndex(Node *leaf, int key, int bTreeOrder) {
    int insertion_index = 0;
    while (insertion_index < bTreeOrder - 1 && leaf->keyPointers[insertion_index] < key) {
        insertion_index++;
    }
    return insertion_index;
}

// Helper function to calculate split point
int calculateSplitPoint(int bTreeOrder) {
    return (bTreeOrder % 2 == 0) ? (bTreeOrder - 1) / 2 : (bTreeOrder - 1) / 2 + 1;
}

//This method is used to insert the key into leaf
Node *divideLeafAndInsert(BTreeMaster *treeManager, Node *leaf, Value *key, RecordInfo *pointer) {

    Node *new_leaf = buildNode(treeManager);
    new_leaf->isLeafNode = true;
    int bTreeOrder = treeManager->treeOrder;

    Value **temp_keys = calloc(1,bTreeOrder * sizeof(Value));
    void **temp_pointers = calloc(1,bTreeOrder * sizeof(void *));

    int insertion_index = findInsertionIndex(leaf, key->v.intV, bTreeOrder);

    memcpy(temp_keys, leaf->keyPointers, insertion_index * sizeof(Value *));
    temp_keys[insertion_index] = key;
    memcpy(temp_keys + insertion_index + 1, leaf->keyPointers + insertion_index, (bTreeOrder - 1 - insertion_index) * sizeof(Value *));

    memcpy(temp_pointers, leaf->nodePointers, insertion_index * sizeof(void *));
    temp_pointers[insertion_index] = pointer;
    memcpy(temp_pointers + insertion_index + 1, leaf->nodePointers + insertion_index, (bTreeOrder - 1 - insertion_index) * sizeof(void *));

    int split = calculateSplitPoint(bTreeOrder);

    // Update leaf and new_leaf nodes
    leaf->keyCount = split;
    memcpy(leaf->keyPointers, temp_keys, split * sizeof(Value *));
    memcpy(leaf->nodePointers, temp_pointers, split * sizeof(void *));

    new_leaf->keyCount = bTreeOrder - split - 1;
    memcpy(new_leaf->keyPointers, temp_keys + split, (bTreeOrder - split - 1) * sizeof(Value *));
    memcpy(new_leaf->nodePointers, temp_pointers + split, (bTreeOrder - split - 1) * sizeof(void *));

    // Set the rightmost pointers
    new_leaf->nodePointers[bTreeOrder - 1] = leaf->nodePointers[bTreeOrder - 1];
    leaf->nodePointers[bTreeOrder - 1] = new_leaf;
    leaf->isNodeFull = true;

    // Update the parent of new_leaf
    new_leaf->parent = leaf->parent;

    // Increment the number of entries in the B-tree
    treeManager->totalEntries++;

    // Free temporary arrays
    free(temp_pointers);
    free(temp_keys);

    // Insert the new leaf into the parent
    return updateParentAfterSplit(treeManager, leaf, new_leaf->keyPointers[0], new_leaf);
}


//This helper is used to return the split value
int get_splitValue(int bTreeOrder) {
    return bTreeOrder % 2 == 0 ? bTreeOrder / 2 : bTreeOrder / 2 + 1;
}

//This helper is used to copy temp to old node
Node *assign_temp_to_old_node(Node *node, Node **temp_pointers, Value **temp_keys, int split) {
    int i;
    for (i = 0; i < split - 1; i++) {
        node->nodePointers[i] = temp_pointers[i];
        node->keyPointers[i] = temp_keys[i];
        node->keyCount++;
    }
    node->nodePointers[i] = temp_pointers[i];
    return node;
}

//This helper is used to divide and insert into the node
Node *divideNodeAndInsert(BTreeMaster *treeManager, Node *old_node, int left_index, int key, Node *rightNode) {

    int i, divideAt, firstKeyInNextNode;
    Node *new_node, *child;
    Value **temp_keys;
    Node **temp_pointers;

    int bTreeOrder = treeManager->treeOrder;

    temp_pointers = calloc(1,(bTreeOrder + 1) * sizeof(Node *));
    temp_keys = calloc(1,bTreeOrder * sizeof(Value *));

     i = 0;
     while (i < old_node->keyCount + 1) {
    if (i == left_index + 1) {
        temp_pointers[i] = rightNode;
    } else {
        temp_pointers[i] = old_node->nodePointers[i];
    }
    i++;
     }

     i = 0;
     while (i < old_node->keyCount) {
    if (i == left_index) {
        temp_keys[i] = key;
    } else {
        temp_keys[i] = old_node->keyPointers[i];
    }
    i++;
     }

    divideAt = get_splitValue(bTreeOrder);

    new_node = buildNode(treeManager);
    old_node->keyCount = 0;

    old_node = assign_temp_to_old_node(old_node, temp_pointers, temp_keys, divideAt);

    firstKeyInNextNode = temp_keys[divideAt - 1];

    new_node = assign_temp_to_old_node(new_node, &temp_pointers[divideAt], &temp_keys[divideAt], bTreeOrder);

    free(temp_pointers);
    free(temp_keys);
    new_node->parent = old_node->parent;
    for (i = 0; i <= new_node->keyCount; i++) {
        child = new_node->nodePointers[i];
        child->parent = new_node;
    }

    treeManager->totalEntries++;

    return updateParentAfterSplit(treeManager, old_node, firstKeyInNextNode, new_node);
}

// Helper function to find the left index
int find_left_child_position(Node *primaryNode, Node *leftNode) {
    int left_index;
    for (left_index = 0; left_index <= primaryNode->keyCount && primaryNode->nodePointers[left_index] != leftNode; left_index++);
    return left_index;
}

//This helper is used to update the parent after splitting leaf or non leaf node
Node *updateParentAfterSplit(BTreeMaster *treeManager, Node *leftNode, int key, Node *rightNode) {

    Node *primaryNode = leftNode->parent;
    int bTreeOrder = treeManager->treeOrder;

    // Check if it is the new root.
    if (primaryNode == NULL) {
        return addToRoot(treeManager, leftNode, key, rightNode);
    }

    // Find the left index
    int left_index = find_left_child_position(primaryNode, leftNode);

    // Check if the new key can be accommodated in the node.
    if (primaryNode->keyCount < bTreeOrder - 1) {
        return enterKeyValuePair(primaryNode, left_index, key, rightNode);
    }
    primaryNode->isRootNode = true;
    primaryNode->treeLevel ++;
    // If it cannot be accommodated, split the node preserving the B+ Tree properties.
    return divideNodeAndInsert(treeManager, primaryNode, left_index, key, rightNode);
}




// Inserts a new key and pointer to a node into a node into which these can fit without violating the B+ tree properties.
Node * enterKeyValuePair(Node * primaryNode, int left_index, int key, Node * rightNode) {

	int i;
	for (i = left_index; i <= primaryNode->keyCount; i++) {
		primaryNode->nodePointers[i + 1] = primaryNode->nodePointers[i];
		primaryNode->keyPointers[i] = primaryNode->keyPointers[i - 1];
	}

	primaryNode->nodePointers[left_index + 1] = rightNode;
	primaryNode->keyPointers[left_index] = key;
	primaryNode->keyCount++;

	return primaryNode;
}

//this helper is used to intialize the root
void initializeRoot(Node *root, int key, Node *leftNode, Node *rightNode) {
    root->keyPointers[0] = key;
    *(root->nodePointers) = leftNode;
	*(root->nodePointers + 1) = rightNode;
    root->keyCount = 1;
    root->parent = NULL;
    leftNode->parent = rightNode->parent = root;
}

// Creates a new root for two subtrees and inserts the appropriate key into the new root.
Node * addToRoot(BTreeMaster * treeManager, Node * leftNode, int key, Node * rightNode) {

	Node * root = buildNode(treeManager);
	root->keyPointers[0] = key;

	initializeRoot(root, key, leftNode, rightNode);
	
	return root;
}

// Creates a new general node, which can be adapted to serve as either a leaf or an internal node.


//This helper is used to allocate memory to node
Node* allocateNode() {
    Node* node = calloc(1,sizeof(Node));
    if (!node) {
        printf("Node creation.");
        exit(RC_NULL_ARGUMENT);
    }
    return node;
}

//This helper is used to allocate memory to keys
void allocateKeys(Node* node, int bTreeOrder) {
    node->keyPointers = calloc(bTreeOrder - 1, sizeof(Value *));
    if (!node->keyPointers) {
        exit(RC_NULL_ARGUMENT);
    }
}

//This helper is used to allocate memory to pointers
void allocatePointers(Node* node, int bTreeOrder) {
    node->nodePointers = calloc(bTreeOrder, sizeof(void *));
    if (!node->nodePointers) {
        exit(RC_NULL_ARGUMENT);
    }
}

//This helper is used to build a node
Node* buildNode(BTreeMaster* treeManager) {
    // Increment node count
    treeManager->totalNodes++;

    // Get order of B-tree
    int bTreeOrder = treeManager->treeOrder;

    // Allocate memory for new node
    Node* new_node = allocateNode();
    allocateKeys(new_node, bTreeOrder);
    allocatePointers(new_node, bTreeOrder);

    // Initialize node fields
    new_node->isLeafNode = false;
    new_node->keyCount = 0;
    new_node->parent =NULL;

    // Return new node
    return new_node;
}



//This helper is used to search for a given key in all leaf nodes
Node* searchForLeaf(Node* root, Value* key) {

    int keyToSearch = key->v.intV;
    while (!root->isLeafNode) {
        int i;
        for (i = 0; i < root->keyCount; i++) {
            if (keyToSearch < root->keyPointers[i]) {
                break;
            }
        }
        root = (Node*)root->nodePointers[i];
    }

    return root;
}


//This helper is used to find for key in the root node
RecordInfo * searchForRecord(Node * root, Value *key) {
    Node *leafNode = searchForLeaf(root, key);
    int keyToSearch = key->v.intV;

    if (leafNode != NULL) {
       
    for (int i = 0; i < leafNode->keyCount; ++i) {
        if (leafNode->keyPointers[i] != keyToSearch) {
            return NULL;
        }
        else
        {
            return (RecordInfo *)leafNode->nodePointers[i];
        }
    }
    }
    return NULL;
}


//To support multiple datatypes
bool compareValues(Value *key1, Value *key2, int compOp) {
    switch (key1->dt) {
        case DT_INT:
            if (key1->v.intV == key2->v.intV) {
                return compOp == 0;
            } else if (key1->v.intV < key2->v.intV) {
                return compOp == -1;
            } else {
                return compOp == 1;
            }
            break;
        case DT_FLOAT:
            if (key1->v.floatV == key2->v.floatV) {
                return compOp == 0;
            } else if (key1->v.floatV < key2->v.floatV) {
                return compOp == -1;
            } else {
                return compOp == 1;
            }
            break;
        case DT_STRING:
            if (strcmp(key1->v.stringV, key2->v.stringV) == 0) {
                return compOp == 0;
            } else if (strcmp(key1->v.stringV, key2->v.stringV) < 0) {
                return compOp == -1;
            } else {
                return compOp == 1;
            }
            break;
        case DT_BOOL:
            if (key1->v.boolV == key2->v.boolV) {
                return compOp == 0;
            } else {
                return false;
            }
            break;
    }
    // The switch statement covers all possible cases, so this line is unreachable.
    return false;
}
