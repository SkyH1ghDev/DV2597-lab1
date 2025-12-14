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
constexpr int NUM_THREADS = 3;
pthread_barrier_t barrier;
struct ThreadArgs
{
    int i = 0;
    int j = 0;
    int id = 0;
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
    pthread_barrier_init(&barrier, nullptr, NUM_THREADS);

    auto t1 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threadArgs[i].i = 1;
        threadArgs[i].j = 1;
        threadArgs[i].id = i;
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
    ThreadArgs* arguments = static_cast<ThreadArgs*>(args);

    /* Gaussian elimination algorithm, Algo 8.4 from Grama */
    for (int k = 0; k < matrixSize; ) /* Outer loop */
    {

        // Dividerar raden och sätter värdet på diagonalen till ett
        arguments->i = k + 1 + arguments->id;
        while (true)
        {
            if (arguments->i >= matrixSize)
            {
                break;
            }

            matrix[k][arguments->i] = matrix[k][arguments->i] / matrix[k][k];

            arguments->i += NUM_THREADS;
        }

        pthread_barrier_wait(&barrier);
        if (arguments->id == 0)
        {
            result[k] = equalities[k] / matrix[k][k];
            matrix[k][k] = 1.0;
        }


        arguments->i = k + 1 + arguments->id;
        while (true)
        {

            if (arguments->i >= matrixSize)
            {
                break;
            }

            arguments->j = k + 1;
            while (true)
            {
                if (arguments->j >= matrixSize)
                {
                    break;
                }

                matrix[arguments->i][arguments->j] = matrix[arguments->i][arguments->j] - matrix[arguments->i][k] * matrix[k][arguments->j]; /* Elimination step */
                ++arguments->j;
            }


            if (arguments->id == 0)
            {
                equalities[arguments->i] = equalities[arguments->i] - matrix[arguments->i][k] * result[k];
                matrix[arguments->i][k] = 0.0;
            }

            arguments->i += NUM_THREADS;
        }

        pthread_barrier_wait(&barrier);

        if (arguments->id == 0)
        {
            ++k;
        }
    }

    return nullptr;
}

void*
parFunc(void* args)
{

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
