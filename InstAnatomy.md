Instructions are C structs of type `struct inst`. They have three members:
```
struct inst {                   /* instruction */
    short   type;
    short   line;
    union {
        ....
    } data
}
```

`type` is the most important member. It contains contains the interpreter hint for how the `data` member should be accessed and interacted with. Accessing members of `data` that do not correspond to the current `type` will return uninitialized data and is broken behavior.

## Static Types ##
These types do not contain pointers. This is not to say that the `inst` won't contain a hint to something that has to be dereferenced, but in such a case the memory will be freed in such a way that doesn't require tracking a link count.

These types do not have their own pages, and documentation can be found on the StaticInst page.

| **Type**       | **#** | **Description** |
|:---------------|:------|:----------------|
| PROG\_CLEARED  |   0   | Contains no initialized data at all. The result of using `CLEAR()` on an inst. |
| PROG\_INTEGER  |   2   | data.number contains a standard C int. |
| PROG\_FLOAT    |   3   | data.number contains a standard C long. |
| PROG\_OBJECT   |   4   | data.objref contains a `dbref` type, which is a `typedef`'d C integer. |
| PROG\_VAR      |   5   |                 |
| PROG\_LVAR     |   6   |                 |
| PROG\_SVAR     |   7   |                 |


## Pointer Types ##
These data types contain memory pointers and maintain a link count. Passing the `inst` through `CLEAR()` will reduce the link count by 1; when the link count reaches zero, the referenced memory will be freed. Failure to pass the `inst` through `CLEAR()` before a primitive finishes will result in memory leaks.

You should always try to `dup` these types on the stack wherever possible for maximum efficiency.

| **Type**          | **#** | **Wiki Page**    | **Description** |
|:------------------|:------|:-----------------|:----------------|
| PROG\_STRING      |   9   | ProgString       | data.string->data contains a pointer to a C string. |
| PROG\_FUNCTION    |  10   | ProgFunction     |                 |
| PROG\_LOCK        |  11   | ProgLock         |                 |
| PROG\_ADD         |  12   | ProgAdd          | data.addr contains a pointer to a MUF function. |
| PROG\_ARRAY       |  16   | ProgArray        |                 |
| PROG\_DICTIONARY  |  17   | ProgDictionary   |                 |
| PROG\_SOCKET      |  18   | ProgSocket       | data.muf\_socket contains a pointer to a MUF socket. |
| PROG\_MYSQL       |  27   | ProgMysql        |                 |