#ifndef TAGS
#define TAGS

#define START_TASK_TAG       0
#define END_TASK_TAG         1
#define WORKER_AVAILABLE_TAG 2
// Manager → upstream worker: "your new downstream is rank X"
#define DOWNSTREAM_TAG       3
// Upstream worker → downstream worker: direct per-step completion notification
#define STEP_DONE_TAG        4

#endif