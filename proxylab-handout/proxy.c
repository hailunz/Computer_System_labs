/*
 *  proxylab
 *  Hailun Zhu, ID: hailunz
 *  Xinkai Wang, ID: xinkaiw
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "csapp.h"
#include "cache.h"

//proxy with cache

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr="Connection: close\r\n";
static const char *proxy_con_hdr="Proxy-Connection: close\r\n";

void doit(int fd);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
void parse_requesthdrs(rio_t *rp,char *req);
int parse_uri(char *uri,char *host,char *hostname, char *path,int *req_port);
void *thread(void *vargp);

//cache
cache *cache_head=NULL;


int main(int argc, char **argv)
{
    pthread_t tid;
    //printf("%s%s%s", user_agent_hdr, accept_hdr, accept_encoding_hdr);
    int listenfd, *connfdp, port=0, clientlen;
    struct sockaddr_in clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
	
    Signal(SIGPIPE,SIG_IGN);

    port = atoi(argv[1]);

    if (port==0){
        fprintf(stderr,"wrong port number.\n");
        exit(1);
    }
    
    // cache
    cache_head=init_cache();

    listenfd = Open_listenfd(port);

    if (listenfd == -1){
        fprintf(stderr,"Error:open listenfd\n");
        exit(1);
    }

    clientlen = sizeof(clientaddr);
    
    while (1) {

        printf("main wait\n");
        connfdp=Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
        Pthread_create(&tid,NULL,thread,connfdp);

        printf("end main while\n");
    }

    return 0;
}

/* thread routine*/

void *thread(void *vargp){
    int connfd=*((int *)vargp);
    Pthread_detach(Pthread_self());
    Free(vargp);
    
    doit(connfd);
    
    display_cache(cache_head);
    
    printf("return doit\n");
    Close(connfd);

    return NULL;
}

/*
 * doit - handle one HTTP request/response transaction
 */

void doit(int fd) 
{
    printf("doit %d\n",fd);
    
    char buf[MAX_OBJECT_SIZE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE];
    char path[MAXLINE]="\0";
    int req_port=0;
    char host[MAXLINE];
    
    int serverfd,serverlen;
    struct sockaddr_in serveraddr;

    rio_t rio;
    int err=0;
    char reqhdr[MAX_OBJECT_SIZE];


    //cache
    char cache_buf[MAX_OBJECT_SIZE];
    ssize_t size;
    size=0;
    cache_node *cache_hit=NULL;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);

    if ((err=rio_readlineb(&rio, buf, MAX_OBJECT_SIZE))<0){
        clienterror(fd,"get wrong","400","bad request","bad get");
        return;
    }
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("get[%s]",buf);
    
    if (strcmp(buf,"\0")==0){
        clienterror(fd,"get 0","400","Bad Request","invalid request");
        return;
    }
    if (strcasecmp(method, "GET")) { 
        clienterror(fd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return;
    }

    //parse uri
    if (parse_uri(uri,host,hostname,path,&req_port)<0){
        clienterror(fd,uri,"400","Bad Request","invalid request");
        return;
    }

    printf("uri %s %s %s %s %d\n",uri,host,hostname,path,req_port);

    cache_hit=find_fit(cache_head,uri);
	
    if (cache_hit!=NULL){
        //cache hit
        if (cache_to_client(fd,cache_head,cache_hit)<0){
	    clienterror(fd,"cache","505","read cache error","cache error");}
            return;
    }

    // not hit
    // establish connection with the web server
    serverlen=sizeof(serveraddr);
    serverfd=open_clientfd_r(hostname,req_port);
    printf("server %d\n",serverfd);	

    if (serverfd<=0){
        clienterror(fd,"serverfd","400","bad request","connection error");
        return;
    }	

    //built http request to the server
    sprintf(reqhdr,"GET %s HTTP/1.0\r\n",path);
    strcat(reqhdr,"Host: ");
    strcat(reqhdr,host);
    strcat(reqhdr,"\r\n");
    strcat(reqhdr,user_agent_hdr);
    strcat(reqhdr,accept_hdr);
    strcat(reqhdr,accept_encoding_hdr);

    //send addtional header
    parse_requesthdrs(&rio,reqhdr);
    Rio_writen(serverfd,reqhdr,strlen(reqhdr));

    //read from server
    rio_t server_rio;
    Rio_readinitb(&server_rio,serverfd);
    char read_server[MAX_OBJECT_SIZE];
    ssize_t length = 0;
    cache_node *new_node = NULL;

    while((length=Rio_readnb(&server_rio,read_server,MAX_OBJECT_SIZE))>0){
        printf("length%lu\n",length);
        //printf("%s",read_server);
        //forward to the client
        Rio_writen(fd,read_server,length);
        size+=length;
        if (size<=MAX_OBJECT_SIZE){
            sprintf(cache_buf,"%s",read_server);
        }
    }
    printf("buf size%lu\n",size);
    //Rio_writen(fd,read_server,MAXLINE);
    
    if (size<=MAX_OBJECT_SIZE){
        new_node=build_node(uri,size,cache_buf);
        printf("new node%p %lu\n",new_node,new_node->size);
        server_to_cache(cache_head,new_node);
    }

    Close(serverfd);

    return;
}


/*
 * read_requesthdrs - build HTTP request
 */

void parse_requesthdrs(rio_t *rp,char *req) 
{	
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);

    printf("web r%s",buf);
    while(strcmp(buf, "\r\n")) {
        printf("web r%s",buf);
        if (strstr(buf,"Host:")!=NULL){
            Rio_readlineb(rp, buf, MAXLINE);	
            continue;
        }

        else if (strstr(buf,"User-Agent:")!=NULL){
            Rio_readlineb(rp, buf, MAXLINE);
            continue;    
        }

        else if (strstr(buf,"Accept:")!=NULL){
            Rio_readlineb(rp, buf, MAXLINE);
            continue;
        }
        else if (strstr(buf,"Accept-Encoding:")!=NULL){
            Rio_readlineb(rp, buf, MAXLINE);
            continue;
        }
        else if (strstr(buf,"Connection:")!=NULL){
            Rio_readlineb(rp, buf, MAXLINE);
            continue;
        }
        else {
            strcat(req,buf);
            Rio_readlineb(rp, buf, MAXLINE);
        }

    }
    strcat(req,connection_hdr);
    strcat(req,proxy_con_hdr);
    strcat(req,"\r\n");
    printf("req %s",req);

    return;
}

    
int parse_uri(char *uri,char *host, char *hostname, char *path,int *req_port){
	
    if (strncasecmp(uri,"http://",7)){
        //error
        return -1;
    }

    char protocol[MAXLINE];
    char port[MAXLINE];
    //char *host;
    sscanf(uri, "%[^:]://%[^/]%s", protocol, host, path);
    
    if (strcmp(path,"\0")==0){
        sprintf(path,"/");
    }
    
    // check if has port
    char *port_begin;
    port_begin=strchr(host,':');
//	printf("port%s\n",port_begin);

    //has port
    if (port_begin!=NULL){
        strcpy(port,port_begin+1);
        sscanf(host,"%[^:]",hostname);

        printf("hostname:%s port:%s\n",hostname, port);	
    }
    else {
        strcpy(port,"80");//default port
        strcpy(hostname,host);
        printf("hostname%s\n",hostname);	
    }
    *req_port=atoi(port);

    return 0;
}

/*
 * clienterror - returns an error message to the client
 */

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];
    printf("client error %s\n",cause);
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
