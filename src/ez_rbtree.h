#ifndef EZ_RBTREE_H
#define EZ_RBTREE_H

#include <stdint.h>

typedef struct ez_rbtree_node_s ez_rbtree_node_t;
typedef struct ez_rbtree_s      ez_rbtree_t;

typedef int  (*rbTreeNodeCompare)(ez_rbtree_node_t *newNode, ez_rbtree_node_t *existNode);
typedef int  (*findCompareKey)   (ez_rbtree_node_t *node, void *find_args);
typedef void (*rbtree_node_proc) (ez_rbtree_node_t *node);

struct ez_rbtree_node_s {
	ez_rbtree_node_t  *left;
	ez_rbtree_node_t  *right;
	ez_rbtree_node_t  *parent;
	uint8_t 	       color;
};

struct ez_rbtree_s {
	ez_rbtree_node_t  *root;
	ez_rbtree_node_t  *sentinel;
	rbTreeNodeCompare  node_cmp_proc;
};

void rbtree_init(ez_rbtree_t * tree, ez_rbtree_node_t * sentinel, rbTreeNodeCompare node_cmp_proc);

void rbtree_insert(ez_rbtree_t * tree, ez_rbtree_node_t * node);

void rbtree_delete(ez_rbtree_t * tree, ez_rbtree_node_t * node);

ez_rbtree_node_t * rbtree_find_node(ez_rbtree_t * tree, findCompareKey find_proc, void * find_args);

ez_rbtree_node_t * rbtree_min_node(ez_rbtree_t * tree);

void rbtree_foreach(ez_rbtree_t * tree, rbtree_node_proc proc);

#endif /* EZ_RBTREE_H */
