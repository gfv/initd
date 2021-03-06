#ifndef NULL_TASK_H
#define NULL_TASK_H

#include "task_handle_fwd.h"
#include "null_task_data.h"
#include "epoll.h"

struct task_context;

task_handle_ptr create_task(task_context& ctx, null_task_data const&);

#endif // NULL_TASK_H
