/***************************************************************************
 *
 * Sequential version of Gaussian elimination
 *
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <pthread.h>
#include <array>
#include <chrono>
#include <iostream>

#define MAX_SIZE 4096

typedef double Matrix[MAX_SIZE][MAX_SIZE];

int	matrixSize;		/* matrix size		*/
int	maxNumber;		/* max number of element*/
char	*Init;		/* matrix init type	*/
int	PRINT;		/* print switch		*/
Matrix	matrix;		/* matrix A		*/
double	equalities[MAX_SIZE];	/* vector b             */
double	result[MAX_SIZE];	/* vector y             */
constexpr int NUM_THREADS = 32;
struct ThreadArgs
{
    int i = 0;
    int j = 0;
    int k = 0;
    int id = 0;
    int inc = 1;
};

/* forward declarations */
void* work(void* args);
void* parFunc(void* args);
void Init_Matrix(void);
void Print_Matrix(void);
void Init_Default(void);
int Read_Options(int, char **);

int
main(int argc, char **argv)
{
    int i, timestart, timeend, iter;

    Init_Default();		/* Init default values	*/
    Read_Options(argc,argv);	/* Read arguments	*/
    Init_Matrix();		/* Init the matrix	*/

    std::array<pthread_t, NUM_THREADS> threadArr{};
    std::array<ThreadArgs, NUM_THREADS> threadArgs{};

    auto t1 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threadArgs[i].k = k;
        threadArgs[i].j = k + 1;
        threadArgs[i].id = i;
        threadArgs[i].i = 1;
        threadArgs[i].inc = 1;
        pthread_create(&threadArr[i], nullptr, work, &threadArgs[i]);
    }

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        pthread_join(threadArr[i], nullptr);
    }

    auto t2 = std::chrono::high_resolution_clock::now();


    if (PRINT == 1)
    {
        Print_Matrix();
    }

    std::cout << std::chrono::duration<double>(t2 - t1);
}

void*
work(void* args)
{


    /* Gaussian elimination algorithm, Algo 8.4 from Grama */
    for (int k = 0; k < matrixSize; k++) /* Outer loop */
    {
        // Dividerar raden och sätter värdet på diagonalen till ett
        for (int j = k + 1; j < matrixSize; j++)
        {
            matrix[k][j] = matrix[k][j] / matrix[k][k]; /* Division step */
        }

        result[k] = equalities[k] / matrix[k][k];
        matrix[k][k] = 1.0;



    }
}

void*
parFunc(void* args)
{
    ThreadArgs* arguments = static_cast<ThreadArgs*>(args);

    arguments->i = (arguments->id + 1) * arguments->inc + arguments->k;

    while (true)
    {

        if (arguments->i >= matrixSize)
        {
            break;
        }

        for (arguments->j = arguments->k + 1; arguments->j < matrixSize; arguments->j++)
        {
            matrix[arguments->i][arguments->j] = matrix[arguments->i][arguments->j] - matrix[arguments->i][arguments->k] * matrix[arguments->k][arguments->j]; /* Elimination step */
        }

        equalities[arguments->i] = equalities[arguments->i] - matrix[arguments->i][arguments->k] * result[arguments->k];
        matrix[arguments->i][arguments->k] = 0.0;

        arguments->i += NUM_THREADS;
    }

    return nullptr;
}

void
Init_Matrix()
{
    int i, j;

    printf("\nsize      = %dx%d ", matrixSize, matrixSize);
    printf("\nmaxnum    = %d \n", maxNumber);
    printf("Init	  = %s \n", Init);
    printf("Initializing matrix...");

    if (strcmp(Init,"rand") == 0) {
        for (i = 0; i < matrixSize; i++){
            for (j = 0; j < matrixSize; j++) {
                if (i == j) /* diagonal dominance */
                    matrix[i][j] = (double)(rand() % maxNumber) + 5.0;
                else
                    matrix[i][j] = (double)(rand() % maxNumber) + 1.0;
            }
        }
    }
    if (strcmp(Init,"fast") == 0) {
        for (i = 0; i < matrixSize; i++) {
            for (j = 0; j < matrixSize; j++) {
                if (i == j) /* diagonal dominance */
                    matrix[i][j] = 5.0;
                else
                    matrix[i][j] = 2.0;
            }
        }
    }

    /* Initialize vectors b and y */
    for (i = 0; i < matrixSize; i++) {
        equalities[i] = 2.0;
        result[i] = 1.0;
    }

    printf("done \n\n");
    if (PRINT == 1)
        Print_Matrix();
}

void
Print_Matrix()
{
    int i, j;

    printf("Matrix A:\n");
    for (i = 0; i < matrixSize; i++) {
        printf("[");
        for (j = 0; j < matrixSize; j++)
            printf(" %5.2f,", matrix[i][j]);
        printf("]\n");
    }
    printf("Vector b:\n[");
    for (j = 0; j < matrixSize; j++)
        printf(" %5.2f,", equalities[j]);
    printf("]\n");
    printf("Vector y:\n[");
    for (j = 0; j < matrixSize; j++)
        printf(" %5.2f,", result[j]);
    printf("]\n");
    printf("\n\n");
}

void
Init_Default()
{
    matrixSize = 2048;
    Init = "rand";
    maxNumber = 15.0;
    PRINT = 0;
}

int
Read_Options(int argc, char **argv)
{
    char    *prog;

    prog = *argv;
    while (++argv, --argc > 0)
        if (**argv == '-')
            switch ( *++*argv ) {
                case 'n':
                    --argc;
                    matrixSize = atoi(*++argv);
                    break;
                case 'h':
                    printf("\nHELP: try sor -u \n\n");
                    exit(0);
                    break;
                case 'u':
                    printf("\nUsage: gaussian [-n problemsize]\n");
                    printf("           [-D] show default values \n");
                    printf("           [-h] help \n");
                    printf("           [-I init_type] fast/rand \n");
                    printf("           [-m maxnum] max random no \n");
                    printf("           [-P print_switch] 0/1 \n");
                    exit(0);
                    break;
                case 'D':
                    printf("\nDefault:  n         = %d ", matrixSize);
                    printf("\n          Init      = rand" );
                    printf("\n          maxnum    = 5 ");
                    printf("\n          P         = 0 \n\n");
                    exit(0);
                    break;
                case 'I':
                    --argc;
                    Init = *++argv;
                    break;
                case 'm':
                    --argc;
                    maxNumber = atoi(*++argv);
                    break;
                case 'P':
                    --argc;
                    PRINT = atoi(*++argv);
                    break;
                default:
                    printf("%s: ignored option: -%s\n", prog, *argv);
                    printf("HELP: try %s -u \n\n", prog);
                    break;
            }

    return 0;
}
