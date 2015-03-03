#ifndef EZ_RBTREE_H
#define EZ_RBTREE_H

#include <stdint.h>

typedef int rbtree_key_t;
typedef void *rbtree_value_t;

typedef struct ezRBTreeNode_t ezRBTreeNode;
typedef struct ezRBTree_t ezRBTree;

struct ezRBTreeNode_t {
	rbtree_key_t   key;
	rbtree_value_t value;

	ezRBTreeNode  *left;
	ezRBTreeNode  *right;
	ezRBTreeNode  *parent;
	uint8_t 	   color;
};

struct ezRBTree_t {
	ezRBTreeNode *root;
	ezRBTreeNode *sentinel;
};

void rbtree_init(ezRBTree * tree, ezRBTreeNode * sentinel);

void rbtree_insert(ezRBTree * tree, ezRBTreeNode * node);

void rbtree_delete(ezRBTree * tree, ezRBTreeNode * node);

ezRBTreeNode * rbtree_find_node(ezRBTree * tree, rbtree_key_t key);

rbtree_value_t rbtree_find_value(ezRBTree * tree, rbtree_key_t key);

ezRBTreeNode * rbtree_min_node(ezRBTree * tree);

#endif /* EZ_RBTREE_H */
