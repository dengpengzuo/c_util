#include <stdio.h>

#include <ez_rbtree.h>

#define NODES 100

static ezRBTree rbtree;
static ezRBTreeNode sentinel;
static ezRBTreeNode nodes[NODES];

int main(int argc, char **argv) {
    ezRBTreeNode *node = NULL;
    rbtree_init(&rbtree, &sentinel);

    for (int i = 0; i < NODES; ++i) {
        nodes[i].key = i;
    }

    for (int i = 0; i < NODES; ++i) {
        rbtree_insert(&rbtree, &nodes[i]);
    }
    node = rbtree_find_node(&rbtree, 100);
    if (node != NULL) {
        fprintf(stdout, "find student 100 :>!\n");
    }
    node = rbtree_find_node(&rbtree, 50);
    if (node == NULL || node->key != 50) {
        fprintf(stdout, "find student 50 :>!\n");
    }
    node = rbtree_min_node(&rbtree);
    while (node != NULL) {
        fprintf(stdout, "delete min student.age = %d\n", node->key);
        rbtree_delete(&rbtree, node);
        node = rbtree_min_node(&rbtree);
    }
    return 0;
}
