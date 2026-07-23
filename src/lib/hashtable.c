#include "lib/hashtable.h"
#include <stdlib.h>
#include <string.h>

struct entry {
  void *key;
  void *value;
};

struct entryList {
  struct entry entry;
  struct entryList *next;
};

struct Hashtable {
  struct entryList **data;
  size_t size; // number of entries

  size_t buckets; // number of buckets
  size_t maxLoad; // max of size/buckets before resize

  KeyHashFn *keyhashfn; // to determine which bucket
  KeyEqFn *keyeqfn;     // to compare keys
};

Hashtable *HashtableCreate(size_t buckets, size_t maxLoad, KeyHashFn *keyhashfn,
                           KeyEqFn *keyeqfn) {
  struct entryList **data = calloc(buckets, sizeof(struct entryList *));
  if (data == NULL) {
    return NULL;
  }

  struct Hashtable *H = malloc(sizeof(struct Hashtable));
  if (H == NULL) {
    free(data);
    return NULL;
  }

  *H = (struct Hashtable){
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
void HashtableDestroy(Hashtable *H) {
  if (H == NULL)
    return;

  freeBuckets(H->data, H->buckets);
  free(H);
}

size_t HashtableSize(Hashtable *H) {
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
    void *key2 = head->entry.key;
    bool matches = (*eq)(key, key2);
    if (matches) {
      return head;
    }
    head = head->next;
  }
  return NULL;
}

Status HashtableFind(Hashtable *H, void *key, void ***out) {
  if (H == NULL) {
    return ERR;
  }

  size_t bucketIdx = (*(H->keyhashfn))(key) % H->buckets;
  struct entryList *head = H->data[bucketIdx];
  struct entryList *b = searchBuckets(H->keyeqfn, head, key);
  if (out != NULL) {
    if (b == NULL) {
      *out = NULL;
    } else {
      *out = &b->entry.value;
    }
  }
  return OK;
}

static Status add(struct entryList **data, KeyHashFn *keyhashfn,
                  size_t numBuckets, void *key, void *value) {
  size_t bucketIdx = (*(keyhashfn))(key) % numBuckets;
  struct entryList *head = data[bucketIdx];
  struct entryList *newHead = malloc(sizeof(struct entryList));
  if (newHead == NULL) {
    return ERR;
  }
  *newHead = (struct entryList){
      .entry = (struct entry){.key = key, .value = value}, .next = head};
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
      struct entry e = curr->entry;
      Status s = add(newBuckets, keyhashfn, numNewBuckets, e.key, e.value);
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

Status HashtableAdd(Hashtable *H, void *key, void *value) {
  if (H == NULL) {
    return ERR;
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

  Status s = add(H->data, H->keyhashfn, H->buckets, key, value);
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

Status HashtableRemove(Hashtable *H, void *key, void ***out) {
  if (H == NULL) {
    return ERR;
  }

  size_t bucketIdx = (*(H->keyhashfn))(key) % H->buckets;
  struct entryList *head = H->data[bucketIdx];
  struct entryList *b = searchBuckets(H->keyeqfn, head, key);
  if (out != NULL) {
    if (b == NULL) {
      *out = NULL;
    } else {
      *out = &b->entry.value;
    }
  }
  if (b != NULL) {
    removeBucket(&(H->data[bucketIdx]), b);
    H->size--;
  }
  return OK;
}

Status HashtableMerge(Hashtable *srcH, Hashtable *destH) {
  if (srcH == NULL || destH == NULL) {
    return ERR;
  }

  for (size_t i = 0; i < srcH->buckets; i++) {
    struct entryList *curr = srcH->data[i];
    while (curr != NULL) {
      struct entry e = curr->entry;
      Status s = HashtableAdd(destH, e.key, e.value);
      if (s == ERR) {
        return ERR;
      }
      curr = curr->next;
    }
  }

  return OK;
}
