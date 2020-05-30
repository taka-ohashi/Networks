#include "linkedlist.h"

void addToNodelist(Nodelist *nodelist, ClientNode *node) {
    if (nodelist->head == NULL) {
        nodelist->head = node;
    } else {
        ClientNode *temp = nodelist->head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = node;
    }
}

ClientNode *initializeClientNode(int socketNum) {
    ClientNode *node = malloc(sizeof(ClientNode));
    if (node == NULL) {
        fprintf(stderr, "Error trying to malloc for a CharNode\n");
        exit(EXIT_FAILURE);
    } else {
        node->handle = NULL;
        node->socketNum = socketNum;
        node->next = NULL;
    }

    return node;
}

Nodelist *initializeNodelist() {
    Nodelist *list = malloc(sizeof(Nodelist));
    if (list == NULL) {
        fprintf(stderr, "Error tring to malloc for a Nodelist\n");
        exit(EXIT_FAILURE);
    } else {
        list->head = NULL;
    }

    return list;
}

ClientNode *findNode(Nodelist *list, int socketNum) {
    ClientNode *temp = list->head;
    while (temp != NULL) {
        if (temp->socketNum == socketNum) {
            return temp;
        }
        temp = temp->next;
    }

    return NULL;
}

void freeNode(ClientNode *temp) {
    if (temp->handle != NULL) {
        free(temp->handle);
    }

    free(temp);
}

/* LinkedList remove node algorithm found on the internet */
void removeNode(Nodelist *list, int socketNum) {
    ClientNode *temp = list->head;
    ClientNode *prev = NULL;
    if (temp != NULL && temp->socketNum == socketNum) {
        list->head = temp->next;
        freeNode(temp);
        return;
    }

    while (temp != NULL && temp->socketNum != socketNum) {
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) {
        return;
    }

    prev->next = temp->next;

    freeNode(temp);
}

void freeNodelist(Nodelist *list) {
    if (list != NULL) {
        ClientNode *head = list->head;
        while (head != NULL) {
            ClientNode *temp = head;
            head = head->next;
            freeNode(temp);
        }
    }

    free(list);
}
