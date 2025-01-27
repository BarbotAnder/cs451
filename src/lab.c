#include "lab.h"
#include <stdio.h>

list_t *list_init(void (*destroy_data)(void *), int (*compare_to)(const void *, const void *)) {
    list_t *list = (list_t *)malloc(sizeof(list_t));
    if (list == NULL) {
        return NULL;
    }
    list->destroy_data = destroy_data;
    list->compare_to = compare_to;
    list->size = 0;
    //sentinel node contains no data and begins by pointing to self in both directions
    node_t *sentinel = (node_t *)malloc(sizeof(node_t));
    if (sentinel == NULL) {
        free(list);
        return NULL;
    }
    sentinel->data = NULL;
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    list->head = sentinel;
    return list;
}

void list_destroy(list_t **list) {
    if (list == NULL || *list == NULL) {
        return;
    }
    node_t *current = (*list)->head->next;
    while (current != (*list)->head) {
        node_t *next = current->next;
        (*list)->destroy_data(current->data);
        free(current);
        current = next;
    }
    free((*list)->head);
    free(*list);
    *list = NULL;
}

list_t *list_add(list_t *list, void *data) {
    if (list == NULL || data == NULL) {
        return NULL;
    }
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL) {
        return NULL;
    }
    new_node->data = data;
    new_node->next = list->head->next;
    new_node->prev = list->head;
    list->head->next->prev = new_node;
    list->head->next = new_node;
    list->size++;
    return list;
}

void *list_remove_index(list_t *list, size_t index) {
    if (list == NULL || index >= list->size) {
        return NULL;
    }
    node_t *curr = list->head->next;
    for (size_t i = 0; i < index; i++) {
        curr = curr->next;
    }
    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;
    void *data = curr->data;
    free(curr);
    list->size--;
    return data;
}

int list_indexof(list_t *list, void *data) {
    if (list == NULL || data == NULL) {
        return -1;
    }
    node_t *current = list->head->next;
    size_t index = 0;
    while (index < list->size) {
        if (list->compare_to(current->data, data) == 0) {
            return index;
        }
        current = current->next;
        index++;
    }
    return -1;
}