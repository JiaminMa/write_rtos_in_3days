#ifndef LIB_H
#define LIB_H

#include <stdint.h>

/*Bitmap*/
typedef struct bitmap_tag {
    uint32_t bitmap;
}bitmap_t;

extern void bitmap_init(bitmap_t *bitmap);
extern uint32_t bitmap_count(void);
extern void bitmap_set(bitmap_t *bitmap, uint32_t pos);
extern void bitmap_clear(bitmap_t *bitmap, uint32_t pos);
extern uint32_t bitmap_get_first_set(bitmap_t *bitmap);

/*Double Linked List*/
typedef struct list_node_tag {
    struct list_node_tag *prev;
    struct list_node_tag *next;
}list_node_t;

extern void list_node_init(list_node_t *node);

typedef struct list_tag {
    list_node_t head;
    uint32_t node_count;
}list_t;

#define container_of(node, parent, name) (parent *)((uint32_t)node - (uint32_t)&((parent *)0)->name)
extern void list_init(list_t *list);
extern uint32_t list_count(list_t *list);
extern list_node_t *list_head(list_t *list);
extern list_node_t *list_tail(list_t *list);
extern list_node_t *node_prev(list_node_t *list_node);
extern list_node_t *node_next(list_node_t *list_node);
extern void list_remove_all(list_t *list);
extern void list_insert_head(list_t *list, list_node_t *list_node);
extern void list_append_last(list_t *list, list_node_t *list_node);
extern list_node_t *list_remove_first(list_t *list);
extern void list_remove(list_t *list, list_node_t *node);

#endif /*LIB_H*/
