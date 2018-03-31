#include <stdint.h>
#include <stddef.h>

#include "ez_rbtree.h"

#define rbt_red(node)               ((node)->color = 1)
#define rbt_black(node)             ((node)->color = 0)
#define rbt_is_red(node)            ((node)->color)
#define rbt_is_black(node)          (!rbt_is_red(node))
#define rbt_copy_color(n1, n2)      (n1->color = n2->color)

void rbtree_init(ezRBTree *tree, ezRBTreeNode *sentinel, rbTreeNodeCompare node_cmp_proc)
{
    /* a sentinel must be black */
    rbt_black(sentinel);
    sentinel->parent = NULL;
    sentinel->left = NULL;
    sentinel->right = NULL;

    tree->root = sentinel;
    tree->sentinel = sentinel;
    tree->node_cmp_proc = node_cmp_proc;
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

ezRBTreeNode * rbtree_find_node(ezRBTree * tree, findCompareKey find_proc, void * find_args)
{
    ezRBTreeNode *node, *sentinel ;
    node = tree->root;
    sentinel = tree->sentinel;
    if (node == sentinel) return NULL;

    do {
        int r = find_proc(node, find_args);
        if (r == 0)
            return node;
        node = (r < 0) ? node->right : node->left;
    } while (node != sentinel);

    return NULL;
}

// tree, *root, node, sentinel
static void default_rbtree_insert_value(ezRBTree * tree, ezRBTreeNode *begin, ezRBTreeNode *node, ezRBTreeNode *sentinel) {
    ezRBTreeNode **p;

    for (; ;) {
        p = (tree->node_cmp_proc(node, begin) < 0) ? &begin->left : &begin->right;
        if (*p == sentinel) {
            break;
        }
        begin = *p;
    }

    *p = node;
    node->parent = begin;
    node->left = sentinel;
    node->right = sentinel;
    rbt_red(node); // 新节点, 直接为red.
}

void rbtree_insert(ezRBTree *tree, ezRBTreeNode *node) {
    ezRBTreeNode **root, *temp, *sentinel;
    // 1.节点非红即黑。
    // 2.根节点是黑色。
    // 3.所有NULL结点称为叶子节点，且认为颜色为黑
    // 4.所有红节点的子节点都为黑色。
    // 5.从任一节点到其叶子节点的所有路径上都包含相同数目的黑节点。
    /* a binary tree insert */
    root = (ezRBTreeNode **) &tree->root;
    sentinel = tree->sentinel;

    if (*root == sentinel) {
        node->parent = NULL;
        node->left = sentinel;
        node->right = sentinel;
        rbt_black(node); // insert root, set black.
        *root = node;

        return;
    }

    default_rbtree_insert_value(tree, *root, node, sentinel); // 新节点，直接设为 red.

    /* re-balance tree */
    while (node != *root && rbt_is_red(node->parent)) {       // 新节点&父节点都是red，就要进行 re-balance tree.

        if (node->parent == node->parent->parent->left) {
            temp = node->parent->parent->right;               // 叔节点

            if (rbt_is_red(temp)) {                           // 叔节点是red
                rbt_black(node->parent);                      // 父节点,叔节点都设为 black.
                rbt_black(temp);
                rbt_red(node->parent->parent);                // 祖父节点，设为red. 继续.
                node = node->parent->parent;

            } else {                                          // 叔节点是 black.
                if (node == node->parent->right) {
                    node = node->parent;
                    rbtree_left_rotate(root, sentinel, node); // 左旋，祖节点作为父节点的右儿子，父节点的右儿子写到祖节点左儿子.
                }

                rbt_black(node->parent);                      // node是red, parent是black
                rbt_red(node->parent->parent);                // parent.parent是red
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
