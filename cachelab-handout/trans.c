/*
 * lab: cachelab-part B
 * name: Hailun Zhu
 * Andrew ID: hailunz
 *
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]){
    
	REQUIRES(M > 0);
   	REQUIRES(N > 0);

	int i,j,col,row,tmp,tmp1,tmp2,tmp3,tmp4,tmp5,tmp6,tmp7;

	/* 32*32, blocksize 8 */
	if ((N==32)&&(M==32)){

	for (row=0; row<N; row+=8){
		for (col=0; col<M; col+=8){
			for (i=row; i<row+8; i++){
				for (j=col; j<col+8; j++){
						if (i!=j){
							B[j][i]=A[i][j];
						}
						else {
							tmp=A[i][j];
						}
				}

				/* if diag,to reduce conflicts*/
				if (row==col){
				B[i][i]=tmp;
				}
			}	

		}
	}

}
	/* 64*64, blocksize 8, use B to store extra data in A;
	* cache could only hold 4 lines, so in the 8*8 block, use 4 sub blocks; 
	* sub-block is 4*4;
	* simply use 4*4 block, the misses will exceed the limit.	
	*/
else if ((N==64)&&(M==64)){

	for (row=0; row<N; row+=8){
        	 for (col=0; col<M; col+=8){
	        	for (i=row; i<row+4; i++){
        		j=col;	            
		/* transpose 1st sub block, store 2nd sub block*/ 
			tmp=A[i][j++];
			tmp1=A[i][j++];
			tmp2=A[i][j++];
			tmp3=A[i][j++];
			tmp4=A[i][j++];		
			tmp5=A[i][j++];
			tmp6=A[i][j++];	
			tmp7=A[i][j++];
                   
			B[col][i]=tmp;
                	B[col+1][i]=tmp1;
                	B[col+2][i]=tmp2;
               		B[col+3][i]=tmp3;

                /*buffer data*/   
			B[col][i+4]=tmp4;
                	B[col+1][i+4]=tmp5;
                	B[col+2][i+4]=tmp6;
               		B[col+3][i+4]=tmp7;
    }
                	for (i=row,j=col; i<row+4; i++){
			//j=col;				
		/* transpose 2nd and 3rd sub block,*/	
		
			tmp=A[i+4][col];
        	        tmp1=A[i+4][col+1];
                	tmp2=A[i+4][col+2];
			tmp3=A[i+4][col+3];
			
			tmp4=B[j][row+4];
             		tmp5=B[j][row+5];
                	tmp6=B[j][row+6];
        	        tmp7=B[j][row+7];		
	
		/*transpose*/  
              		B[col][i+4]=tmp;
               		B[col+1][i+4]=tmp1;
                	B[col+2][i+4]=tmp2;
                	B[col+3][i+4]=tmp3;
		   
		B[j+4][row]=tmp4;
                	B[j+4][row+1]=tmp5;
                	B[j+4][row+2]=tmp6;
                	B[j+4][row+3]=tmp7;
j++;
			}

		for (i=row+4; i<row+8; i++){
	 	    	for(j=col+4;j<col+8;j++){

		/* transpose 4th sub block*/
		   if(i!=j){
			B[j][i]= A[i][j];
			}
			else tmp=A[i][j];
                }
			if(row==col){
			B[i][i]=tmp;
			}
                }
            }
        }
	
}

	/*61*67, blocksize of 17, because it's irregular*/
	else if ((M==61)&&(N==67)){

	for (row=0; row<N; row+=18){
		for (col=0; col<M; col+=17){	
			for (i=row; (i<row+18)&&(i<N); i++){
				for (j=col; (j<col+17)&&(j<M); j++){
						B[j][i]=A[i][j];
				}
			}
		}
	}
}
	else{
		printf("wrong matrix size!\n");
	}

    ENSURES(is_transpose(M, N, A, B));
}


/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}


/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
//    registerTransFunction(trans, trans_desc); 
    /* Register any additional transpose functions */


}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

