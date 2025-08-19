from mpi4py import MPI
import time
import csv
import socket
from collections import defaultdict

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

num_steps = 10  # Number of steps per task
num_tasks = 16
time_per_task = 1.0

# Define dependencies
dependencies = {}
for task_id in range(num_tasks):
    dependencies[task_id] = set()
    for i in range(num_steps):
        if task_id - i - 1 >= 0:
            dependencies[task_id].add((task_id - i - 1, i))

if rank == 0:
    # Manager
    completed = set()
    assigned = set()
    step_count = defaultdict(int)
    task_queue = list(range(num_tasks))
    active_workers = set(range(1, size))

    tic = time.time()

    while task_queue or assigned:
        # Receive step completions
        while comm.Iprobe(source=MPI.ANY_SOURCE, tag=1):
            msg = comm.recv(source=MPI.ANY_SOURCE, tag=1)
            completed.add(tuple(msg))
            task_id, step = msg
            step_count[task_id] += 1
            print(f"Manager: received completion of {msg}")
            if step_count[task_id] == num_steps:
                assigned.remove(task_id)
                print(f"Manager: task {task_id} fully completed")

        # Assign ready tasks
        for task_id in task_queue[:]:
            deps = dependencies.get(task_id, set())
            if deps.issubset(completed):
                if active_workers:
                    worker = active_workers.pop()
                    comm.send(task_id, dest=worker, tag=0)
                    assigned.add(task_id)
                    task_queue.remove(task_id)
                    print(f"Manager: assigned task {task_id} to worker {worker}")

        # Reclaim workers
        for worker in range(1, size):
            if comm.Iprobe(source=worker, tag=2):
                _ = comm.recv(source=worker, tag=2)
                active_workers.add(worker)
                print(f"Manager: worker {worker} is now free")

    toc = time.time()
    exec_time = toc - tic
    total_time = num_tasks * num_steps * time_per_task
    print(f"Manager: exec time {exec_time:.2f} sec. Speedup {total_time/exec_time:.2f} using {size - 1} workers")

    # Send stop signal
    for worker in range(1, size):
        comm.send(None, dest=worker, tag=0)
        print(f"Manager: sent stop signal to worker {worker}")

else:
    # Worker
    hostname = socket.gethostname()
    log_file = f"activity_log_{rank}_{hostname}.csv"

    with open(log_file, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["worker_id", "task_id", "step", "start_time", "end_time"])

        while True:
            task_id = comm.recv(source=0, tag=0)
            if task_id is None:
                break
            for step in range(num_steps):
                start_time = time.time()
                time.sleep(time_per_task)
                end_time = time.time()
                writer.writerow([rank, task_id, step, start_time, end_time])
                f.flush()
                comm.send((task_id, step), dest=0, tag=1)
            comm.send(None, dest=0, tag=2)