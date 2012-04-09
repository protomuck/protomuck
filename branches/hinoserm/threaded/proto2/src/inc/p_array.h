extern void prim_array_make(PRIM_PROTOTYPE);
extern void prim_array_make_dict(PRIM_PROTOTYPE);
extern void prim_array_explode(PRIM_PROTOTYPE);
extern void prim_array_vals(PRIM_PROTOTYPE);
extern void prim_array_keys(PRIM_PROTOTYPE);
extern void prim_array_count(PRIM_PROTOTYPE);
extern void prim_array_first(PRIM_PROTOTYPE);
extern void prim_array_last(PRIM_PROTOTYPE);
extern void prim_array_next(PRIM_PROTOTYPE);
extern void prim_array_prev(PRIM_PROTOTYPE);
extern void prim_array_getitem(PRIM_PROTOTYPE);
extern void prim_array_setitem(PRIM_PROTOTYPE);
extern void prim_array_appenditem(PRIM_PROTOTYPE);
extern void prim_array_insertitem(PRIM_PROTOTYPE);
extern void prim_array_getrange(PRIM_PROTOTYPE);
extern void prim_array_setrange(PRIM_PROTOTYPE);
extern void prim_array_insertrange(PRIM_PROTOTYPE);
extern void prim_array_delitem(PRIM_PROTOTYPE);
extern void prim_array_delrange(PRIM_PROTOTYPE);
extern void prim_array_findval(PRIM_PROTOTYPE);
extern void prim_array_excludeval(PRIM_PROTOTYPE);
extern void prim_array_sort(PRIM_PROTOTYPE);
extern void prim_array_sort_indexed(PRIM_PROTOTYPE);
extern void prim_array_join(PRIM_PROTOTYPE);
extern void prim_array_matchkey(PRIM_PROTOTYPE);
extern void prim_array_matchval(PRIM_PROTOTYPE);
extern void prim_array_extract(PRIM_PROTOTYPE);

extern void prim_array_n_union(PRIM_PROTOTYPE);
extern void prim_array_n_intersection(PRIM_PROTOTYPE);
extern void prim_array_n_difference(PRIM_PROTOTYPE);

extern void prim_array_notify(PRIM_PROTOTYPE);
extern void prim_array_ansi_notify(PRIM_PROTOTYPE);
extern void prim_array_reverse(PRIM_PROTOTYPE);

extern void prim_array_get_propdirs(PRIM_PROTOTYPE);
extern void prim_array_get_propvals(PRIM_PROTOTYPE);
extern void prim_array_get_proplist(PRIM_PROTOTYPE);
extern void prim_array_put_propvals(PRIM_PROTOTYPE);
extern void prim_array_put_proplist(PRIM_PROTOTYPE);

extern void prim_array_get_reflist(PRIM_PROTOTYPE);
extern void prim_array_put_reflist(PRIM_PROTOTYPE);
extern void prim_explode_array(PRIM_PROTOTYPE);
extern void prim_array_cut(PRIM_PROTOTYPE);
extern void prim_array_compare(PRIM_PROTOTYPE);
extern void prim_array_interpret(PRIM_PROTOTYPE);

extern void prim_array_nested_get(PRIM_PROTOTYPE);
extern void prim_array_nested_set(PRIM_PROTOTYPE);
extern void prim_array_nested_del(PRIM_PROTOTYPE);

extern void prim_array_filter_flags(PRIM_PROTOTYPE);
extern void prim_array_sum(PRIM_PROTOTYPE);
extern void prim_array_string_fragment(PRIM_PROTOTYPE);

