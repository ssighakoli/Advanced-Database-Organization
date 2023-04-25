#include "btree_insertionHelper.h"
#include "btree_deletionHelper.h"
#include "dt.h"
#include "string.h"

//this helper is used to find the key index of the node
int findKeyIndex(Node *node, int key) {
    int i = 0;
    while (node->keyPointers[i] != key && i < node->keyCount) {
        i++;
    }
    return i;
}

//this helper is used to find the pointer index of the node
int findPointerIndex(Node *node, Node *pointer) {
    int i = 0;
    while (node->nodePointers[i] != pointer) {
        i++;
    }
    return i;
}

//This helper to used to shift the keys to left when startIndex is given
void shiftKeysLeft(Node *node, int startIndex) {
    for (int i = startIndex + 1; i < node->keyCount; i++) {
        node->keyPointers[i - 1] = node->keyPointers[i];
    }
}

//This helper to used to shift the pointers to left when startIndex is given
void shiftPointersLeft(Node *node, int startIndex, int num_pointers) {
    for (int i = startIndex + 1; i < num_pointers; i++) {
        node->nodePointers[i - 1] = node->nodePointers[i];
    }
}

//This helper is used to calculate the number of pointers in given  node
int calculateNumPointers(Node *node) {
    return node->keyCount + !node->isLeafNode;
}

//This helper is used to remove a key from the given node
Node *eraseKeyFromNode(BTreeMaster *treeManager, Node *node, int key, Node *pointer) {
    int i, num_pointers;
    int bTreeOrder = treeManager->treeOrder;

    // Remove the key and shift other keys accordingly.
    i = findKeyIndex(node, key);
    shiftKeysLeft(node, i);

   num_pointers = calculateNumPointers(node);

    i = findPointerIndex(node, pointer);
    shiftPointersLeft(node, i, num_pointers);

    // One key fewer.
    node->keyCount =  node->keyCount -1;
    treeManager->totalEntries = treeManager->totalEntries -1;

    // Set the other pointers to NULL for tidiness.
    int nullPointersStartIndex;
    if (node->isLeafNode) {
        nullPointersStartIndex = node->keyCount;
    } else {
        nullPointersStartIndex = node->keyCount + 1;
    }
    for (i = nullPointersStartIndex; i < bTreeOrder; i++) {
        node->nodePointers[i] = NULL;
    }

    return node;
}

//This helper is used to balance the root incase of underflow
Node* balanceRoot(Node* root) {
    if (root->keyCount > 0) {
        // Root is not empty, do nothing and return it
        return root;
    }
    
    // If the root is a leaf, set the new root to NULL. Otherwise, promote the first child as the new root.
    Node* new_root = root->isLeafNode ? NULL : root->nodePointers[0];
    
    if (new_root != NULL) {
        // Set the new root's parent pointer to NULL since it's the new root of the tree.
        new_root->parent = NULL;
    }
    
    // Free up memory space
    free(root);
    
    // Return the new root (which may be NULL if the tree is empty).
    return new_root;
}

//This helper is used to get the min number of keys required in a node to avoid underflow
int get_min_keys(Node* n, int bTreeOrder) {
    int min_keys = (bTreeOrder - 1) / 2;

    if (!n->isLeafNode)
        min_keys++;  // Add 1 for internal nodes

    return min_keys;
}


//This helper is used to find the next node index to help in redistribution
int findAdjacentNodeIndex(Node *n) {
    int k;
    int totalKeys = n->parent->keyCount;
    for (k = 0; k <= totalKeys; k++) {
        if (n->parent->nodePointers[k] == n) {
            return k - 1;
        }
    }
    return -1;
}

//This helper is used to decide whether merging is required or redistribution is required in case of node underflow
Node *handleMergeOrRedistribute(BTreeMaster *treeManager, Node *n, Node *adjacentNode, int adjacentNodeindex, int nextNodeIndex) {
    int capacity = n->isLeafNode ? treeManager->treeOrder : treeManager->treeOrder - 1;
    int shouldMerge = adjacentNode->keyCount + n->keyCount < capacity;

    if (shouldMerge) {
        // Merging
        return combineSiblings(treeManager, n, adjacentNode, adjacentNodeindex, nextNodeIndex);
    } else {
        // Re-distributing
        int redistributionIndex;
        if (adjacentNodeindex == -1) {
            redistributionIndex = 0;
        } else {
            redistributionIndex = adjacentNodeindex;
        }
        return balanceSiblings(treeManager->baseNode, n, adjacentNode, adjacentNodeindex, redistributionIndex, nextNodeIndex);
    }
}

