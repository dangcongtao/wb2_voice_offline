/**
 * @file tcp_server.c
 * @author your name (you@domain.com)
 * @brief TCP server
 * @version 0.1
 * @date 2022-10-13
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <FreeRTOS.h>
#include <task.h>
#include <lwip/sockets.h>
#include <blog.h>
#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "tcp_server.h"

#define MAX_CLIENT_NUM 20
static struct sockaddr_in serv_addr;
static tcp_client_msg_t tcp_client;
/**
 * @brief tcp_server_init
 *        TCP Server Initialization Function
 * @param s_ip server ip addr,The default(NULL) is the IP address of the wifi statio
 * @param s_port server port
 * @return Network identifier:socket fd
 */
int socketfd = 0;
int tcp_server_init(char *s_ip, int s_port)
{
    /* Configure the server address */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(s_port);

    // Creat socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
        return -1;

    // bind IP addr
    int ret = bind(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0)
    {
        printf("Socket unable to bind: errno %d\r\n", ret);
        return -1;
    }
    printf("tcp server start ip:%s:%d\r\n", inet_ntoa(serv_addr.sin_addr.s_addr), ntohs(serv_addr.sin_port));

    return socketfd;
}

int tcp_server_listen(int socketfd)
{
    // listening connections.The maximum number of connections is 4
    int ret = listen(socketfd, MAX_CLIENT_NUM);
    if (ret != 0)
    {
        printf("Error occured during listen: errno %d\r\n", ret);
        return -1;
    }
    printf("tcp server listening.....\r\n");

    return 0;
}

/**
 * @brief tcp_server_accept
 *
 * @param socketfd
 * @param tcp_accpet_cb
 * @return int
 */
static int sock_fd[MAX_CLIENT_NUM];
int tcp_server_accept(int socketfd, tcp_accpet_t tcp_accpet_cb)
{
    struct sockaddr_in s_addr;
    int conn_sock = 0;
    int i = 0;
    u32_t socket_len = sizeof(s_addr);

    conn_sock = accept(socketfd, (struct sockaddr *)&s_addr, &socket_len);
    if (conn_sock < 0)
    {
        printf("Unable to accept connection: errno %d\r\n", conn_sock);
        return -1;
    }
    else if (conn_sock > 0)
    {
        printf("accept connection success\r\n");
        for (i = 0; i < MAX_CLIENT_NUM; i++)
        {
            if (sock_fd[i] == 0)
            {
                sock_fd[i] = conn_sock;
                tcp_client.socket_id = i;
                tcp_client.socket_fd = conn_sock;
                tcp_client.ip_addr = (void *)&s_addr;
        
                printf("client:%s, socket: %d, id:%d\r\n", inet_ntoa(s_addr.sin_addr.s_addr), conn_sock, i);
                
                // xTaskCreate(tcp_accpet_cb, "tcp_accpet_cb", 1024, &tcp_client, 17, NULL);
                tcp_accpet_cb(&tcp_client);

                return 0;
            }
        }
    }

    // tcp_server_deinit();
    return 0;
}
/**
 * @brief tcp_server_send
 *
 * @param socket_fd
 * @param data
 * @return int
 */
int tcp_server_send(int socket_id, char *data)
{
    return write(sock_fd[socket_id], data, strlen(data));
}

/**
 * @brief tcp_server_receive
 *
 * @param socket_fd
 * @param recv_data
 * @return int
 */
int tcp_server_receive(int socket_id, char *recv_data)
{
    if (socket_id > MAX_CLIENT_NUM)
        return -1;

    return read(sock_fd[socket_id], recv_data, 1024);
}

/**
 * @brief tcp_server_close
 *
 * @param socket_id
 * @return int
 */
int tcp_server_close(int socket_id)
{
    if (socket_id > MAX_CLIENT_NUM)
        return -1;

    printf("tcp server close socket_id:%d, socket:%d\r\n", socket_id, sock_fd[socket_id]);
    int ret = close(sock_fd[socket_id]);
    sock_fd[socket_id] = 0;

    return ret;
}
/**
 * @brief tcp_server_deinit
 *
 * @return int
 */
int tcp_server_deinit(void)
{
    return close(socketfd);
}