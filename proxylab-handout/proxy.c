/*
 * implementation of the proxy
 */
#include <stdio.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* number of threads and buf size and number of cachelines */
#define THREADNUM 4
#define SBUFSIZE 16
#define CACHELINE_NUM 6

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* sbuf and cache memory */
sbuf_t sbuf;
Cache *cache;

/* header structure */
struct header {
    char host[MAXLINE];
    char user_agent[MAXLINE];
    char connection[MAXLINE];
    char proxy_connection[MAXLINE];
};

/* uri structure */
struct uri {
    char host[MAXLINE];
    char port[MAXLINE];
    char path[MAXLINE];
};

void *thread(void *vargp);
void doit(int fd);
void parse_uri(char *uri, struct uri *up);
void generate_header(struct header *hp, struct uri *up, char *requesthdr);
void sigpipe_handler(int sig);

int main(int argc, char **argv)
{
    int i, listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    
    /* check if arguments are valid */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    /* handle the SIGPIPE */
    signal(SIGPIPE, sigpipe_handler);

    /* listen */
    listenfd = Open_listenfd(argv[1]);
    
    sbuf_init(&sbuf, SBUFSIZE);
    for (i = 0; i < THREADNUM; i++)
        Pthread_create(&tid, NULL, thread, NULL);

    /* initialize the cache */
    cache = malloc(sizeof(Cache));
    cache_init(cache, CACHELINE_NUM);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        sbuf_insert(&sbuf, connfd);
    }
    return 0;
}

void *thread(void *vargp)
{
    Pthread_detach(pthread_self());
    while (1) {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
        Close(connfd);
    }
}

/* process the request message */
void doit(int fd)
{
    int serverfd, object_size;
    rio_t rio, server_rio;
    char buf[MAXBUF], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char requesthdr[MAXLINE];
    char *object_cache;
    char uri_tmp[MAXLINE];
    char object_buf[MAX_OBJECT_SIZE];
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
        printf("Proxy does not implement this method\r\n");
        return;
    }

    /* cache hit */
    if ((object_cache = cache_fetch(cache, uri, fd)) != NULL)
        return;

    /* transfer the request to the server */
    up = (struct uri *)malloc(sizeof(struct uri));
    hp = (struct header *)malloc(sizeof(struct header));
    strcpy(uri_tmp, uri);
    parse_uri(uri_tmp, up);
    generate_header(hp, up, requesthdr);
    serverfd = Open_clientfd(up->host, up->port);
    if (serverfd < 0) {
        sprintf(buf, "Connection faild\r\n");
        Rio_writen(fd, buf, strlen(buf));
        return;
    }

    /* modify the version of http */
    strcpy(version, "HTTP/1.0");

    /* GET request */
    sprintf(buf, "%s %s %s\r\n", method, up->path, version);
    Rio_writen(serverfd, buf, strlen(buf));

    /* request header */
    Rio_writen(serverfd, requesthdr, strlen(requesthdr));

    /* free the heap memory to avoid memory leak */
    free(up);
    free(hp);

    /* receive messages from the serve and transfer them to the client */
    Rio_readinitb(&server_rio, serverfd);
    object_size = 0;
    while((n = Rio_readlineb(&server_rio, buf, MAXLINE)) > 0) {
        object_size += (int)n;
        printf("Proxy received %d bytes from the server\n", (int)n);
        /* fill in the buffer if the total size of file is valid in cache */
        if (n <= MAX_OBJECT_SIZE)
            strcat(object_buf, buf);
        Rio_writen(fd, buf, n);
    }

    /* cache valid object */
    if (object_size <= MAX_OBJECT_SIZE)
        object_insert(cache, uri, object_buf);

    Close(serverfd);
}

/* extract useful information and details from the uri for further processing */
void parse_uri(char *uri, struct uri *up) 
{
    char *host, *port, *path;
    int portnum;

    host = strstr(uri, "//");
    
    /* uri without prefix "http://" */
    if (host == NULL) {
        port = strstr(uri, ":");
        /* port is defined */
        if (port != NULL) {
            sscanf(port + 1, "%d%s", &portnum, up->path);   // extract port and path in char
            sprintf(up->port, "%d", portnum);               // convert the port from char form into number
            *port = '\0';
        }
        /* port is not defined */
        else {
            path = strstr(uri, "/");    // locate the path
            if (path != NULL) {
                strcpy(up->path, path);
                *path = '\0';
            }
            strcpy(up->port, "80");     // use defalut port 80
        }
        strcpy(up->host, uri);
    }

    /* uri with prefix "http://" */
    else {
        port = strstr(host + 2, ":");   // locate the true position of host
        /* the same as mentioned above */
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

/* generate the header which is sent at first */
void generate_header(struct header *hp, struct uri *up, char *requesthdr)
{
    sprintf(hp->host, "Host: %s:%s\r\n", up->host, up->port);
    sprintf(hp->user_agent, "%s", user_agent_hdr);
    sprintf(hp->connection, "Connection: Close\r\n");
    sprintf(hp->proxy_connection, "Proxy-Connection: Close\r\n");
    sprintf(requesthdr, "\r\n%s%s%s%s\r\n", hp->host, hp->user_agent, hp->connection, hp->proxy_connection);
}

/* ignore SIGPIPE */
void sigpipe_handler(int sig)
{
    Sio_puts("Broken pipe\n");
    return;
}