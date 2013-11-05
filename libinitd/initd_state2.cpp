#include "initd_state2.h"

#include "tasks/create_async_task_handle.h"
#include "task_description.h"

#include "make_unique.h"

task2::task2(async_task_handle_sp handle)
    : handle(std::move(handle))
    , should_work(false)
    , stopped_dependencies(0)
    , running_dependants(0)
    , counted_in_dependencies(false)
    , counted_in_dependants(false)
    , counted_in_pending_tasks(false)
{}

bool task2::are_dependencies_running() const
{
    return stopped_dependencies == 0;
}

bool task2::are_dependants_stopped() const
{
    return running_dependants == 0;
}

void task2::sync(initd_state2* istate)
{
    bool affect_dependencies = handle->is_running() || handle->is_in_transition();
    if (affect_dependencies != counted_in_dependencies)
    {
        increment_counter_in_dependencies(affect_dependencies ? 1 : -1);
        counted_in_dependencies = affect_dependencies;
    }

    bool affect_dependants = handle->is_running() && !handle->is_in_transition();
    if (affect_dependants != counted_in_dependants)
    {
        increment_counter_in_dependants(affect_dependants ? -1 : 1);
        counted_in_dependants = affect_dependants;
    }

    bool affect_pending_tasks = handle->is_in_transition() || handle->is_running() != should_work;
    if (affect_pending_tasks != counted_in_pending_tasks)
    {
        istate->pending_tasks += (affect_pending_tasks ? 1 : -1);
        counted_in_pending_tasks = affect_pending_tasks;
    }
}

void task2::increment_counter_in_dependencies(std::ptrdiff_t delta)
{
    for (task2* dep : dependencies)
        dep->running_dependants += delta;
}

void task2::increment_counter_in_dependants(std::ptrdiff_t delta)
{
    for (task2* dep : dependants)
        dep->stopped_dependencies += delta;
}

initd_state2::initd_state2(sysapi::epoll& ep, task_descriptions descriptions)
    : ep(ep)
    , pending_tasks(0)
{
    auto const& descrs = descriptions.get_all_tasks();
    tasks.resize(descrs.size());

    for (size_t i = 0; i != descrs.size(); ++i)
    {
        tasks[i] = make_unique<task2>(create_async_task_handle(ep, [this, i]() {
            tasks[i]->sync(this);

            if (tasks[i]->handle->is_running())
                enqueue_all(tasks[i]->dependants);
            else
                enqueue_all(tasks[i]->dependencies);

        }, descrs[i]->get_data()));

        descr_to_task.insert(std::make_pair(descrs[i].get(), tasks[i].get()));
    }

    for (size_t i = 0; i != descrs.size(); ++i)
    {
        task_description* descr = descrs[i].get();
        task2* my_task = tasks[i].get();

        for (task_description* dep : descr->get_dependencies())
        {
            task2* dep_task = descr_to_task.find(dep)->second;
            my_task->dependencies.push_back(dep_task);
            dep_task->dependants.push_back(my_task);

            ++my_task->stopped_dependencies;
        }
    }
}

void initd_state2::set_run_level(run_level const& rlevel)
{
    clear_should_work_flag();
    for (task_description* d : rlevel.requisites)
        mark_should_work(*descr_to_task.find(d)->second);

    for (task2_sp const& tp : tasks)
        tp->sync(this);

    enqueue_all();
}

bool initd_state2::has_pending_operations()
{
    return pending_tasks != 0;
}

void initd_state2::clear_should_work_flag()
{
    for (task2_sp const& tp : tasks)
        tp->should_work = false;
}

void initd_state2::mark_should_work(task2& t)
{
    bool old = t.should_work;

    t.should_work = true;

    if (!old)
        for (task2* dep : t.dependencies)
            mark_should_work(*dep);
}

void initd_state2::enqueue_all()
{
    for (task2_sp const& tp : tasks)
        enqueue_one(*tp);
}

void initd_state2::enqueue_all(std::vector<task2*> const& tts)
{
    for (task2* t : tts)
        enqueue_one(*t);
}

void initd_state2::enqueue_one(task2& t)
{
    if (!t.handle->is_running() && t.should_work && t.are_dependencies_running())
        t.handle->set_should_work(true);

    if (t.handle->is_running() && !t.should_work && t.are_dependants_stopped())
        t.handle->set_should_work(false);
}