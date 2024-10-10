/*
 * File to contain communication data structures shared by
 * kernel and userpsace component.
 */

// The proc device that will be created by the kernel module
#define PROC_FILE_PATH "/proc/kernel_util" // for use by the userspace agent
#define PROC_ENTRY "kernel_util" // for use by the kmod in creating the proc device
#define MODULE_NAME PROC_ENTRY // proc entry happens to be the same as the module name here

// Message types
#define MONITOR 1
#define UNMONITOR 2

struct message {
    int msg_type;
    int buf_len;
    char *buf;
};
