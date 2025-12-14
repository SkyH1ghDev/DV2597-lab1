/***************************************************************************
 *
 * Sequential version of Gaussian elimination
 *
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <array>
#include <chrono>
#include <iostream>

#define MAX_SIZE 4096

typedef double Matrix[MAX_SIZE][MAX_SIZE];

int matrixSize; /* matrix size		*/
int maxNumber; /* max number of element*/
char* Init; /* matrix init type	*/
int PRINT; /* print switch		*/
Matrix matrix; /* matrix A		*/
double equalities[MAX_SIZE]; /* vector b             */
double result[MAX_SIZE]; /* vector y             */
constexpr int MAX_THREADS = 32;
int g_currThreads = 0;
pthread_barrier_t barrier;

struct ThreadArgs
{
    int id = 0;
};

/* forward declarations */
void* work(void* args);

void* parFunc(void* args);

void Init_Matrix(void);

void Print_Matrix(void);

void Init_Default(void);

int Read_Options(int, char**);

int
main(int argc, char** argv)
{
    int i, timestart, timeend, iter;

    Init_Default(); /* Init default values	*/
    Read_Options(argc, argv); /* Read arguments	*/
    Init_Matrix(); /* Init the matrix	*/

    for (int numThreads = 1, iteration = 0;
         iteration < 5 && numThreads <= MAX_THREADS;)
    {
        g_currThreads = numThreads;
        std::array<pthread_t, MAX_THREADS> threadArr{};
        std::array<ThreadArgs, MAX_THREADS> threadArgs{};
        pthread_barrier_init(&barrier, nullptr, numThreads);

        auto t1 = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < numThreads; ++i)
        {
            threadArgs[i].id = i;
            static_cast<void>(pthread_create(&threadArr[i], nullptr, work, &threadArgs[i]));
        }

        for (int i = 0; i < numThreads; ++i)
        {
            static_cast<void>(pthread_join(threadArr[i], nullptr));
        }

        auto t2 = std::chrono::high_resolution_clock::now();

        std::cout << "iteration: " << iteration << ", num threads: " << numThreads << ", time: " <<
                std::chrono::duration<double>(t2 - t1).count() << "\n";

        ++iteration;
        if (iteration == 5)
        {
            iteration = 0;
            numThreads *= 2;
        }
    }


    if (PRINT == 1)
    {
        Print_Matrix();
    }
}

void*
work(void* args)
{
    ThreadArgs* arguments = static_cast<ThreadArgs *>(args);

    for (int k = 0; k < matrixSize; ++k)
    {
        // Dividerar raden och sätter värdet på diagonalen till ett

        //std::cout << "Thread " << arguments->id << " has begun division step with K = " << k << "\n";

        for (int i = k + 1 + arguments->id; i < matrixSize; i += g_currThreads)
        {
            matrix[k][i] = matrix[k][i] / matrix[k][k];
        }

        pthread_barrier_wait(&barrier);

        if (arguments->id == 0)
        {
            result[k] = equalities[k] / matrix[k][k];
            matrix[k][k] = 1.0;
        }

        pthread_barrier_wait(&barrier);

        //std::cout << "Thread " << arguments->id << " has begun elimination step with K = " << k << "\n";

        // ELIMINATION STEP

        for (int i = k + 1 + arguments->id; i < matrixSize; i += g_currThreads)
        {
            for (int j = k + 1; j < matrixSize; ++j)
            {
                matrix[i][j] = matrix[i][j] - matrix[i][k] * matrix[k][j];
            }

            equalities[i] = equalities[i] - matrix[i][k] * result[k];
            matrix[i][k] = 0.0;
        }

        pthread_barrier_wait(&barrier);
    }

    //std::cout << "Thread " << arguments->id << " has exited\n";

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

    if (strcmp(Init, "rand") == 0)
    {
        for (i = 0; i < matrixSize; i++)
        {
            for (j = 0; j < matrixSize; j++)
            {
                if (i == j) /* diagonal dominance */
                    matrix[i][j] = (double) (rand() % maxNumber) + 5.0;
                else
                    matrix[i][j] = (double) (rand() % maxNumber) + 1.0;
            }
        }
    }
    if (strcmp(Init, "fast") == 0)
    {
        for (i = 0; i < matrixSize; i++)
        {
            for (j = 0; j < matrixSize; j++)
            {
                if (i == j) /* diagonal dominance */
                    matrix[i][j] = 5.0;
                else
                    matrix[i][j] = 2.0;
            }
        }
    }

    /* Initialize vectors b and y */
    for (i = 0; i < matrixSize; i++)
    {
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
    for (i = 0; i < matrixSize; i++)
    {
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
Read_Options(int argc, char** argv)
{
    char* prog;

    prog = *argv;
    while (++argv, --argc > 0)
        if (**argv == '-')
            switch (*++*argv)
            {
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
                    printf("\n          Init      = rand");
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
