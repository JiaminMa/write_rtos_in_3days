#ifndef FLAG_GROUP_H
#define FLAG_GROUP_H

#include "event.h"
typedef struct flag_group_tag {

    event_t event;
    uint32_t flag;
}flag_group_t;

typedef struct flag_group_info_tag {

    uint32_t flags;
    uint32_t task_count;
}flag_group_info_t;

#define FLAGGROUP_CLEAR         (0x0 << 0)
#define FLAGGROUP_SET           (0x1 << 0)
#define FLAGGROUP_ANY           (0x0 << 1)
#define FLAGGROUP_ALL           (0x1 << 1)

#define FLAGGROUP_SET_ALL       (FLAGGROUP_SET | FLAGGROUP_ALL)
#define FLAGGROUP_SET_ANY       (FLAGGROUP_SET | FLAGGROUP_ANY)
#define FLAGGROUP_CLEAR_ALL     (FLAGGROUP_CLEAR | FLAGGROUP_ALL)
#define FLAGGROUP_CLEAR_ANY     (FLAGGROUP_CLEAR | FLAGGROUP_ANY)

#define FLAGGROUP_CONSUME       (1 << 7)


extern void flag_group_init(flag_group_t *flag_group, uint32_t flags);
extern uint32_t flag_group_wait(flag_group_t *flag_group, uint32_t wait_type,
                            uint32_t request_flag, uint32_t *result_flag, uint32_t wait_ticks);
extern uint32_t flag_group_no_wait_get(flag_group_t *flag_group, uint32_t wait_type, uint32_t request_flag,
                            uint32_t *result_flag);
extern void flag_group_notify(flag_group_t *flag_group, uint8_t is_set, uint32_t flag);
extern void flag_group_get_info(flag_group_t *flag_group, flag_group_info_t *info);
extern uint32_t flag_group_destory(flag_group_t *flag_group);

#endif /*FLAG_GROUP_H*/
