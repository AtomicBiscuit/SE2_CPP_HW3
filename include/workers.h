#pragma once


#include <atomic>
#include <vector>
#include <memory>
#include <thread>
#include "tasks.h"

class WorkerHandler {
private:
    int workers = 0;
    bool is_active = false;
public:
    std::atomic<std::vector<std::unique_ptr<Task>> *> atomic_tasks = nullptr;
    std::atomic<int> ind = 0;
    std::atomic<int> finish = 0;
    std::atomic<int> start = 0;

    WorkerHandler() = default;

    void init(int n);

    void set_tasks(std::vector<std::unique_ptr<Task>> *);

    void wait_until_end();

private:
    static void _worker_impl(WorkerHandler &);
};