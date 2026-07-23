#ifndef hashset_h
#define hashset_h

#include "status.h"
#include <stdlib.h>

typedef size_t((KeyHashFn)(void *));
typedef bool((KeyEqFn)(void *, void *));
typedef void((KeyIterFn)(void *));

typedef struct Hashset Hashset;

/**
 * creates a hashset
 * keyhashfn and keyeqfn must be deterministic
 */
Hashset *HashsetCreate(size_t buckets, size_t maxLoad, KeyHashFn *keyhashfn,
                       KeyEqFn *keyeqfn);

/**
 * frees a hashset, does nothing if H == NULL
 */
void HashsetDestroy(Hashset *H);

/**
 * returns the number of items in a hashset, or 0 if it's null
 */
size_t HashsetSize(Hashset *H);

/**
 * sets exists depending on whether key exists
 * returns ERR if H == NULL
 */
Status HashsetFind(Hashset *H, void *key, bool *exists);

/**
 * adds an entry, NOT shadowing previous entries with the same key
 * returns ERR is H == NULL, or out of memory
 */
Status HashsetAdd(Hashset *H, void *key);

/**
 * sets exists depending on whether key exists
 * removes the key
 * returns ERR if H == NULL
 */
Status HashsetRemove(Hashset *H, void *key, bool *exists);

/**
 * unions srcH's entries into destH
 * returns ERR if srcH or destH are NULL, or out of memory
 */
Status HashsetMerge(Hashset *srcH, Hashset *destH);

/**
 * calls f for each key of the hashset in some order
 * returns ERR if H == NULL
 */
Status HashsetIter(Hashset *H, KeyIterFn *keyiterfn);

#endif
