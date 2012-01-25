#ifndef JSON_H
#define JSON_H

#ifdef JSON_SUPPORT

/* To encode a dbref or lock in JSON, we need an escape sequence that is
   unlikely to collide with legitimate data. Thankfully, that's what the
   "private use area" of the Unicode spec is for. To reduce risk of collision
   even further, we "sign" the beginning of the string with a combination of
   private use characters. This is then followed by a "type classifier"
   character that indicates the type of data we're encoding.

   Encoding string: SIGNATURE + TYPE + string representation of data

   EXAMPLES:

   Assuming a signature block of EE42+EF69, with EE00 representing "dbref" and
   "EE01" representing a lock string...

   UTF-16BE|  UTF-8
   --------+---------
    U+EE42 | EE B9 82
    U+EF69 | EE BD A9
    U+EE00 | EE B8 80
    U+EE01 | EE B8 81

          #42 -> "\xEE\xB9\x82\xEE\xBD\xA9\xEE\xBD\x80" "42"
                  |       SIGNATURE      ||   TYPE   |   |DATA->
   "#42&!#42" -> "\xEE\xB9\x82\xEE\xBD\xA9\xEE\xB8\x81" "#42&!#42"
                  |       SIGNATURE      ||   TYPE   |   |DATA->

   Note that the "data" portion of the string is split off from the hex prefix.
   This is to prevent the first character of data from being interpreted as part
   of the final \x escape if the first data character is numeric.
*/
//#define UTF8_SIGNATURE "\xEE\xB9\x82\xEE\xBD\xA9"
//#define JSON_PREFIX_DBREF UTF8_SIGNATURE "\uEF00"
//#define JSON_PREFIX_LOCK  UTF8_SIGNATURE "\uEF01"
#define UTF8_SIGNATURE "\xEE\xB9\x82\xEE\xBD\xA9"
#define JSON_PREFIX_DBREF UTF8_SIGNATURE "\xEE\xB8\x80"
#define JSON_PREFIX_LOCK  UTF8_SIGNATURE "\xEE\xB8\x81"

typedef struct json_queue_t {
    stk_array *mnode;
    json_t *jnode;
    struct json_queue_t *nxt;
} json_queue;

json_t *array_to_json(stk_array *arr, dbref player);
stk_array *json_to_array(json_t *jroot, int minrefs);
stk_array *get_json_error(json_error_t *error);

#endif /* JSON_SUPPORT */

#endif /* JSON_H */

