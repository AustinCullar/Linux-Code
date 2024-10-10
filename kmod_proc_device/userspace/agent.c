#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "comms.h"

#define finit_module(fd, param_values, flags) syscall(__NR_finit_module, fd, param_values, flags)
#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)

/*
 * Craft a message to send to the kernel module.
 */
struct message *craft_message(int command_type, char *command)
{
    struct message *msg = NULL;

    msg = calloc(1, sizeof(struct message));
    if (NULL == msg) {
        perror("calloc");
        goto exit;
    }

    msg->msg_type = command_type;
    msg->buf_len = strlen(command)+1;
    msg->buf = command;

exit:
    return msg;
}

/*
 * Read response from kernel module (/proc/kernel_util).
 */
int recv_msg_from_kmod()
{
    int rc = -1;
    int fd = -1;
    char buffer[20];
    ssize_t buffer_size = sizeof(buffer);

    printf("Attempting to open proc device %s\n", PROC_FILE_PATH);
    fd = open(PROC_FILE_PATH, O_RDONLY);
    if (-1 == fd) {
        printf("ERROR: failed to open proc device (errno %d)\n", errno);
        goto exit;
    }

    if (0 >= read(fd, buffer, buffer_size)) {
        printf("ERROR: failed to read all bytes from proc device (errno %d)\n", errno);
        goto exit;
    }

    printf("Received response: %s\n", buffer);

    rc = 0;

exit:
    return rc;
}

/*
 * Send the provided message to the kernel module.
 */
int send_msg_to_kmod(struct message *msg)
{
    int rc = -1;
    int fd = -1;
    ssize_t bytes_written = 0;

    // Send message to kmod
    fd = open(PROC_FILE_PATH, O_RDWR);
    if (-1 == fd) {
        printf("ERROR: failed to open proc device (errno %d)\n", errno);
        goto exit;
    }

    // write bytes to proc device
    bytes_written = write(fd, msg, sizeof(*msg));
    if (bytes_written != sizeof(*msg)) {
        printf("Failed to write all bytes to proc device?");
    }

    rc = 0;

exit:
    return rc;
}

/*
 * Load the kernel module.
 */
int load_kernel_module(char *module_path)
{
    int rc = -1;
    int fd = -1;

    // load kmod
    fd = open(module_path, O_RDONLY | O_CLOEXEC);
    if (-1 == fd) {
        printf("ERROR: failed to open kernel module file (errno: %d)", errno);
        goto exit;
    }

    if (0 != finit_module(fd, "", 0)) {
        printf("ERROR: failed to load kernel module (errno: %d)\n", errno);
        goto exit;
    }

    rc = 0;

exit:
    close(fd);
    return rc;
}

/*
 * Remove the kernel module.
 */
int remove_kernel_module()
{
    int rc = -1;

    rc = delete_module(MODULE_NAME, O_NONBLOCK);
    if (0 > rc) {
        perror("rmmod");
        goto exit;
    }

    rc = 0;

exit:
    return rc;
}

/*
 * Create and send message to the kmod, then read the kmod's response from the
 * proc device.
 */
void get_kmod_response_for_message(int msg_type, char *msg_str)
{
    // create message
    struct message *msg = craft_message(msg_type, msg_str);
    if (NULL == msg) {
        printf("ERROR: failed to create message\n");
        goto exit;
    }

    // send message
    if (send_msg_to_kmod(msg)) {
        printf("ERROR: failed to send message to kmod\n");
        goto exit;
    }

    // receive response
    if (recv_msg_from_kmod()) {
        printf("ERROR: failed to get response from proc device\n");
        goto exit;
    }

exit:
    if (msg) {
        free(msg);
    }
}

/*
 * Print usage for the agent
 */
void print_usage(char *prog_name)
{
    printf("%s <filename>\n", prog_name);
}

int main(int argc, char **argv)
{
    int rc = -1;
    int fd = -1;

    printf("Starting agent\n");

    if (2 > argc) {
        printf("ERROR: agent requires module filename as an argument\n");
        print_usage(argv[0]);
        goto exit;
    }

    // load kernel module
    rc = load_kernel_module(argv[1]);
    if (0 > rc) {
       goto exit;
    }

    get_kmod_response_for_message(1, "command one");
    get_kmod_response_for_message(2, "command two");

    // unload kmod
    rc = remove_kernel_module();
    if (0 > rc) {
        printf("ERROR: failed to remove kernel module\n");
        goto exit;
    }

    rc = 0;

exit:
    return rc;
}
