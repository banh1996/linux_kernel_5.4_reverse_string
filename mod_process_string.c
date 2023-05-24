
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <linux/kthread.h>

#define DEBUG_LOG printk
#define DRIVER_AUTHOR "TRAN NGOC HUNG"
#define DRIVER_DESC "Test module for reverse string"

#define MAX_BUF_LEN     1024

#define SERVER_PORT  6789
#define COMMON_ADDR "127.0.0.1"

static struct task_struct *server_thread = NULL;
static struct socket *gp_sock = NULL;


static int __init create_server_socket(void)
{
    int result;
    struct sockaddr_in addr_server;

    result = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &gp_sock);
    if (result < 0) {
        pr_err("Failed to create server socket: %d\n", result);
        return result;
    }

    memset(&addr_server, 0, sizeof(struct sockaddr_in));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);  // Bind to all available interfaces
    addr_server.sin_port = htons(SERVER_PORT);

    // bind
    result = gp_sock->ops->bind(gp_sock, (struct sockaddr *)&addr_server, sizeof(struct sockaddr_in));
    if (result < 0)
    {
        DEBUG_LOG(KERN_INFO "Error bind: %d\n", result);
        sock_release(gp_sock);
        return result;
    }
    else
        DEBUG_LOG(KERN_INFO "Bind socket success\n");

    return result;
}

static int reverse_string(char *in_data, char *out_data, int len)
{
    int i;
    for (i = 0; i < len; i++)
        *(out_data + i) = *(in_data + ((len - 2) - i));

    return 0;
}

static int receive_message_from_client(void *pv)
{
    int result = 0;
    int data_receive_len;
    
    struct msghdr msg_receive;
    struct iovec  iov_receive;
    struct sockaddr_in server_addr;
    mm_segment_t oldfs;
    char *data_reverse;
    char *data_receive;

    //init buffer for receiving
    if ((data_reverse = kmalloc(MAX_BUF_LEN, GFP_KERNEL)) == NULL)
        return -1;
    if ((data_receive = kmalloc(MAX_BUF_LEN, GFP_KERNEL)) == NULL)
    {
        kfree(data_reverse);
        return -1;
    }

    allow_signal(SIGKILL|SIGTERM|SIGSTOP);
    set_current_state(TASK_INTERRUPTIBLE);

    while (!kthread_should_stop()) {
        // Create msg_receive
        memset(&msg_receive, 0, sizeof(struct msghdr));
        memset(&iov_receive, 0, sizeof(struct iovec));

        msg_receive.msg_name = &server_addr;
        msg_receive.msg_namelen = sizeof(struct sockaddr_in);
        msg_receive.msg_control = NULL;
        msg_receive.msg_controllen = 0;
        msg_receive.msg_flags = 0;

        iov_receive.iov_base = data_receive;
        iov_receive.iov_len = MAX_BUF_LEN;
        iov_iter_init(&msg_receive.msg_iter, READ, &iov_receive, 1, MAX_BUF_LEN + 1);

        // Receive message
        oldfs = get_fs();
        set_fs(KERNEL_DS);
        memset(data_reverse, 0, MAX_BUF_LEN);
        memset(data_receive, 0, MAX_BUF_LEN);
        DEBUG_LOG(KERN_INFO "Waiting for message coming .......\n");
        result = gp_sock->ops->recvmsg(gp_sock, (struct msghdr *)&msg_receive, sizeof(struct msghdr), 0);
        if (result < 0)
        {
            DEBUG_LOG(KERN_INFO "Error Receive message: %d\n", result);
            break;
        }
        else if (result == 0)
        {
            DEBUG_LOG(KERN_INFO "sock shutdown: %d\n", result);
            break;
        }

        set_fs(oldfs);
        iov_receive = iov_iter_iovec(&msg_receive.msg_iter);
        data_receive_len = result;
        DEBUG_LOG(KERN_INFO "Receive message len: %d and %s\n", data_receive_len, data_receive);

        /*reverse string*/
        reverse_string(data_receive, data_reverse, data_receive_len);
        iov_receive.iov_base = data_reverse;
        iov_iter_init(&msg_receive.msg_iter, READ, &iov_receive, 1, data_receive_len);
        /*send back reverse string to client*/
        sock_sendmsg(gp_sock, &msg_receive);
    }

    /* free mem */
    if (data_reverse)
        kfree(data_reverse);
    if (data_receive)
        kfree(data_receive);

    DEBUG_LOG(KERN_INFO "End thread\n");

    return result;
}


static int __init init_hello(void)
{
    //Init socket
    if ((gp_sock = (struct socket *)kmalloc(sizeof(struct socket), GFP_KERNEL)) == NULL)
    {
        DEBUG_LOG(KERN_INFO "Error alloc memmory sock");
        return -1;
    }
    else
        DEBUG_LOG(KERN_INFO "kmalloc \"sock\", size: %ld\n", sizeof(struct socket));

    if (create_server_socket() < 0)
        return -1;

    // Start the server thread
    server_thread = kthread_run(receive_message_from_client, NULL, "server_thread");
    if (IS_ERR(server_thread)) {
        printk(KERN_ERR "Failed to start server thread\n");
        if (gp_sock) {
            sock_release(gp_sock);
            gp_sock = NULL;
        }
        return PTR_ERR(server_thread);
    }

    return 0;
}

static void __exit exit_hello(void)
{
    if (gp_sock) {
        //kernel_sock_shutdown(gp_sock, SHUT_RDWR);
        gp_sock->ops->shutdown(gp_sock, SHUT_RDWR);
    }

    // Stop the server thread
    if (server_thread) {
        DEBUG_LOG(KERN_INFO "stopping kthread\n");
        kthread_stop(server_thread);
        server_thread = NULL;
    }

    if (gp_sock) {
        sock_release(gp_sock);
        gp_sock = NULL;
    }

    DEBUG_LOG("Goodbye module\n");
}

module_init(init_hello);
module_exit(exit_hello);

MODULE_LICENSE("GPL");                 /* license */
MODULE_AUTHOR(DRIVER_AUTHOR);          /* author */
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_SUPPORTED_DEVICE("testdevice");
