#include <stdint.h>
#include <stddef.h>

#include "ez_rbtree.h"

#define rbt_red(node)               ((node)->color = 1)
#define rbt_black(node)             ((node)->color = 0)
#define rbt_is_red(node)            ((node)->color)
#define rbt_is_black(node)          (!rbt_is_red(node))
#define rbt_copy_color(n1, n2)      (n1->color = n2->color)

void rbtree_init(ezRBTree *tree, ezRBTreeNode *sentinel)
{
    /* a sentinel must be black */
    rbt_black(sentinel);
    sentinel->parent = NULL;
    sentinel->left = NULL;
    sentinel->right = NULL;
    sentinel->key = 0;
    sentinel->value = NULL;

    tree->root = sentinel;
    tree->sentinel = sentinel;
}

static ezRBTreeNode * rbtree_min(ezRBTreeNode *node, ezRBTreeNode *sentinel);

ezRBTreeNode *rbtree_min_node(ezRBTree *tree) {
    return rbtree_min(tree->root, tree->sentinel);
}

static void rbtree_left_rotate(ezRBTreeNode **root, ezRBTreeNode *sentinel, ezRBTreeNode *node) {
    ezRBTreeNode *temp;

    temp = node->right;
    node->right = temp->left;

    if (temp->left != sentinel) {
        temp->left->parent = node;
    }

    temp->parent = node->parent;

    if (node == *root) {
        *root = temp;

    } else if (node == node->parent->left) {
        node->parent->left = temp;

    } else {
        node->parent->right = temp;
    }

    temp->left = node;
    node->parent = temp;
}

static void rbtree_right_rotate(ezRBTreeNode **root, ezRBTreeNode *sentinel, ezRBTreeNode *node) {
    ezRBTreeNode *temp;

    temp = node->left;
    node->left = temp->right;

    if (temp->right != sentinel) {
        temp->right->parent = node;
    }

    temp->parent = node->parent;

    if (node == *root) {
        *root = temp;

    } else if (node == node->parent->right) {
        node->parent->right = temp;

    } else {
        node->parent->left = temp;
    }

    temp->right = node;
    node->parent = temp;
}

static inline ezRBTreeNode * rbtree_min(ezRBTreeNode *node, ezRBTreeNode *sentinel) {
    while (node != NULL && node->left != sentinel) {
        node = node->left;
    }
    return node;
}

ezRBTreeNode * rbtree_find_node(ezRBTree * tree, rbtree_key_t key)
{
    ezRBTreeNode *node = tree->root;
    ezRBTreeNode *sentinel = tree->sentinel;
    if (node == sentinel) {
        return NULL;
    }
    do {
        if (key == node->key)
            return node;
        node = (key < node->key) ? node->left : node->right;
    } while (node != NULL);

    return node;
}

rbtree_value_t rbtree_find_value(ezRBTree * tree, rbtree_key_t key)
{
    ezRBTreeNode * node = rbtree_find_node(tree, key);
    return node == NULL ? NULL : node->value;
}

static void default_rbtree_insert_value(ezRBTreeNode *temp, ezRBTreeNode *node, ezRBTreeNode *sentinel) {
    ezRBTreeNode **p;

    for (; ;) {
        p = (node->key < temp->key) ? &temp->left : &temp->right;
        if (*p == sentinel) {
            break;
        }
        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    rbt_red(node);
}

void rbtree_insert(ezRBTree *tree, ezRBTreeNode *node) {
    ezRBTreeNode **root, *temp, *sentinel;

    /* a binary tree insert */
    root = (ezRBTreeNode **) &tree->root;
    sentinel = tree->sentinel;

    if (*root == sentinel) {
        node->parent = NULL;
        node->left = sentinel;
        node->right = sentinel;
        rbt_black(node);
        *root = node;

        return;
    }

    default_rbtree_insert_value(*root, node, sentinel);

    /* re-balance tree */
    while (node != *root && rbt_is_red(node->parent)) {

        if (node->parent == node->parent->parent->left) {
            temp = node->parent->parent->right;

            if (rbt_is_red(temp)) {
                rbt_black(node->parent);
                rbt_black(temp);
                rbt_red(node->parent->parent);
                node = node->parent->parent;

            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    rbtree_left_rotate(root, sentinel, node);
                }

                rbt_black(node->parent);
                rbt_red(node->parent->parent);
                rbtree_right_rotate(root, sentinel, node->parent->parent);
            }

        } else {
            temp = node->parent->parent->left;

            if (rbt_is_red(temp)) {
                rbt_black(node->parent);
                rbt_black(temp);
                rbt_red(node->parent->parent);
                node = node->parent->parent;

            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    rbtree_right_rotate(root, sentinel, node);
                }

                rbt_black(node->parent);
                rbt_red(node->parent->parent);
                rbtree_left_rotate(root, sentinel, node->parent->parent);
            }
        }
    }

    rbt_black(*root);
}

