#include <stdlib.h>

struct node {
  struct node *left;
  struct node *right;
  void *key;
  int data;
};

typedef int t_compare_func(const void *, const void *);

const struct node *tree_search (const struct node *root, const void *keyy,
                                t_compare_func *comp)
{
  int result;
  const struct node *cur_item = root;

  while (cur_item != NULL) {
      result = (*comp)(cur_item->key, keyy);
      if (result == 0)
         break;
      else if (result > 0)
         cur_item = cur_item->left;
      else
         cur_item = cur_item->right;
  }
  return cur_item;
}
