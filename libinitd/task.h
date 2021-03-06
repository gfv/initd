#ifndef TASK_H
#define TASK_H

#include "task_fwd.h"
#include "tasks/async_task_handle_fwd.h"
#include <vector>

struct initd_state;

struct task
{
    task(async_task_handle_sp handle);

    async_task_handle*        get_handle();
    std::vector<task*> const& get_dependencies() const;
    std::vector<task*> const& get_dependants() const;

    void clear_should_work();
    void mark_should_work();
    void sync(initd_state* istate);
    void enqueue_this();

    friend void add_task_dependency(task& dependant, task& dependency);

private:
    bool are_dependencies_running() const;
    bool are_dependants_stopped() const;

    void increment_counter_in_dependencies(std::ptrdiff_t);
    void increment_counter_in_dependants(std::ptrdiff_t);

private:
    async_task_handle_sp handle;
    bool                 should_work;

    std::vector<task*>   dependencies;
    std::vector<task*>   dependants;

    size_t               stopped_dependencies;
    size_t               running_dependants;

    bool                 counted_in_dependencies;
    bool                 counted_in_dependants;
    bool                 counted_in_pending_tasks;
};

#endif
