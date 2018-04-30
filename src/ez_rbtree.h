#ifndef EZ_RBTREE_H
#define EZ_RBTREE_H

#include <stdint.h>

typedef struct ez_rbtree_node_s ez_rbtree_node;
typedef struct ez_rbtree_s ez_rbtree_t;
typedef int (*rbTreeNodeCompare)(ez_rbtree_node *newNode, ez_rbtree_node *existNode);
typedef int (*findCompareKey)(ez_rbtree_node *node, void *find_args);

struct ez_rbtree_node_s {
	ez_rbtree_node  *left;
	ez_rbtree_node  *right;
	ez_rbtree_node  *parent;
	uint8_t 	   color;
};

struct ez_rbtree_s {
	ez_rbtree_node *root;
	ez_rbtree_node *sentinel;
	rbTreeNodeCompare  node_cmp_proc;
};

void rbtree_init(ez_rbtree_t * tree, ez_rbtree_node * sentinel, rbTreeNodeCompare node_cmp_proc);

void rbtree_insert(ez_rbtree_t * tree, ez_rbtree_node * node);

void rbtree_delete(ez_rbtree_t * tree, ez_rbtree_node * node);

ez_rbtree_node * rbtree_find_node(ez_rbtree_t * tree, findCompareKey find_proc, void * find_args);

ez_rbtree_node * rbtree_min_node(ez_rbtree_t * tree);

#endif /* EZ_RBTREE_H */
