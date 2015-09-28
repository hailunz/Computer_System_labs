/**************************************************************
Lab: Cache Simulator(cachelab-partA)
name: Hailun Zhu
andrew ID: hailunz
email: hailunz@andrew.cmu.edu
 **********************************************************************/
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/*define line*/
typedef struct{
	int valid;
	unsigned long int tag;
	int lastTime;
}line;

/*define set*/
typedef struct{
	line* lines;
}set;

/*define cache*/
typedef struct{
	set* sets;
}cache;

/*arguments*/
int hits,misses,evicts;
int s,b,E;
int S,B;
int clocktime=0;

/*get tag */
unsigned long int getTag(unsigned long int addr,int s, int b){
	unsigned long int tag;
	tag=addr>>(s+b);
	return tag;
}

/* get Set index*/
unsigned long int getSet(unsigned long int addr, int s, int b){
	unsigned long int set;
	int t;
	t=64-b-s;
	set=(addr<<t)>>(b+t);
	return set;
}

/* initial cache*/
cache iniCache(int S,int lineNum){
	cache newCa;
	set newSet;
	int setIdx;
	int lineIdx;

	newCa.sets=(set*) malloc(sizeof(set)*S);

	for	(setIdx=0;setIdx<S;setIdx++){
		
		newSet.lines=(line*) malloc(sizeof(line)*lineNum);
		newCa.sets[setIdx]=newSet;

		for (lineIdx=0;lineIdx<lineNum;lineIdx++){
		
			newSet.lines[lineIdx].valid=0;
			newSet.lines[lineIdx].tag=0;
			newSet.lines[lineIdx].lastTime=0;
		}
	}

	return newCa;

}

/* free cache*/
void freeCache(cache caSim,int S){
	set newSet;
	int setIdx;

	for (setIdx=0;setIdx<S;setIdx++){
			
		newSet=caSim.sets[setIdx];
		if (newSet.lines!=NULL)
			free (newSet.lines);
		}

	if (caSim.sets!=NULL)
		free(caSim.sets);
}

/* if cache is not full,then find the index of empty line */
int findEmpIdx(set newSet,int lineNum){
	int lineIdx;
	line caLine;
	int empIdx=0;
	
	for (lineIdx=0;lineIdx<lineNum;lineIdx++){
		
		caLine=newSet.lines[lineIdx];
		
		if (caLine.valid==0){
			empIdx=lineIdx;		
			return empIdx;
		}
	}
	return -1;
}

/* if cache is full, find the LRU index */
int findEvtIdx(set newSet,int lineNum){
	int lineIdx;
	line caLine;
	int leastTime=0;
	int lruIdx=0;

	for (lineIdx=0;lineIdx<lineNum;lineIdx++){
		
		caLine=newSet.lines[lineIdx];
		
		if (lineIdx==0){
			leastTime=caLine.lastTime;
		}
		
		else if (caLine.lastTime<leastTime){
			lruIdx=lineIdx;
			leastTime=caLine.lastTime;
			}
		}
	
	return lruIdx;
}

/* if L/S, a data load or store; in S, only when misses then load */
void getLS(cache caSim, unsigned long int addr){
	int lineIdx;
	int cacheFull=1;
	int lineNum=E;
	int preHits=hits;
	unsigned long int addrTag;
	int addrSet;
	set reqSet;
	line caLine;
	int emptLineIdx;
	int lruIdx;

	addrTag=getTag(addr,s,b);
	addrSet=(int) getSet(addr,s,b);

	reqSet=caSim.sets[addrSet];

	for (lineIdx=0;lineIdx<lineNum;lineIdx++){
		caLine=reqSet.lines[lineIdx];
		
		if (caLine.valid&&(caLine.tag==addrTag)){
			hits++;
			reqSet.lines[lineIdx].lastTime= ++clocktime;
		}
	
		else if (!caLine.valid){
			cacheFull=0;
		}
		
	}
	
	if (hits!=preHits){
		return ;
	}

	/*if preHits== hits, then miss or evict,count it*/	
	else {
		misses++;
		if (cacheFull){
			evicts++;
		}
	}
	
	/* if not hit, find the empty line index or LRU index*/
	if (!cacheFull){
		emptLineIdx=findEmpIdx(reqSet,lineNum);
		reqSet.lines[emptLineIdx].valid=1;
		reqSet.lines[emptLineIdx].tag=addrTag;
		reqSet.lines[emptLineIdx].lastTime= ++clocktime;
	}

	else if(cacheFull) {
 		lruIdx=findEvtIdx(reqSet,lineNum);
		reqSet.lines[lruIdx].tag=addrTag;
		reqSet.lines[lruIdx].lastTime= ++clocktime;
	}
	
}


int main(int argc, char **argv){
	
	cache cSim;

	FILE * readFile;
	char * fileName;

	char mType;
	long unsigned int address;
	int size;

	int opt;
	
	s=0;
	b=0;
	E=0;
	hits=0;
	misses=0;
	evicts=0;

	while(-1 != (opt=getopt(argc,argv, "s:E:b:t:"))){
		switch(opt){
			case 's':
				s=atoi(optarg);
				break;
			case 'E':
				E=atoi(optarg);
				break;
			case 'b':
				b=atoi(optarg);
				break;
			case 't':
				fileName=optarg;
				break;
			default:
				printf("wrong argument!\n");
				break;
		}
	}

	S=1<<s;
	B=1<<b;

	/* initial the cache. */	
	cSim=iniCache(S,E);
	
	/* read File */
	readFile=fopen(fileName,"r");
	
	if (readFile==NULL){
		printf("file open error!\n");
		return -1;
	}
	if (readFile!=NULL){
		while ((fscanf(readFile," %c %lx,%d",&mType,&address,&size))>0){
			switch(mType){
				case 'I':
					break;
				case 'L':
					getLS(cSim,address);
					break;
				case 'M':
					getLS(cSim,address);
					getLS(cSim,address);
					break;
				case 'S':
					getLS(cSim,address);
					break;
				default:
					break;
			}
		}
	}

	printSummary(hits,misses,evicts);
	freeCache(cSim,S);
	fclose(readFile);
	return 0;
}