void rbtree_delete(ezRBTree *tree, ezRBTreeNode *node) {
    uint8_t red;
    ezRBTreeNode **root, *sentinel, *subst, *temp, *w;

    /* a binary tree delete */
    root = (ezRBTreeNode **) &tree->root;
    sentinel = tree->sentinel;

    if (node->left == sentinel) {
        temp = node->right;
        subst = node;

    } else if (node->right == sentinel) {
        temp = node->left;
        subst = node;

    } else {
        subst = rbtree_min(node->right, sentinel);

        if (subst->left != sentinel) {
            temp = subst->left;
        } else {
            temp = subst->right;
        }
    }

    if (subst == *root) {
        *root = temp;
        rbt_black(temp);

        /* DEBUG stuff */
        node->left = NULL;
        node->right = NULL;
        node->parent = NULL;
        node->key = 0;

        return;
    }

    red = rbt_is_red(subst);

    if (subst == subst->parent->left) {
        subst->parent->left = temp;
    } else {
        subst->parent->right = temp;
    }

    if (subst == node) {

        temp->parent = subst->parent;

    } else {

        if (subst->parent == node) {
            temp->parent = subst;

        } else {
            temp->parent = subst->parent;
        }

        subst->left = node->left;
        subst->right = node->right;
        subst->parent = node->parent;
        rbt_copy_color(subst, node);

        if (node == *root) {
            *root = subst;

        } else {
            if (node == node->parent->left) {
                node->parent->left = subst;
            } else {
                node->parent->right = subst;
            }
        }

        if (subst->left != sentinel) {
            subst->left->parent = subst;
        }

        if (subst->right != sentinel) {
            subst->right->parent = subst;
        }
    }

    /* DEBUG stuff */
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->key = 0;

    if (red) {
        return;
    }

    /* a delete fixup */
    while (temp != *root && rbt_is_black(temp)) {

        if (temp == temp->parent->left) {
            w = temp->parent->right;

            if (rbt_is_red(w)) {
                rbt_black(w);
                rbt_red(temp->parent);
                rbtree_left_rotate(root, sentinel, temp->parent);
                w = temp->parent->right;
            }

            if (rbt_is_black(w->left) && rbt_is_black(w->right)) {
                rbt_red(w);
                temp = temp->parent;

            } else {
                if (rbt_is_black(w->right)) {
                    rbt_black(w->left);
                    rbt_red(w);
                    rbtree_right_rotate(root, sentinel, w);
                    w = temp->parent->right;
                }

                rbt_copy_color(w, temp->parent);
                rbt_black(temp->parent);
                rbt_black(w->right);
                rbtree_left_rotate(root, sentinel, temp->parent);
                temp = *root;
            }

        } else {
            w = temp->parent->left;

            if (rbt_is_red(w)) {
                rbt_black(w);
                rbt_red(temp->parent);
                rbtree_right_rotate(root, sentinel, temp->parent);
                w = temp->parent->left;
            }

            if (rbt_is_black(w->left) && rbt_is_black(w->right)) {
                rbt_red(w);
                temp = temp->parent;

            } else {
                if (rbt_is_black(w->left)) {
                    rbt_black(w->right);
                    rbt_red(w);
                    rbtree_left_rotate(root, sentinel, w);
                    w = temp->parent->left;
                }

                rbt_copy_color(w, temp->parent);
                rbt_black(temp->parent);
                rbt_black(w->left);
                rbtree_right_rotate(root, sentinel, temp->parent);
                temp = *root;
            }
        }
    }

    rbt_black(temp);
}
