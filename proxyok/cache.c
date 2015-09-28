/*  proxylab-cache.c
 *  cache implementation
 *  Hailun Zhu, ID: hailunz
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "csapp.h"
#include "cache.h"


pthread_rwlock_t rdlock,wrlock;

/*
 * init_cache - init a cache list
 */
cache *init_cache() {
printf("init cache\n");
   cache *list = (cache *)malloc(sizeof(cache));
    list->header = NULL;
    list->end = NULL;
    list->remain_size = MAX_CACHE_SIZE;
    pthread_rwlock_init(&rdlock,NULL);	
    pthread_rwlock_init(&wrlock,NULL);	
    return list;
}

void display_cache(cache *head){
cache_node *node=head->header;
printf("cache head %p\n",node);
while(node!=NULL){
printf("cblock [%s] [%p]\n ",node->uri,node->next);
node=node->next;
}
printf("cache end %p\n",head->end);

}
/*
 * search cache note by uri
 */
cache_node *search_cache_node(cache *list, char *uri) {
printf("search cache\n");
	pthread_rwlock_rdlock(&rdlock);
	cache_node *node = list->header;
    while (node != NULL) {
        if (strcmp(node->uri, uri) == 0) {
            return node;
		}
        node = node->next;
    }

	pthread_rwlock_unlock(&rdlock);
    return NULL;
}

/*
 *
 * * update_node 
 */
int update_node(cache *list, char *uri) {
printf("update node\n");
    cache_node *node = list->header;
	cache_node *prev_node = NULL;

    while (node != NULL) {
        if (strcmp(node->uri, uri) == 0) {
			
			if (list->end == node) {
				return 0;
            }
			else {

				if (list->header == node){
				list->header=node->next;
			//	return node;
				}

				else {
					prev_node->next=node->next;
				}
				list->end->next = node;
				list->end = node;
				node->next = NULL;
				list->remain_size += node->size;
				return 0;
			}
		}
        prev_node = node;
        node = node->next;
    }
    return 0;
}

/*
 * cache_to_client
 * read from cache and send to client
 */
int cache_to_client(int clientfd, cache *list, cache_node *node) {
printf(" cache to client\n");
    if (list == NULL) {
		return -1;
    }
	if (node == NULL){
		return -1;
	}
	pthread_rwlock_rdlock(&rdlock);
	if (rio_writen(clientfd, node->content,node->size)<0){
		return -1;
	}
	pthread_rwlock_unlock(&rdlock);
	
	//lru
	pthread_rwlock_wrlock(&wrlock);
	update_node(list,node->uri);
	pthread_rwlock_unlock(&wrlock);

	return 0;
}

/*
 * build_node - build a cache node
 */
cache_node *build_node(char *uri, ssize_t size,char *content) {
printf("build node\n");
pthread_rwlock_wrlock(&wrlock);
    cache_node *node = (cache_node *)malloc(sizeof(cache_node));

    if (node == NULL) {
        return NULL;
    }
    strcpy(node->uri,uri); 
    if (node->uri == NULL) {
        return NULL;
    }
    node->size = size;

printf("node size%p %lu\n",node,node->size);
	strcpy(node->content,content);

    if (node->content == NULL) {
        return NULL;
    }

    node->next = NULL;
pthread_rwlock_unlock(&wrlock);
    return node;
}

/*
 * add_cache node 
 * * Add a cache node to the end of the cache list
 */
int add_cache_node(cache *list,cache_node *node) {
printf("add cache node\n");
	
	//pthread_rwlock_wrlock(&lock);

	if (list->header == NULL) {
        list->header = list->end = node;
        list->remain_size -= node->size;
		return -1;
    } 

	
	else if ((list->remain_size)>= (node->size)){
        list->end->next = node;
        list->end = node;
		node->next=NULL;
        list->remain_size -= node->size;
		return -1;
    }
	return 0;
//pthread_rwlock_unlock(&lock);
}
/*
 * delete_cache_node
 */
void delete_cache_node(cache *list) {
printf("delte cache\n");
   
	cache_node *node = list->header;

    if (node == NULL) {
        return ;
    }

	 if (node == list->end) {
        list->header=NULL;
		list->end = NULL;
    }

    list->header = node->next;
    list->remain_size += node->size;
Free(node);

    return ;
}
/*
 * add_cache: use eviction
 * the least used cache block is stored at the front(head)
 */
void server_to_cache(cache *list, cache_node *node) {
printf("server to cache\n");
 printf("remain size%lu %lu\n",list->remain_size,node->size);   
	pthread_rwlock_wrlock(&wrlock);
//if ((node->size))
    while (list->remain_size < node->size) {
		delete_cache_node(list);
    }

    add_cache_node(list, node);
    
	pthread_rwlock_unlock(&wrlock);
}
