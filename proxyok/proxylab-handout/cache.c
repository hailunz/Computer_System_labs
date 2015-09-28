/*  
 *  proxylab-cache.c
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
 * init_cache 
 * initial a cache list head
 */
cache *init_cache() {
    
    printf("init cache\n");
    
    pthread_rwlock_init(&rdlock,NULL);
    pthread_rwlock_init(&wrlock,NULL);
    
    cache *head = (cache *)malloc(sizeof(cache));
    head->header = NULL;
    head->end = NULL;
    head->remain_size = MAX_CACHE_SIZE;
    
    return head;
}

/* 
 * display the cache
 */
void display_cache(cache *head){
    cache_node *node=head->header;
    printf("cache head %p\n",node);
    
    while(node!=NULL){
        printf("cache block [%s] [%p]\n ",node->uri,node->next);
        node=node->next;
    }
    printf("cache end %p\n",head->end);
}

/*
 * search cache note by uri
 */
cache_node *find_fit(cache *list_head, char *uri) {
    
    printf("search cache\n");
    
	pthread_rwlock_rdlock(&rdlock);
	cache_node *node = list_head->header;
    
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
 * update_node :
 * if read the cache, update the list.
 * the node should be added at the end of the list
 * it is the most recently used cache block.
 */
int update_node(cache *list_head, char *uri) {
    
    printf("update node\n");
    cache_node *node = list_head->header;
	cache_node *prev_node = NULL;

    while (node != NULL) {
        if (strcmp(node->uri, uri) == 0) {
			
            // no need to update
			if (list_head->end == node) {
				return 0;
            }
			else {

				if (list_head->header == node){
                    list_head->header=node->next;
				}

				else {
					prev_node->next=node->next;
				}
				list_head->end->next = node;
				list_head->end = node;
				node->next = NULL;
				list_head->remain_size += node->size;
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
int cache_to_client(int clientfd, cache *list_head, cache_node *node) {
    
    printf(" cache to client\n");
    
    if ((list_head == NULL)||(node == NULL)){
		return -1;
    }

	pthread_rwlock_rdlock(&rdlock);
	if (rio_writen(clientfd, node->content,node->size)<0){
		return -1;
	}
	pthread_rwlock_unlock(&rdlock);
	
	//lru
	pthread_rwlock_wrlock(&wrlock);
	update_node(list_head,node->uri);
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
    node->size = size;
    //printf("node size%p %lu\n",node,node->size);
    strcpy(node->content,content);
    node->next = NULL;
    
    pthread_rwlock_unlock(&wrlock);
    return node;
}

/*
 * add_cache node 
 * add a cache node to the end of the cache list
 */
int add_cache_node(cache *list_head,cache_node *node) {
    printf("add cache node\n");
	
	//pthread_rwlock_wrlock(&lock);
    // have no cache
	if (list_head->header == NULL) {
        list_head->header =node;
        list_head->end = node;
        list_head->remain_size -= node->size;
		return 0;
    } 
    
    // add at the end of the cache list
	else if ((list_head->remain_size)>= (node->size)){
        node->next=NULL;
        
        list_head->end->next = node;
        list_head->end = node;
		
        list_head->remain_size -= node->size;
		return 0;
    }
    
	return -1;
    //pthread_rwlock_unlock(&lock);
}

/*
 * delete_cache_node
 * delete the first cache block
 */
int delete_cache_node(cache *list_head) {
    printf("delete cache\n");
   
	cache_node *node = list_head->header;
    
    // no cache
    if (node == NULL) {
        return -1;
    }
   
    //only one cache block
    if (list_head->header == list_head->end) {
       
        list_head->header=NULL;
		list_head->end = NULL;
    }
    
    
    list_head->header = node->next;
    list_head->remain_size += node->size;
    
    Free(node);

    return 0;
}

/*
 * add_cache: use eviction
 * the least used cache block is stored at the front(head), 
 * the most recently used cache block is stored at the end.
 */
void server_to_cache(cache *list_head, cache_node *node) {
   
    printf("server to cache\n");
    printf("remain size%lu %lu\n",list_head->remain_size,node->size);
    
	pthread_rwlock_wrlock(&wrlock);

    while ((list_head->remain_size) < (node->size)) {
		
        delete_cache_node(list_head);
    }

   add_cache_node(list_head, node);
    
	pthread_rwlock_unlock(&wrlock);
}
