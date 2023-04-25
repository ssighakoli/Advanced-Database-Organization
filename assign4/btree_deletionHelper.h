#ifndef BTREE_DELETIONHELPER_H
#define BTREE_DELETIONHELPER_H


#include "btree_mgr.h"
#include "buffer_mgr.h"
#include "btree_insertionHelper.h"



int findKeyIndex(Node *n, int key);
int findPointerIndex(Node *n, Node *pointer);


void shiftKeysLeft(Node *n, int startIndex) ;
void shiftPointersLeft(Node *n, int startIndex, int num_pointers);
Node * balanceRoot(Node * root);

int findAdjacentNodeIndex(Node *n);

Node *handleMergeOrRedistribute(BTreeMaster *treeManager, Node *n, Node *adjacentNode, int adjacentNodeindex, int nextNodeIndex);
void swapNodes(Node **n1, Node **n2);


void mergeNonLeafNodes(Node *n, Node *adjacentNode, int firstNodeIndex, int adjacentNodePos);
void mergeLeafNodes(Node *n, Node *adjacentNode, int bTreeOrder, int adjacentNodePos);
Node * combineSiblings(BTreeMaster * treeManager, Node * n, Node * adjacentNode, int adjacentNodeindex, int nextNodeIndex);


void moveFromRightAdjacentNode(Node *n, Node *adjacentNode, int firstNodeIndex, int nextNodeIndex);
void moveFromLeftAdjacentNode(Node *n, Node *adjacentNode, int firstNodeIndex, int nextNodeIndex);
Node * balanceSiblings(Node * root, Node * n, Node * adjacentNode, int adjacentNodeindex, int firstNodeIndex, int nextNodeIndex);

int get_min_keys(Node* n, int bTreeOrder);
Node * eraseKey(BTreeMaster * treeManager, Node * n, int key, void * pointer);

Node * discardKey(BTreeMaster * treeManager, Value * key);

Node * eraseKeyFromNode(BTreeMaster * treeManager, Node * n, int key, Node * pointer);



#endif 
