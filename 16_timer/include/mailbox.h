#ifndef MAILBOX_H
#define MAILBOX_H

#include "event.h"
#include "config.h"

#define MBOX_SEND_FRONT         0x12345678
#define MBOX_SEND_NORMAL        0
typedef struct mbox_tag {

    event_t event;
    uint32_t count;
    uint32_t read;
    uint32_t write;
    uint32_t max_count;
    void **msg_buffer;

}mbox_t;

typedef struct mbox_info_tag {
    uint32_t count;
    uint32_t max_count;
    uint32_t task_count;
}mbox_info_t;

extern void mbox_init(mbox_t *mbox, void **msg_buffer, uint32_t max_count);
extern uint32_t mbox_get(mbox_t *mbox, void **msg, uint32_t wait_ticks);
extern uint32_t mbox_get_no_wait(mbox_t *mbox, void **msg);
extern uint32_t mbox_send(mbox_t *mbox, void *msg, uint32_t notify_opition);
extern void mbox_flush(mbox_t *mbox);
extern uint32_t mbox_destory(mbox_t *mbox);
extern void mbox_get_info(mbox_t *mbox, mbox_info_t *info);
#endif /*MAILBOX_H*/
