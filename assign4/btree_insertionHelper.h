#ifndef BTREE_INSERTIONHELPER_H
#define BTREE_INSERTIONHELPER_H

#include "btree_mgr.h"
#include "buffer_mgr.h"

typedef struct RecordInfo {
	RID recordId;
} RecordInfo;


typedef struct Node {
	int treeLevel;
	void ** nodePointers;
	Value ** keyPointers;
	struct Node * parent;
	bool isLeafNode;
	bool isRootNode;
	bool isNodeFull;
	int keyCount;
} Node;


typedef struct BTreeMaster {
	BM_BufferPool bufferPool;
	int treeOrder;
	int totalNodes;
	int totalEntries;
	Node * baseNode;
	DataType keyType;
	int informationArray[7][3];
} BTreeMaster;


typedef struct BTreeScanner {
	Node * node;
	BTreeMaster btreeMaster;
	int searchKey;
} BTreeScanner;


Node * searchForLeaf(Node * root, Value * key);
RecordInfo * searchForRecord(Node * root, Value * key);

int findInsertionIndex(Node *leaf, int key, int bTreeOrder);
int find_left_child_position(Node *primaryNode, Node *leftNode);

Node * constructBTree(BTreeMaster * treeManager, Value * key, RecordInfo * pointer);

Node* allocateNode();
void allocatePointers(Node* node, int bTreeOrder);
void allocateKeys(Node* node, int bTreeOrder);

Node * buildNode(BTreeMaster * treeManager);

Node * divideLeafAndInsert(BTreeMaster * treeManager, Node * leafNode, Value* key, RecordInfo * recordPointer);
Node * enterKeyValuePair(Node * primaryNode, int left_index, int key, Node * rightNode);

Node * divideNodeAndInsert(BTreeMaster * treeManager, Node * parent, int left_index, int key, Node * rightNode);
Node * updateParentAfterSplit(BTreeMaster * treeManager, Node * leftNode, int key, Node * rightNode);

void initializeRoot(Node *root, int key, Node *leftNode, Node *rightNode);
Node * addToRoot(BTreeMaster * treeManager, Node * leftNode, int key, Node * rightNode);


extern char* printNode(Node *n);
void printNodeInfo(Node *node);

bool compareValues(Value *key1, Value *key2, int compOp);

#endif 
