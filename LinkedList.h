//
// Created by happy on 3/9/23.
//

#ifndef LEETCODE_LINKEDLIST_H
#define LEETCODE_LINKEDLIST_H
typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct LinkedList {
    Node *head;

    void (*free)(struct LinkedList *this);

    int (*insert)(struct LinkedList *this, int index,void *data, int size);

    int (*size)(struct LinkedList *this);

    void *(*at)(struct LinkedList *this, int index);
    void (*clear)(struct LinkedList *this);
} LinkedList;

int LinkedList_initialize(LinkedList *this);

#endif //LEETCODE_LINKEDLIST_H