//This method is used to remove a key from the node
Node *eraseKey(BTreeMaster *treeManager, Node *n, int key, void *pointer) {
    n = eraseKeyFromNode(treeManager, n, key, pointer);

    if (n == treeManager->baseNode) {
        return balanceRoot(treeManager->baseNode);
    }

    int min_keys = get_min_keys(n, treeManager->treeOrder);

    // Node stays at or above minimum.
    if (n->keyCount >= min_keys) {
        return treeManager->baseNode;
    }

    int adjacentNodeindex = findAdjacentNodeIndex(n);
    int firstNodeIndex;
    if (adjacentNodeindex == -1) {
        firstNodeIndex = 0;
    } else {
        firstNodeIndex = adjacentNodeindex;
    }
    int nextNodeIndex = n->parent->keyPointers[firstNodeIndex];
    
    Node *adjacentNode;
    if (adjacentNodeindex == -1) {
        adjacentNode = n->parent->nodePointers[1];
    } else {
        adjacentNode = n->parent->nodePointers[adjacentNodeindex];
    }

    return handleMergeOrRedistribute(treeManager, n, adjacentNode, adjacentNodeindex, nextNodeIndex);
}

// Helper function to swap two nodes
void swapNodes(Node **n1, Node **n2) {
    Node *tmp = *n1;
    *n1 = *n2;
    *n2 = tmp;
}

//This helepr is used to merge two non leaf nodes
void mergeNonLeafNodes(Node *n, Node *adjacentNode, int firstNodeIndex, int adjacentNodePos) {
    int i, j, n_end;

    adjacentNode->keyPointers[adjacentNodePos] = firstNodeIndex;
    adjacentNode->keyCount++;

    n_end = n->keyCount;

    i = adjacentNodePos + 1;
    j = 0;
    while (j < n_end) {
        adjacentNode->keyPointers[i] = n->keyPointers[j];
        adjacentNode->nodePointers[i] = n->nodePointers[j];
        adjacentNode->keyCount++;
        n->keyCount--;

        i++;
        j++;
    }

    adjacentNode->nodePointers[i] = n->nodePointers[j];

    i = 0;
    while (i < adjacentNode->keyCount + 1) {
        Node *tmp = (Node *) adjacentNode->nodePointers[i];
        tmp->parent = adjacentNode;
        i++;
    }
}

//This helepr is used to merge two leaf nodes
void mergeLeafNodes(Node *n, Node *adjacentNode, int bTreeOrder, int adjacentNodePos) {
    int i = adjacentNodePos;
    int j = 0;

    if (n->keyCount > 0) {
        do {
            adjacentNode->keyPointers[i] = n->keyPointers[j];
            adjacentNode->nodePointers[i] = n->nodePointers[j];
            adjacentNode->keyCount++;

            i++;
            j++;
        } while (j < n->keyCount);
    }
    
    adjacentNode->nodePointers[bTreeOrder - 1] = n->nodePointers[bTreeOrder - 1];
}

//This helper is used to combine two siblings to avoid underflow
Node *combineSiblings(BTreeMaster *treeManager, Node *n, Node *adjacentNode, int adjacentNodeindex, int nextNodeIndex) {
    int adjacentNodePos;
    int bTreeOrder = treeManager->treeOrder;


    if (adjacentNodeindex == -1) {
        swapNodes(&n, &adjacentNode);
    }

    adjacentNodePos = adjacentNode->keyCount;

    if (!n->isLeafNode) {
        mergeNonLeafNodes(n, adjacentNode, nextNodeIndex, adjacentNodePos);
    } else {
        mergeLeafNodes(n, adjacentNode, bTreeOrder, adjacentNodePos);
    }

    treeManager->baseNode = eraseKey(treeManager, n->parent, nextNodeIndex, n);

    free(n);
    return treeManager->baseNode;
}


