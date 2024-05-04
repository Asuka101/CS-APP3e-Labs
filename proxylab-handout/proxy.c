#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* header structure */
struct header {
    char host[MAXLINE];
    char user_agent[MAXLINE];
    char connection[MAXLINE];
    char proxy_connection[MAXLINE];
};

struct uri {
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
};

void doit(int fd);
void parse_uri(char *uri, struct uri *up);
void generate_header(struct header *hp, struct uri *up, char *requesthdr);
void sigpipe_handler(int sig);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    
    /* check if arguments are valid */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    /* handle the SIGPIPE */
    signal(SIGPIPE, sigpipe_handler);

    /* listen */
    listenfd = Open_listenfd(argv[1]);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accept connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
    return 0;
}

void doit(int fd)
{
    int serverfd;
    rio_t rio, server_rio;
    char buf[MAXBUF], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char requesthdr[MAXLINE];
    struct header *hp;
    struct uri *up;
    size_t n;
    
    /* initialize the rio buffer and read data line by line */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return;
    printf("%s", buf);
    
    sscanf(buf, "%s %s %s", method, uri, version);
    /* whether the method is GET or not */
    if (strcasecmp(method, "GET")) {
        sprintf(buf, "Proxy does not implement this method\r\n");
        Rio_writen(fd, buf, sizeof(buf));
        return;
    }

    /* transfer the request to the server */
    up = (struct uri *)malloc(sizeof(struct uri));
    hp = (struct header *)malloc(sizeof(struct header));
    parse_uri(uri, up);
    generate_header(hp, up, requesthdr);
    serverfd = Open_clientfd(up->host, up->port);
    if (serverfd < 0) {
        sprintf(buf, "Connection faild\n");
        Rio_writen(fd, buf, sizeof(buf));
        return;
    }

    /* modify the version of http */
    strcpy(version, "HTTP/1.0");

    /* GET request */
    sprintf(buf, "%s %s %s\r\n", method, up->path, version);
    Rio_writen(serverfd, buf, sizeof(buf));

    /* request header */
    Rio_writen(serverfd, requesthdr, sizeof(requesthdr));

    free(up);
    free(hp);

    /* receive messages from the serve and transfer them to the client */
    Rio_readinitb(&server_rio, serverfd);
    while((n = Rio_readlineb(&server_rio, buf, MAXLINE)) > 0) {
        printf("Proxy received %d bytes from the server\n", (int)n);
        Rio_writen(fd, buf, n);
    }
    
    Close(serverfd);
}

void parse_uri(char *uri, struct uri *up) 
{
    char *host, *port, *path;
    int portnum;
    host = strstr(uri, "//");
    if (host == NULL) {
        port = strstr(uri, ":");
        if (port != NULL) {
            sscanf(port + 1, "%d%s", &portnum, up->path);
            sprintf(up->port, "%d", portnum);
            *port = '\0';
        }
        else {
            path = strstr(uri, "/");
            if (path != NULL) {
                strcpy(up->path, path);
                *path = '\0';
            }
            strcpy(up->port, "80");
        }
        strcpy(up->host, uri);
    }

    else {
        port = strstr(host + 2, ":");
        if (port != NULL) {
            sscanf(port + 1, "%d%s", &portnum, up->path);
            sprintf(up->port, "%d", portnum);
            *port = '\0';
        }

        else {
            path = strstr(host + 2, "/");
            if (path != NULL) {
                strcpy(up->path, path);
                *path = '\0';
            }
            strcpy(up->port, "80");
        }
        strcpy(up->host, host + 2); 
    }
}

void generate_header(struct header *hp, struct uri *up, char *requesthdr)
{
    sprintf(hp->host, "Host: %s:%s\r\n", up->host, up->port);
    sprintf(hp->user_agent, "%s", user_agent_hdr);
    sprintf(hp->connection, "Connection: Close\r\n");
    sprintf(hp->proxy_connection, "Proxy-Connection: Close\r\n");
    sprintf(requesthdr, "\r\n%s%s%s%s\r\n", hp->host, hp->user_agent, hp->connection, hp->proxy_connection);
}

void sigpipe_handler(int sig)
{
    Sio_puts("SIGPIPE caught\n");
    return;
}