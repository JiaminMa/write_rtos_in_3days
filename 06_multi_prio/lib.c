#include "lib.h"
#include "os_stdio.h"
void bitmap_init(bitmap_t *bitmap)
{
    bitmap->bitmap = 0;
}

uint32_t bitmap_count()
{
    return 32;
}

void bitmap_set(bitmap_t *bitmap, uint32_t pos)
{
    bitmap->bitmap |= 1 << pos;
}

void bitmap_clear(bitmap_t *bitmap, uint32_t pos)
{
    bitmap->bitmap &= ~(1 << pos);
}

uint32_t bitmap_get_first_set(bitmap_t *bitmap)
{
    uint32_t pos = 32;
    static const uint8_t quick_table[] =
    {
        /* 00 */ 0xff, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 10 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 20 */ 5,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 30 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 40 */ 6,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 50 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 60 */ 5,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 70 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 80 */ 7,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* 90 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* A0 */ 5,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* B0 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* C0 */ 6,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* D0 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* E0 */ 5,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
        /* F0 */ 4,    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
    };

    if (bitmap->bitmap & 0xff) {
        pos = quick_table[bitmap->bitmap & 0xff];
    } else if (bitmap->bitmap & 0xff00) {
        pos = quick_table[(bitmap->bitmap >> 8) & 0xff] + 8;
    } else if (bitmap->bitmap & 0xff0000) {
        pos = quick_table[(bitmap->bitmap >> 16) & 0xff] + 16;
    } else if (bitmap->bitmap & 0xFF000000) {
        pos = quick_table[(bitmap->bitmap >> 24) & 0xFF] + 24;
    } else {
        pos = bitmap_count();
    }

    return pos;
}

void list_node_init(list_node_t *node)
{
    if (node != (list_node_t *) NULL) {
        node->prev = node;
        node->next = node;
    }
}

void list_init(list_t *list)
{

    if (list != (list_t * )NULL) {
        list->node_count = 0;
        list_node_init(&(list->head));
    }
}

uint32_t list_count(list_t *list)
{
    uint32_t ret = 0;
    if (list != (list_t * )NULL) {
       ret = list->node_count;
    }
    return ret;
}

list_node_t *list_head(list_t *list)
{
    list_node_t *head = &(list->head);
    list_node_t *ret = (list_node_t *)NULL;
    if (list->node_count != 0) {
        ret = head->next;
    }
    return ret;
}

list_node_t *list_tail(list_t *list)
{
    list_node_t *head = &(list->head);
    list_node_t *ret = (list_node_t *)NULL;
    if (list->node_count != 0) {
        ret = head->prev;
    }
    return ret;
}

list_node_t *node_prev(list_node_t *list_node)
{
    list_node_t *ret = (list_node_t *)NULL;
    if (list_node != (list_node_t *)NULL) {
        ret = list_node->prev;
    }
    return ret;
}

list_node_t *node_next(list_node_t *list_node)
{
    list_node_t *ret = (list_node_t *)NULL;
    if (list_node != (list_node_t *)NULL) {
        ret = list_node->next;
    }
    return ret;
}

void list_remove_all(list_t *list)
{
    uint32_t count;
    list_node_t *head = (list_node_t *)NULL;
    list_node_t *next_node = (list_node_t *)NULL;
    list_node_t *cur_node = (list_node_t *)NULL;
    if (list == (list_t *)NULL) {
        goto cleanup;
    }

    head = &(list->head);
    next_node = head->next;
    for (count = list->node_count; count != 0; count--) {
        cur_node = next_node;
        next_node = cur_node->next;
        cur_node->next = cur_node;
        cur_node->prev = cur_node;
    }

    head->prev = head;
    head->next = head;
    list->node_count = 0;
cleanup:
    return;

}

void list_insert_head(list_t *list, list_node_t *list_node)
{
    list_node_t *head = (list_node_t *)NULL;
    list_node_t *next = (list_node_t *)NULL;
    if (list == (list_t *)NULL) {
        goto cleanup;
    }

    head = &(list->head);
    next = head->next;

    head->next = list_node;
    next->prev = list_node;
    list_node->prev = head;
    list_node->next = next;
    list->node_count++;
cleanup:
    return;
}

void list_append_last(list_t *list, list_node_t *list_node)
{
    list_node_t *head = (list_node_t *)NULL;
    list_node_t *next = (list_node_t *)NULL;
    if (list == (list_t *)NULL) {
        goto cleanup;
    }

    head = &(list->head);
    next = head->next;

    head->prev = list_node;
    next->next = list_node;
    list_node->next = head;
    list_node->prev = next;
    list->node_count++;
cleanup:
    return;
}

list_node_t *list_remove_first(list_t *list)
{
    list_node_t *head = (list_node_t *)NULL;
    list_node_t *next = (list_node_t *)NULL;
    list_node_t *ret = (list_node_t *)NULL;

    if (list == (list_t *)NULL) {
        goto cleanup;
    }

    head = &(list->head);
    ret = head->next;
    if (ret == head) {
        goto cleanup;
    }

    next = ret->next;
    head->next = next;
    next->prev = head;
    ret->next = ret;
    ret->prev = ret;
    list->node_count--;
cleanup:
    return ret;
}

void list_remove(list_t *list, list_node_t *node)
{
    list_node_t *next = (list_node_t *)NULL;
    list_node_t *prev = (list_node_t *)NULL;

    if (list == (list_t *) NULL || node == (list_node_t *)NULL) {
        goto cleanup;
    }

    next = node->next;
    prev = node->prev;
    if (next == (list_node_t *)NULL || prev == (list_node_t *)NULL) {
        goto cleanup;
    }

    prev->next = next;
    next->prev = prev;
    list_node_init(node);
cleanup:
    return;
}
