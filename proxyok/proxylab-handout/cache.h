/*
 * cache.h: 
 * header for cache.c
 */

#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_node{
    char uri[MAXLINE];
    ssize_t size;
    struct cache_node *next;
    char content[MAX_OBJECT_SIZE];
} cache_node;

typedef struct cache{
    cache_node *header;
    cache_node *end;
    ssize_t remain_size;
} cache;

/* cache helper functions*/
void display_cache(cache *head);

cache *init_cache();

cache_node *find_fit(cache *list_head, char *uri);
int update_node(cache *list_head, char *uri);
int cache_to_client(int clientfd, cache *list_head, cache_node *node) ;
cache_node *build_node(char *uri, ssize_t size,char *content) ;
int add_cache_node(cache *list_head,cache_node *node);
int delete_cache_node(cache *list_head);
void server_to_cache(cache *list_head, cache_node *node);
