#ifndef INITD_STATE2_H
#define INITD_STATE2_H

#include <vector>
#include <map>

#include "epoll.h"

#include "task_descriptions.h"
#include "run_level.h"

#include "tasks/async_task_handle.h"
#include "task_context.h"

struct initd_state;
struct state_context;

struct task2
{
    task2(async_task_handle_sp handle);

    bool are_dependencies_running() const;
    bool are_dependants_stopped() const;

    async_task_handle_sp handle;
    std::vector<task2*>  dependencies;
    std::vector<task2*>  dependants;
    bool                 should_work;

    void sync(initd_state* istate);

private:
    void increment_counter_in_dependencies(std::ptrdiff_t);
    void increment_counter_in_dependants(std::ptrdiff_t);

public:
    size_t               stopped_dependencies;
    size_t               running_dependants;

    bool                 counted_in_dependencies;
    bool                 counted_in_dependants;
    bool                 counted_in_pending_tasks;
};

typedef std::unique_ptr<task2> task2_sp;

struct initd_state : private task_context
{
    initd_state(state_context& ctx, sysapi::epoll& ep, task_descriptions descriptions);

    void set_run_level(std::string const& run_level_name);
    void set_empty_run_level();

    bool has_pending_operations();

private:
    void clear_should_work_flag();
    void mark_should_work(task2&);
    void enqueue_all();
    void enqueue_all(std::vector<task2*> const&);
    void enqueue_one(task2&);

private:
    // task_context
    sysapi::epoll& get_epoll();
    state_context& get_state_context();

private:
    state_context&                              ctx;
    sysapi::epoll&                              ep;
    std::map<std::string, std::vector<task2*> > run_levels;
    std::vector<task2_sp>                       tasks;
    size_t                                      pending_tasks;

    friend struct task2;
};

#endif