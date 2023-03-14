#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "LinkedList.h"

static void LinkedList_clear(LinkedList *this) {
    Node *tmp;
    Node *head;
    head = this->head;
    while (head) {
        if (head->data) {
            free(head->data);
        }
        tmp = head->next;
        free(head);
        head = tmp;
    }
    this->head = NULL;
}

static void LinkedList_free(LinkedList *this) {
    LinkedList_clear(this);
    free(this);
    this = NULL;
}

static int LinkedList_size(LinkedList *this) {
    Node *head;
    int result = 0;
    head = this->head;
    while (head) {
        result++;
        head = head->next;
    }
    return result;
}

static int LinkedList_insert(LinkedList *this, int index, void *data, int size) {
    Node *head = this->head;
    if (size < 0) {
        return -1;
    }
    Node *newNode = malloc(sizeof(Node));
    newNode->data = malloc(sizeof(char) * size);
    memcpy(newNode->data, data, size);
    if (index < 0) {
        printf("index out of rang\n");
        return -1;
    }
    if (index == 0) {
        newNode->next = this->head;
        this->head = newNode;
        return 0;
    }
    for (int i = 1; i < index; i++) {
        if (!head->next) {
            printf("index out of range\n");
            return -1;
        }
        head = head->next;
    }
    newNode->next = head->next;
    head->next = newNode;
    return 0;
}

static void *LinkedList_at(LinkedList *this, int index) {
    Node *head = this->head;
    if (index < 0) {
        printf("index out of rang\n");
        return NULL;
    }
    for (int i = 0; i < index; i++) {
        if (!head->next) {
            printf("index out of range\n");
            return NULL;
        }
        head = head->next;
    }
    return head->data;
}

int LinkedList_initialize(LinkedList *this) {
    if (!this) {
        return -1;
    }
    this->free = LinkedList_free;
    this->clear = LinkedList_clear;
    this->size = LinkedList_size;
    this->insert = LinkedList_insert;
    this->at = LinkedList_at;
    this->head = NULL;
    return 0;
}

int test() {
    LinkedList *list = malloc(sizeof(LinkedList));
    LinkedList_initialize(list);
    for (int i = 0; i < 10; i++) {
        char data[10];
        sprintf(data, "%d", i);
        list->insert(list, 0, data, strlen(data) + 1);
    }
    printf("size:%d\n", list->size(list));
    printf("at:%s\n", (char *) (list->at(list, 2)));
    for (int i = 0; i < 10; i++) {
        printf("%s\n", (char *) (list->at(list, i)));
    }
    list->free(list);
    return 0;
}

//int main() {
//    return test();
//}
