#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include "comms.h"

MODULE_AUTHOR("Austin Cullar");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

// kmod prototypes (see definitions below for more info)
int process_message(struct message *msg);
ssize_t kernel_util_read(struct file *filp, char *buffer, size_t buffer_size,
                         loff_t *offset);
ssize_t kernel_util_write(struct file *filp, const char __user *buffer,
                          size_t buffer_size, loff_t *offset);

// Global message pointer for use in proc read/write operations
struct message *msg = NULL;
static int response_type = 0;

/*
 * Process the provided `struct message` pointer based on its `msg_type` member.
 *
 * In the logic below, I've interpreted a `msg_type` as:
 * 1 = MONITOR
 * 2 = UNMONITOR
 *
 * Although all I'm doing is printing a kernel message here, one might use this
 * logic to direct the kmod to enable/disable filesystem monitoring, for
 * example.
 */
int process_message(struct message *msg)
{
    int rc = -1;

    if (NULL == msg) {
        goto exit;
    }

    switch (msg->msg_type) {
        case MONITOR:
            printk(KERN_INFO "Received MONITOR command\n");
            response_type = 1;
            break;
        case UNMONITOR:
            printk(KERN_INFO "Received UNMONITOR command\n");
            response_type = 2;
            break;
        default:
            printk(KERN_INFO "Received unknown command: %d\n", msg->msg_type);
            break;
    }

    rc = 0;

exit:
    return rc;
}

/*
 * Called when the user reads from the proc device.
 */
ssize_t kernel_util_read(struct file *filp, char *buffer, size_t buffer_size,
                         loff_t *offset)
{
    // get inode data for `/proc/kernel_util` proc device
    char *data = pde_data(file_inode(filp));
    if (!data) {
        printk(KERN_ALERT "Failed to get inode data\n");
        return -1;
    }

    // since we're only expecting to read a `struct message` from the buffer,
    // if the offset is greater than sizeof(struct message), we're done reading
    if (sizeof(struct message) <= *offset) {
        return 0;
    }

    // determine response string
    // this is what the user/agent will see when attempting to read from the `/proc/kernel_util` file
    char *response_str = NULL;
    switch (response_type) {
        case 1:
            printk(KERN_INFO "response_type: 1\n");
            response_str = "response 1";
            break;
        case 2:
            printk(KERN_INFO "response_type: 2\n");
            response_str = "response 2";
            break;
        default:
            printk(KERN_ALERT "Received unknown response type: %d\n", response_type);
            response_str = "bad request";
            break;
    }

    // adjust buffer size to account for response string (plus 1 byte for null terminator)
    buffer_size = strlen(response_str)+1;

    // copy `response_str` into `buffer`
    if (copy_to_user(buffer, response_str, buffer_size)) {
        printk(KERN_ALERT "Failed to copy data to user");
        return -1;
    }

    // update the offset after writing to `buffer`
    *offset = buffer_size;

    return buffer_size;
}

/*
 * Called when the user writes to the proc device.
 */
ssize_t kernel_util_write(struct file *filp, const char __user *buffer,
                          size_t buffer_size, loff_t *offset)
{
    // get inode data for `/proc/kernel_util` proc device
    char *data = pde_data(file_inode(filp));
    if (!data) {
        printk(KERN_ALERT "Failed to get inode data\n");
        return -1;
    }

    // since we're expecting the user/agent to write a `struct message` to the
    // proc device, getting more or less than `sizeof(struct message)` indicates
    // an error
    if (buffer_size != sizeof(struct message)) {
        printk(KERN_ALERT "too much data written to proc device\n");
        printk(KERN_INFO "\t\tbuffer_size = %zu\n", buffer_size);
        printk(KERN_INFO "\t\tsizeof(struct_message) = %zu\n",
               sizeof(struct message));
        return -1;
    }

    // copy data written to `/proc/kernel_util` into local `msg` variable
    if (copy_from_user(msg, buffer, buffer_size)) {
        printk(KERN_ALERT "Failed to get user data\n");
        return -1;
    }

    printk(KERN_INFO "data received in message:\n");
    printk(KERN_INFO "\t\tmsg->msg_type: %d\n", msg->msg_type);
    printk(KERN_INFO "\t\tmsg->buf_len: %d\n", msg->buf_len);
    printk(KERN_INFO "\t\tmsg->buf: %s\n", msg->buf);

    // process the received message
    if (process_message(msg)) {
        return -1;
    }

    // update the offset after reading from `buffer`
    *offset = buffer_size;

    return buffer_size;
}

/*
 * Registration of proc device read/write functions.
 */
struct proc_ops  proc_fops = {
    .proc_read = kernel_util_read,
    .proc_write = kernel_util_write,
};

/*
 * Initialize the kmod (this is executed when the module is loaded).
 * 
 * Here, I dynamically allocate a `struct message` pointer for use in reading
 * data from the proc device (I only expect the writer to provide a
 * `struct message` as input).
 *
 * Then, I create the proc device with `proc_create_data()`.
 */
static int __init kernel_util_init(void)
{
    printk(KERN_INFO "kernel_util init\n");

    // allocate `message` structure for use in proc read/write ops
    msg = kzalloc(sizeof(struct message), GFP_KERNEL);
    if (!msg) {
        printk(KERN_ALERT "failed to allocate memory for received message\n");
        return 1;;
    }

    // create the proc device
    if (NULL == proc_create_data(PROC_ENTRY, 0666, NULL, &proc_fops, msg)) {
        printk(KERN_ALERT "Failed to create proc device\n");
    }

    return 0;
}

/*
 * Unload the kmod (this is executed when the module is unloaded).
 *
 * Here, I free the memory I allocated in the `kernel_util_init` function. Then,
 * I remove the proc device with `remove_proc_entry()`.
 */
static void __exit kernel_util_exit(void)
{
    printk(KERN_INFO "kernel_util exit\n");
    kfree(msg);
    remove_proc_entry(PROC_ENTRY, NULL);
}

// Register kmod init/exit functions
module_init(kernel_util_init);
module_exit(kernel_util_exit);
