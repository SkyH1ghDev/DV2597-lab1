/***************************************************************************
 *
 * Sequential version of Quick sort
 *
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <iostream>

constexpr int KILO = 1 << 10;
constexpr int MEGA = 1 << 20;
constexpr int MAX_ITEMS = 1 << 26;
constexpr int MAX_THREADS = 32;

alignas(64)
struct ThreadArgs
{
    unsigned int AvailableThreads = 0;
    unsigned int Low = 0;
    unsigned int High = 0;
    pthread_t* ThreadArr = nullptr;
    ThreadArgs* ThreadArgArr = nullptr;
};


static unsigned int* v;

static void
print_array(void)
{
    for (int i = 0; i < MAX_ITEMS; i++)
        printf("%d ", v[i]);
    printf("\n");
}

static void
init_array(void)
{
    int i;
    for (i = 0; i < MAX_ITEMS; i++)
        v[i] = rand();
}

static unsigned int
partition(unsigned int low, unsigned int high, unsigned int pivot_index)
{
    /* move pivot to the bottom of the vector */
    if (pivot_index != low)
        std::swap(v[low], v[pivot_index]);

    pivot_index = low;
    low++;

    /* invariant:
     * v[i] for i less than low are less than or equal to pivot
     * v[i] for i greater than high are greater than pivot
     */

    /* move elements into place */
    while (low <= high)
    {
        if (v[low] <= v[pivot_index])
            low++;
        else if (v[high] > v[pivot_index])
            high--;
        else
            std::swap(v[low], v[high]);
    }

    /* put pivot back between two groups */
    if (high != pivot_index)
        std::swap(v[pivot_index], v[high]);
    return high;
}

void
quick_sort(unsigned int low, unsigned int high)
{
    /* no need to sort a vector of zero or one element */
    if (low >= high)
        return;

    /* select the pivot value */
    unsigned int pivot_index = (low + high) / 2;

    /* partition the vector */
    pivot_index = partition(low, high, pivot_index);

    /* sort the two sub arrays */
    if (low < pivot_index)
        quick_sort(low, pivot_index - 1);
    if (pivot_index < high)
        quick_sort(pivot_index + 1, high);
}

void*
quick_sort(void* pArgs) {
    ThreadArgs* args = static_cast<ThreadArgs *>(pArgs);
    unsigned int low = args->Low;
    unsigned int high = args->High;
    unsigned int availableThreads = args->AvailableThreads;
    pthread_t* threadArr = args->ThreadArr;
    ThreadArgs* threadArgArr = args->ThreadArgArr;

    /* no need to sort a vector of zero or one element */
    if (low >= high)
        return nullptr;

    /* select the pivot value */
    unsigned int pivot_index = (low + high) / 2;

    /* partition the vector */
    pivot_index = partition(low, high, pivot_index);

    /* sort the two sub arrays */

    int flags = static_cast<int>(pivot_index < high) << 2 | static_cast<int>(low < pivot_index) << 1 | static_cast<int>(availableThreads > 1) << 0;

    switch (flags)
    {
        case 0b111:
        [[fallthrough]];
        case 0b101:
        {
            ThreadArgs* threadArgs = &threadArgArr[availableThreads / 2];
            threadArgs->AvailableThreads = availableThreads / 2;
            args->AvailableThreads = availableThreads / 2;
            threadArgs->High = high;
            threadArgs->Low = pivot_index + 1;
            threadArgs->ThreadArr = &args->ThreadArr[availableThreads / 2];
            threadArgs->ThreadArgArr = threadArgs;

            static_cast<void>(pthread_create(&threadArr[availableThreads / 2], nullptr, quick_sort, threadArgs));
        }
        break;

        case 0b100:
            [[fallthrough]];
        case 0b110:
            quick_sort(pivot_index + 1, high);
            break;

        default:
            break;
    }

    switch (flags)
    {
        case 0b011:
            args->High = pivot_index - 1;
            args->Low = low;

            quick_sort(args);
            break;

        case 0b110:
            [[fallthrough]];
        case 0b010:
            quick_sort(low, pivot_index - 1);
            break;

        case 0b111:
            args->High = pivot_index - 1;
            args->Low = low;

            quick_sort(args);
            [[fallthrough]];
        case 0b101:
            static_cast<void>(pthread_join(threadArr[availableThreads / 2], nullptr));
            break;

        default:
            break;
    }

    // right + thread creation
    /*
    if (pivot_index < high && availableThreads > 1)
    [[unlikely]]
    {
        ThreadArgs* threadArgs = &threadArgArr[availableThreads / 2];
        threadArgs->AvailableThreads = availableThreads / 2;
        args->AvailableThreads = availableThreads / 2;
        threadArgs->High = high;
        threadArgs->Low = pivot_index + 1;
        threadArgs->ThreadArr = &args->ThreadArr[availableThreads / 2];
        threadArgs->ThreadArgArr = threadArgs;
        createdThreadID = availableThreads / 2;

        static_cast<void>(pthread_create(&threadArr[createdThreadID], nullptr, quick_sort, threadArgs));
    }
    // right
    else if (pivot_index < high)
    [[likely]]
    {
        quick_sort(pivot_index + 1, high);
    }*/

    // left + thread update
    /*if (low < pivot_index && availableThreads > 1)
    [[unlikely]]
    {
        args->High = pivot_index - 1;
        args->Low = low;

        quick_sort(args);
    } else if (low < pivot_index)
    [[likely]]
    {
        quick_sort(low, pivot_index - 1);
    }

    if (createdThreadID != -1)
    {
        static_cast<void>(pthread_join(threadArr[createdThreadID], nullptr));
    }*/

    return nullptr;
}

int
main(int argc, char** argv)
{
    std::array<pthread_t, MAX_THREADS> threadArr{};
    std::array<ThreadArgs, MAX_THREADS> threadArgs{};

    //print_array();
    v = static_cast<unsigned int*>(malloc(MAX_ITEMS * sizeof(int)));

    for (int numThreads = 1, iteration = 0;
         iteration < 5 && numThreads <= MAX_THREADS;)
    {
        init_array();

        threadArgs[0].AvailableThreads = numThreads;
        threadArgs[0].High = MAX_ITEMS;
        threadArgs[0].Low = 0;
        threadArgs[0].ThreadArr = threadArr.data();
        threadArgs[0].ThreadArgArr = threadArgs.data();

        auto t1 = std::chrono::high_resolution_clock::now();

        static_cast<void>(pthread_create(&threadArr[0], nullptr, quick_sort, &threadArgs[0]));
        static_cast<void>(pthread_join(threadArr[0], nullptr));


        auto t2 = std::chrono::high_resolution_clock::now();

        std::cout << "iteration: " << iteration << ", num threads: " << numThreads << ", time: " <<
                std::chrono::duration<double>(t2 - t1).count() << "\n";

        //print_array();

        ++iteration;
        if (iteration == 5)
        {
            iteration = 0;
            numThreads *= 2;
        }
    }

    free(v);
}