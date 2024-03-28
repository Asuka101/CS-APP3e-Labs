/* 
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

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    switch (M)
    {
        case 32:
            for (int i = 0; i < 32; i += 8)
                for (int j = 0; j < 32; j += 8)
                    for (int k = i; k < i + 8; k++)
                    {
                        int t0 = A[k][j];
                        int t1 = A[k][j + 1];
                        int t2 = A[k][j + 2];
                        int t3 = A[k][j + 3];
                        int t4 = A[k][j + 4];
                        int t5 = A[k][j + 5];
                        int t6 = A[k][j + 6];
                        int t7 = A[k][j + 7];
                        B[j][k] = t0;
                        B[j + 1][k] = t1;
                        B[j + 2][k] = t2;
                        B[j + 3][k] = t3;
                        B[j + 4][k] = t4;
                        B[j + 5][k] = t5;
                        B[j + 6][k] = t6;
                        B[j + 7][k] = t7;
                    }
            /*Total misses:404*/
            // for (int i = 0; i < 32; i++) B[i][i] = A[i][i];
            // for (int n = 2; n < 32; n <<= 1)
            // {
            //     int a = n >> 1;
            //     for (int b = 0; b < 32; b += n)
            //     {
            //         int bound = b + n;
            //         for (int i = b; i < bound; i += a)
            //             for (int j = b; j < bound; j += a)
            //             {
            //                 if (i == j) continue;
            //                 for (int i1 = i; i1 < i + a; i1++)
            //                     for (int j1 = j; j1 < j + a; j1++)
            //                     {
            //                         B[j1][i1] = A[i1][j1];
            //                     }
            //             }
            //     }
            // }
            // for (int i = 0; i < 32; i += 16)
            //     for (int j = 0; j < 32; j += 16)
            //     {
            //         if (i == j) continue;
            //         for (int i1 = i; i1 < i + 16; i1 += 8)
            //             for (int j1 = j; j1 < j + 16; j1 += 8)
            //                 for (int i2 = i1; i2 < i1 + 8; i2++)
            //                     for (int j2 = j1; j2 < j1 + 8; j2++)
            //                     {
            //                         B[j2][i2] = A[i2][j2];
            //                     }
            //     }
            break;
        case 64:
            for (int i = 0; i < 64; i += 8)
                for (int j = 0; j < 64; j += 8)
                {
                    int k, t0, t1, t2, t3, t4, t5, t6, t7;
                    for (k = i; k < i + 4; k++)
                    {
                        t0 = A[k][j];
                        t1 = A[k][j + 1];
                        t2 = A[k][j + 2];
                        t3 = A[k][j + 3];
                        t4 = A[k][j + 4];
                        t5 = A[k][j + 5];
                        t6 = A[k][j + 6];
                        t7 = A[k][j + 7];
                        B[j][k] = t0;
                        B[j + 1][k] = t1;
                        B[j + 2][k] = t2;
                        B[j + 3][k] = t3;
                        B[j][k + 4] = t4;
                        B[j + 1][k + 4] = t5;
                        B[j + 2][k + 4] = t6;
                        B[j + 3][k + 4] = t7;
                    }
                    /*Total misses:1108*/
                    for (k = j; k < j + 4; k++)
                    {
                        int a;
                        t0 = A[i + 4][k];
                        t1 = A[i + 5][k];
                        t2 = A[i + 6][k];
                        t3 = A[i + 7][k];
                        t4 = A[i + 4][k + 4];
                        t5 = A[i + 5][k + 4];
                        t6 = A[i + 6][k + 4];
                        t7 = A[i + 7][k + 4];
                        a = B[k][i + 4];
                        B[k][i + 4] = t0;
                        t0 = a;
                        a = B[k][i + 5];
                        B[k][i + 5] = t1;
                        t1 = a;
                        a = B[k][i + 6];
                        B[k][i + 6] = t2;
                        t2 = a;
                        a = B[k][i + 7];
                        B[k][i + 7] = t3;
                        t3 = a;
                        B[k + 4][i] = t0;
                        B[k + 4][i + 1] = t1;
                        B[k + 4][i + 2] = t2;
                        B[k + 4][i + 3] = t3;
                        B[k + 4][i + 4] = t4;
                        B[k + 4][i + 5] = t5;
                        B[k + 4][i + 6] = t6;
                        B[k + 4][i + 7] = t7;
                    }
                    /*Total misses:1588*/
                    // for (k = i + 4; k < i + 8; k++)
                    // {
                    //     t0 = A[k][j];
                    //     t1 = A[k][j + 1];
                    //     t2 = A[k][j + 2];
                    //     t3 = A[k][j + 3];
                    //     t4 = A[k][j + 4];
                    //     t5 = A[k][j + 5];
                    //     t6 = A[k][j + 6];
                    //     t7 = A[k][j + 7];
                    //     B[j + 4][k - 4] = t0;
                    //     B[j + 5][k - 4] = t1;
                    //     B[j + 6][k - 4] = t2;
                    //     B[j + 7][k - 4] = t3;
                    //     B[j + 4][k] = t4;
                    //     B[j + 5][k] = t5;
                    //     B[j + 6][k] = t6;
                    //     B[j + 7][k] = t7;
                    // }
                    // for (k = j; k < j + 4; k++)
                    // {
                    //     t0 = B[k + 4][i];
                    //     t1 = B[k + 4][i + 1];
                    //     t2 = B[k + 4][i + 2];
                    //     t3 = B[k + 4][i + 3];
                    //     t4 = B[k][i + 4];
                    //     t5 = B[k][i + 5];
                    //     t6 = B[k][i + 6];
                    //     t7 = B[k][i + 7];
                    //     B[k][i + 4] = t0;
                    //     B[k][i + 5] = t1;
                    //     B[k][i + 6] = t2;
                    //     B[k][i + 7] = t3;
                    //     B[k + 4][i] = t4;
                    //     B[k + 4][i + 1] = t5;
                    //     B[k + 4][i + 2] = t6;
                    //     B[k + 4][i + 3] = t7;
                    // }
                }
            break;
        case 61:
            /*bloking size = 8 Total misses:1985*/
            int i, j, k, t0, t1, t2, t3, t4, t5, t6, t7;
            for (int i = 0; i < 64; i += 8)
            {
                for (int j = 0; j < 56; j += 8)
                {
                    for (k = i; k < i + 8; k++)
                    {
                        t0 = A[k][j];
                        t1 = A[k][j + 1];
                        t2 = A[k][j + 2];
                        t3 = A[k][j + 3];
                        t4 = A[k][j + 4];
                        t5 = A[k][j + 5];
                        t6 = A[k][j + 6];
                        t7 = A[k][j + 7];
                        B[j][k] = t0;
                        B[j + 1][k] = t1;
                        B[j + 2][k] = t2;
                        B[j + 3][k] = t3;
                        B[j + 4][k] = t4;
                        B[j + 5][k] = t5;
                        B[j + 6][k] = t6;
                        B[j + 7][k] = t7;
                    }
                }
            }
            for (j = 0; j < 51; j += 3) 
            {
                for (k = 64; k < 67; k++)
                {
                    t0 = A[k][j];
                    t1 = A[k][j + 1];
                    t2 = A[k][j + 2];
                    B[j][k] = t0;
                    B[j + 1][k] = t1;
                    B[j + 2][k] = t2;
                }
            }
            for (j = 51; j < 61; j += 5)
            {
                for (k = 64; k < 67; k++)
                {
                    t0 = A[k][j];
                    t1 = A[k][j + 1];
                    t2 = A[k][j + 2];
                    t3 = A[k][j + 3];
                    t4 = A[k][j + 4];
                    B[j][k] = t0;
                    B[j + 1][k] = t1;
                    B[j + 2][k] = t2;
                    B[j + 3][k] = t3;
                    B[j + 4][k] = t4;
                }
            }
            for (i = 0; i < 60; i += 5)
            {
                for (k = i; k < i + 5; k++)
                {
                    t0 = A[k][56];
                    t1 = A[k][57];
                    t2 = A[k][58];
                    t3 = A[k][59];
                    t4 = A[k][60];
                    B[56][k] = t0;
                    B[57][k] = t1;
                    B[58][k] = t2;
                    B[59][k] = t3;
                    B[60][k] = t4;
                }
            }
            for (k = 60; k < 64; k++)
            {
                    t0 = A[k][56];
                    t1 = A[k][57];
                    t2 = A[k][58];
                    t3 = A[k][59];
                    t4 = A[k][60];
                    B[56][k] = t0;
                    B[57][k] = t1;
                    B[58][k] = t2;
                    B[59][k] = t3;
                    B[60][k] = t4;                
            }
            /*blocking size = 3 Total misses:2596*/
            // int i, j, k, t0, t1, t2;
            // for (i = 0; i < 66; i += 3)
            // {
            //     for (j = 0; j < 60; j += 3)
            //     {
            //         for (k = i; k < i + 3; k++)
            //         {
            //             t0 = A[k][j];
            //             t1 = A[k][j + 1];
            //             t2 = A[k][j + 2];
            //             B[j][k] = t0;
            //             B[j + 1][k] = t1;
            //             B[j + 2][k] = t2;
            //         }
            //     }
            // }
            // for (i = 0; i < 67; i++) B[60][i] = A[i][60];
            // for (j = 0; j < 60; j++) B[j][66] = A[66][j];
            break;
        default:
            break;
    }
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

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

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
    registerTransFunction(trans, trans_desc); 

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