// Helper function to move key-pointer pair from left adjacentNode to n
void moveFromLeftAdjacentNode(Node *n, Node *adjacentNode, int firstNodeIndex, int nextNodeIndex) {
    int i;
    Node *tmp;

    if (!n->isLeafNode)
        n->nodePointers[n->keyCount + 1] = n->nodePointers[n->keyCount];

    // Shift keys and pointers in n to the right
    int n_keys_to_shift = n->keyCount;
    memmove(&n->keyPointers[1], &n->keyPointers[0], n_keys_to_shift * sizeof(int));
    memmove(&n->nodePointers[1], &n->nodePointers[0], (n_keys_to_shift + n->isLeafNode) * sizeof(void *));

    // Move the key and pointer from the left adjacentNode to n
    n->nodePointers[0] = adjacentNode->nodePointers[adjacentNode->keyCount - n->isLeafNode];
    n->keyPointers[0] = n->isLeafNode ? adjacentNode->keyPointers[adjacentNode->keyCount - 1] : nextNodeIndex;

    // Update the parent's key
    n->parent->keyPointers[firstNodeIndex] = n->isLeafNode ? n->keyPointers[0] : adjacentNode->keyPointers[adjacentNode->keyCount - 1];

    if (!n->isLeafNode) {
        tmp = (Node *)n->nodePointers[0];
        tmp->parent = n;
        adjacentNode->nodePointers[adjacentNode->keyCount] = NULL;
    } else {
        adjacentNode->nodePointers[adjacentNode->keyCount - 1] = NULL;
    }
}

// Helper function to move key-pointer pair from right adjacentNode to n
void moveFromRightAdjacentNode(Node *n, Node *adjacentNode, int firstNodeIndex, int nextNodeIndex) {
    int i;
    Node *tmp;

    // Move the key and pointer from the right adjacentNode to n
    n->keyPointers[n->keyCount] = n->isLeafNode ? adjacentNode->keyPointers[0] : nextNodeIndex;
    n->nodePointers[n->keyCount + n->isLeafNode] = adjacentNode->nodePointers[0];
    
    if (!n->isLeafNode) {
        tmp = (Node *)n->nodePointers[n->keyCount + 1];
        tmp->parent = n;
    }
    
    // Update the parent's key
    n->parent->keyPointers[firstNodeIndex] = n->isLeafNode ? adjacentNode->keyPointers[1] : adjacentNode->keyPointers[0];

    // Shift adjacentNode's keys and pointers to the left
    int keys_to_shift = adjacentNode->keyCount - 1;
    memmove(&adjacentNode->keyPointers[0], &adjacentNode->keyPointers[1], keys_to_shift * sizeof(int));
    memmove(&adjacentNode->nodePointers[0], &adjacentNode->nodePointers[1], (keys_to_shift + !n->isLeafNode) * sizeof(void *));
}

//This helper is used to balance siblings in case of udnerflow.
Node *balanceSiblings(Node *root, Node *n, Node *adjacentNode, int adjacentNodeindex, int firstNodeIndex, int nextNodeIndex) {
    if (adjacentNodeindex != -1) {
        moveFromLeftAdjacentNode(n, adjacentNode, firstNodeIndex, nextNodeIndex);
    } else {
        moveFromRightAdjacentNode(n, adjacentNode, firstNodeIndex, nextNodeIndex);
    }

    // n now has one more key and one more pointer; the adjacentNode has one fewer of each.
    n->keyCount++;
    adjacentNode->keyCount--;
    return root;
}

//This method is used to discard a key from the given btree master
Node* discardKey(BTreeMaster* treeManager, Value* key) {

    // Find the record associated with the given key in the tree
    Node* recordNode = searchForRecord(treeManager->baseNode, key);
    if (recordNode == NULL) {
        return treeManager->baseNode;
    }


    // Find the leaf node that contains the record associated with the given key
    RecordInfo* keyRecordInfo = searchForLeaf(treeManager->baseNode, key);
    if (keyRecordInfo == NULL) {
        free(recordNode);
        return treeManager->baseNode;
    }
    treeManager->baseNode = eraseKey(treeManager, keyRecordInfo, key->v.intV, recordNode);

    // Free the memory allocated for the record node
    free(recordNode);

    return treeManager->baseNode;
}
