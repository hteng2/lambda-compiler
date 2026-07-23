#ifndef hashtable_h
#define hashtable_h

#include "status.h"
#include <stdlib.h>

typedef size_t((KeyHashFn)(void *));
typedef bool((KeyEqFn)(void *, void *));

typedef struct Hashtable Hashtable;

/**
 * creates a hashtable
 * keyhashfn and keyeqfn must be deterministic
 */
Hashtable *HashtableCreate(size_t buckets, size_t maxLoad, KeyHashFn *keyhashfn,
                           KeyEqFn *keyeqfn);

/**
 * frees a hashtable, does nothing if H == NULL
 */
void HashtableDestroy(Hashtable *H);

/**
 * returns the number of items in a hashtable, or 0 if it's null
 */
size_t HashtableSize(Hashtable *H);

/**
 * points out at the first entry's value whose key matches key
 * points out at NULL if entry doesn't exist
 * returns ERR if H == NULL
 */
Status HashtableFind(Hashtable *H, void *key, void ***out);

/**
 * adds an entry, shadowing previous entries with the same key
 * returns ERR is H == NULL, or out of memory
 */
Status HashtableAdd(Hashtable *H, void *key, void *value);

/**
 * points out at the first entry's value whose key matches key,
 *     and removes the entry
 * points out at NULL if entry doesn't exist
 * returns ERR if H == NULL
 */
Status HashtableRemove(Hashtable *H, void *key, void ***out);

/**
 * merges srcH's entries into destH
 * returns ERR if srcH or destH are NULL, or out of memory
 */
Status HashtableMerge(Hashtable *srcH, Hashtable *destH);

#endif
