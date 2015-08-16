
```
struct inst {                   /* instruction */
    short   type;
    short   line;
    union {
        ....
        int     number;                     /* used for both primitives and integers */
        double  fnumber;                    /* used for float storage           */
        dbref   objref;                     /* object reference                 */
        ....
    } data;
};

typedef int dbref;              /* offset into db */
```


Static Types are special. Rather than storing a pointer to memory that is shared between copied instances of the same `struct inst`, their allocations are stored statically inside of the `data` union. This is due to their small storage size; there is little to be gained by giving them their own memory space via `alloc`. This also means that there isn't any inherent memory efficiency to be gained by duplicating them on the stack, but the cost of copying them is trivial in all respects.

## PROG\_INTEGER ##
`data.number` contains a signed C integer. On most systems this will range from -2147483648 to 2147483647.

## PROG\_FLOAT ##
`data.fnumber` contains a signed C float. On most systems this will range from 1e-999 to 9.999999999999999e999. Errors resulting from floating point math must be manually detected via the `ERROR?` primitive.

## PROG\_OBJECT ##
`data.objref` contains a `dbref` type, which is a typedef for a standard C integer. It inherits the same storage properties as PROG\_INTEGER.

_TODO: Explain how PROG\_VAR, PROG\_LVAR, and PROG\_SVAR use `data.number` to track variables._