#define PRIMLIST_ARRAY  { "ARRAY_MAKE",         LM1, 1, prim_array_make },            \
                        { "ARRAY_MAKE_DICT",    LM1, 1, prim_array_make_dict },       \
                        { "ARRAY_EXPLODE",      LM1, 1, prim_array_explode },         \
                        { "ARRAY_VALS",         LM1, 1, prim_array_vals },            \
                        { "ARRAY_KEYS",         LM1, 1, prim_array_keys },            \
                        { "ARRAY_FIRST",        LM1, 1, prim_array_first },           \
                        { "ARRAY_LAST",         LM1, 1, prim_array_last },            \
                        { "ARRAY_NEXT",         LM1, 2, prim_array_next },            \
                        { "ARRAY_PREV",         LM1, 2, prim_array_prev },            \
                        { "ARRAY_COUNT",        LM1, 1, prim_array_count },           \
                        { "ARRAY_GETITEM",      LM1, 2, prim_array_getitem },         \
                        { "ARRAY_SETITEM",      LM1, 3, prim_array_setitem },         \
                        { "ARRAY_INSERTITEM",   LM1, 3, prim_array_insertitem },      \
                        { "ARRAY_GETRANGE",     LM1, 3, prim_array_getrange },        \
                        { "ARRAY_SETRANGE",     LM1, 3, prim_array_setrange },        \
                        { "ARRAY_INSERTRANGE",  LM1, 3, prim_array_insertrange },     \
                        { "ARRAY_DELITEM",      LM1, 2, prim_array_delitem },         \
                        { "ARRAY_DELRANGE",     LM1, 3, prim_array_delrange },        \
                        { "ARRAY_NUNION",       LM1, 1, prim_array_n_union },         \
                        { "ARRAY_NINTERSECT",   LM1, 1, prim_array_n_intersection },  \
                        { "ARRAY_NDIFF",        LM1, 1, prim_array_n_difference },    \
                        { "ARRAY_NOTIFY",       LM1, 2, prim_array_notify },          \
                        { "ARRAY_REVERSE",      LM1, 1, prim_array_reverse },         \
                        { "ARRAY_GET_PROPVALS", LM3, 2, prim_array_get_propvals },    \
                        { "ARRAY_PUT_PROPVALS", LM1, 3, prim_array_put_propvals },    \
                        { "ARRAY_GET_PROPDIRS", LM3, 2, prim_array_get_propdirs },    \
                        { "ARRAY_GET_PROPLIST", LM1, 2, prim_array_get_proplist },    \
                        { "ARRAY_PUT_PROPLIST", LM1, 3, prim_array_put_proplist },    \
                        { "ARRAY_GET_REFLIST",  LM1, 2, prim_array_get_reflist },     \
                        { "ARRAY_PUT_REFLIST",  LM1, 3, prim_array_put_reflist },     \
                        { "ARRAY_APPENDITEM",   LM1, 2, prim_array_appenditem },      \
                        { "ARRAY_FINDVAL",      LM1, 2, prim_array_findval },         \
                        { "ARRAY_EXCLUDEVAL",   LM1, 2, prim_array_excludeval },      \
                        { "EXPLODE_ARRAY",      LM1, 2, prim_explode_array },         \
                        { "ARRAY_SORT",         LM1, 2, prim_array_sort },            \
                        { "ARRAY_ANSI_NOTIFY",  LM1, 2, prim_array_ansi_notify },     \
                        { "ARRAY_JOIN",         LM1, 2, prim_array_join },            \
                        { "ARRAY_MATCHKEY",     LM1, 2, prim_array_matchkey },        \
                        { "ARRAY_MATCHVAL",     LM1, 2, prim_array_matchval },        \
                        { "ARRAY_EXTRACT",      LM1, 2, prim_array_extract },         \
                        { "ARRAY_CUT",          LM1, 2, prim_array_cut },             \
                        { "ARRAY_COMPARE",      LM1, 2, prim_array_compare },         \
                        { "ARRAY_SORT_INDEXED", LM1, 3, prim_array_sort_indexed },    \
                        { "ARRAY_INTERPRET",    LM1, 1, prim_array_interpret },       \
                        { "ARRAY_FILTER_FLAGS", LM1, 2, prim_array_filter_flags },    \
                        { "ARRAY_NESTED_GET",   LM1, 2, prim_array_nested_get },      \
                        { "ARRAY_NESTED_SET",   LM1, 3, prim_array_nested_set },      \
                        { "ARRAY_NESTED_DEL",   LM1, 2, prim_array_nested_del },      \
                        { "ARRAY_SUM",          LM1, 1, prim_array_sum },             \
                        { "ARRAY_STRING_FRAGMENT", LM1, 2, prim_array_string_fragment }

#define PRIMS_ARRAY_CNT 51
