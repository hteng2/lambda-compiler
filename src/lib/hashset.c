#include "lib/hashset.h"
#include <stdlib.h>
#include <string.h>

struct entryList {
  void *key;
  struct entryList *next;
};

struct Hashset {
  struct entryList **data;
  size_t size; // number of entries

  size_t buckets; // number of buckets
  size_t maxLoad; // max of size/buckets before resize

  KeyHashFn *keyhashfn; // to determine which bucket
  KeyEqFn *keyeqfn;     // to compare keys
};

Hashset *HashsetCreate(size_t buckets, size_t maxLoad, KeyHashFn *keyhashfn,
                       KeyEqFn *keyeqfn) {
  struct entryList **data = calloc(buckets, sizeof(struct entryList *));
  if (data == NULL) {
    return NULL;
  }

  struct Hashset *H = malloc(sizeof(struct Hashset));
  if (H == NULL) {
    free(data);
    return NULL;
  }

  *H = (struct Hashset){
      .data = data,
      .size = 0,
      .buckets = buckets,
      .maxLoad = maxLoad,
      .keyhashfn = keyhashfn,
      .keyeqfn = keyeqfn,
  };
  return H;
}

static void freeBuckets(struct entryList **buckets, size_t numBuckets) {
  for (size_t i = 0; i < numBuckets; i++) {
    struct entryList *curr = buckets[i];
    while (curr != NULL) {
      struct entryList *next = curr->next;
      free(curr);
      curr = next;
    }
  }

  free(buckets);
}
void HashsetDestroy(Hashset *H) {
  if (H == NULL)
    return;

  freeBuckets(H->data, H->buckets);
  free(H);
}

size_t HashsetSize(Hashset *H) {
  if (H == NULL) {
    return 0;
  }

  return H->size;
}

/**
 * points result at the entry if it exists
 */
static struct entryList *searchBuckets(KeyEqFn *eq, struct entryList *head,
                                       void *key) {
  while (head != NULL) {
    void *key2 = head->key;
    bool matches = (*eq)(key, key2);
    if (matches) {
      return head;
    }
    head = head->next;
  }
  return NULL;
}

Status HashsetFind(Hashset *H, void *key, bool *exists) {
  if (H == NULL) {
    return ERR;
  }

  size_t bucketIdx = (*(H->keyhashfn))(key) % H->buckets;
  struct entryList *head = H->data[bucketIdx];
  struct entryList *b = searchBuckets(H->keyeqfn, head, key);
  if (exists != NULL) {
    *exists = b != NULL;
  }
  return OK;
}

static Status add(struct entryList **data, KeyHashFn *keyhashfn,
                  size_t numBuckets, void *key) {
  size_t bucketIdx = (*(keyhashfn))(key) % numBuckets;
  struct entryList *head = data[bucketIdx];
  struct entryList *newHead = malloc(sizeof(struct entryList));
  if (newHead == NULL) {
    return ERR;
  }
  *newHead = (struct entryList){.key = key, .next = head};
  data[bucketIdx] = newHead;

  return OK;
}

/**
 * frees the old buckets and returns the new one if possible
 * otherwise, does nothing and returns NULL
 */
static struct entryList **resizeBuckets(struct entryList **oldBuckets,
                                        KeyHashFn *keyhashfn,
                                        size_t numOldBuckets,
                                        size_t numNewBuckets) {
  struct entryList **newBuckets =
      malloc(sizeof(struct entryList *) * numNewBuckets);
  if (newBuckets == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < numOldBuckets; i++) {
    struct entryList *curr = oldBuckets[i];
    while (curr != NULL) {
      void *key = curr->key;
      Status s = add(newBuckets, keyhashfn, numNewBuckets, key);
      if (s == ERR) {
        freeBuckets(newBuckets, numNewBuckets);
        return NULL;
      }
      curr = curr->next;
    }
  }

  freeBuckets(oldBuckets, numOldBuckets);
  return newBuckets;
}

Status HashsetAdd(Hashset *H, void *key) {
  if (H == NULL) {
    return ERR;
  }

  bool exists = false;
  if (HashsetFind(H, key, &exists) == ERR) {
    return ERR;
  }
  if (exists) {
    return OK;
  }

  if (H->size / H->buckets >= H->maxLoad) {
    struct entryList **newData =
        resizeBuckets(H->data, H->keyhashfn, H->buckets, 2 * H->buckets);
    if (newData == NULL) {
      return ERR;
    }

    H->data = newData;
    H->buckets *= 2;
  }

  Status s = add(H->data, H->keyhashfn, H->buckets, key);
  if (s == ERR) {
    return ERR;
  }

  H->size++;
  return OK;
}

static void removeBucket(struct entryList **head, struct entryList *target) {
  if (head == NULL) {
    return;
  }

  while (*head != target) {
    if (*head == NULL) {
      return;
    }
    head = &((*head)->next);
  }

  *head = ((*head)->next);
  free(target);
}

Status HashsetRemove(Hashset *H, void *key, bool *exists) {
  if (H == NULL) {
    return ERR;
  }

  size_t bucketIdx = (*(H->keyhashfn))(key) % H->buckets;
  struct entryList *head = H->data[bucketIdx];
  struct entryList *b = searchBuckets(H->keyeqfn, head, key);
  if (exists != NULL) {
    *exists = b != NULL;
  }
  if (b != NULL) {
    removeBucket(&(H->data[bucketIdx]), b);
    H->size--;
  }
  return OK;
}

Status HashsetMerge(Hashset *srcH, Hashset *destH) {
  if (srcH == NULL || destH == NULL) {
    return ERR;
  }

  for (size_t i = 0; i < srcH->buckets; i++) {
    struct entryList *curr = srcH->data[i];
    while (curr != NULL) {
      void *key = curr->key;
      if (HashsetAdd(destH, key) == ERR) {
        return ERR;
      }
      curr = curr->next;
    }
  }

  return OK;
}

Status HashsetIter(Hashset *H, KeyIterFn *keyiterfn) {
  if (H == NULL) {
    return ERR;
  }

  for (size_t i = 0; i < H->buckets; i++) {
    struct entryList *curr = H->data[i];
    while (curr != NULL) {
      (*keyiterfn)(curr->key);
      curr = curr->next;
    }
  }

  return OK;
}
