#ifndef EZ_RBTREE_H
#define EZ_RBTREE_H

#include <stdint.h>

typedef struct ezRBTreeNode_t ezRBTreeNode;
typedef struct ezRBTree_t ezRBTree;
typedef int (*rbTreeNodeCompare)(ezRBTreeNode *newNode, ezRBTreeNode *existNode);
typedef int (*findCompareKey)(ezRBTreeNode *node, void *find_args);

struct ezRBTreeNode_t {
	ezRBTreeNode  *left;
	ezRBTreeNode  *right;
	ezRBTreeNode  *parent;
	uint8_t 	   color;
};

struct ezRBTree_t {
	ezRBTreeNode *root;
	ezRBTreeNode *sentinel;
	rbTreeNodeCompare  node_cmp_proc;
};

void rbtree_init(ezRBTree * tree, ezRBTreeNode * sentinel, rbTreeNodeCompare node_cmp_proc);

void rbtree_insert(ezRBTree * tree, ezRBTreeNode * node);

void rbtree_delete(ezRBTree * tree, ezRBTreeNode * node);

ezRBTreeNode * rbtree_find_node(ezRBTree * tree, findCompareKey find_proc, void * find_args);

ezRBTreeNode * rbtree_min_node(ezRBTree * tree);

#endif /* EZ_RBTREE_H */
