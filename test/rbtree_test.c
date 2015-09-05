#include <stdio.h>
#include <ez_rbtree.h>
#include <ez_util.h>

#define NODES 100

#define cast_to_type_ptr(ptr, type)     EZ_CONTAINER_OF(ptr, type, rb_node)
#define rb_entry(type_ptr)          (&((type_ptr)->rb_node))
#define RB_TREE_NODE                ezRBTreeNode rb_node

typedef struct student {
    RB_TREE_NODE;
    int age;
} student;

static ezRBTree rbtree;
static ezRBTreeNode sentinel;

static student nodes[NODES];

static int student_compare_proc(ezRBTreeNode *new_node, ezRBTreeNode *exists_node)
{
    student *n = cast_to_type_ptr(new_node, student);
    student *o = cast_to_type_ptr(exists_node, student);
    return n->age == o->age ? 0 : (n->age > o->age ? 1 : -1);
}

static int student_find_proc(ezRBTreeNode *node, void *find_args)
{
    int age = *((int *) find_args);
    student *o = cast_to_type_ptr(node, student);
    return o->age == age ? 0 : (o->age > age ? 1 : -1);
}

int main(int argc, char **argv) {
    ezRBTreeNode *node = NULL;
    rbtree_init(&rbtree, &sentinel, student_compare_proc);

    for (int i = 0; i < NODES; ++i) {
        nodes[i].age = i;
    }

    for (int i = 0; i < NODES; ++i) {
        rbtree_insert(&rbtree, rb_entry(&nodes[i]));
    }
    int find_age = 100;
    node = rbtree_find_node(&rbtree, student_find_proc, &find_age);
    if (node != NULL) {
        fprintf(stdout, "error:::>find student 100 :>!\n");
    }
    find_age = 50;
    node = rbtree_find_node(&rbtree, student_find_proc, &find_age);
    if (node == NULL || cast_to_type_ptr(node, student)->age != find_age) {
        fprintf(stdout, "error::>not find student 50 :>!\n");
    }
    node = rbtree_min_node(&rbtree);
    while (node != NULL) {
        fprintf(stdout, "delete min student.age = %d\n", cast_to_type_ptr(node, student)->age);
        rbtree_delete(&rbtree, node);
        node = rbtree_min_node(&rbtree);
    }
    return 0;
}
