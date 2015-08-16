
```
struct inst {                   /* instruction */
    short   type;
    short   line;
    union {
        ....
        struct stk_array_t *array;      /* FB6 style array                  */
        ....
    } data;
};

typedef struct stk_array_t {
        int links;                  /* number of pointers  to array */
        int items;                  /* number of items in array */
        short type;                 /* type of array */
        union {
                array_data *packed; /* pointer to packed array */
                array_tree *dict;   /* pointer to dictionary AVL tree */
        } data;
} stk_array;

typedef struct inst array_data;
typedef struct inst array_iter;

typedef struct array_tree_t {
        struct array_tree_t *left;
        struct array_tree_t *right;
        array_iter key;
        array_data data;
        int height;
} array_tree;

```