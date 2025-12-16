/***************************************************************************
 *
 * Sequential version of Quick sort
 *
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <deque>
#include <tuple>
#include <functional>

enum ThreadStatusEnum : std::uint8_t
{
    Working = 0,
    Waiting = 1,
};

constexpr int KILO = 1 << 10;
constexpr int MEGA = 1 << 20;
constexpr int MAX_ITEMS = 1 << 26;
constexpr int MAX_THREADS = 32;
int g_currThreads = 0;
std::array<ThreadStatusEnum, MAX_THREADS> g_threadStatusArr = std::array<ThreadStatusEnum, MAX_THREADS>{};
std::array<pthread_mutex_t, MAX_THREADS> g_queueMutexArr = std::array<pthread_mutex_t, MAX_THREADS>{};
std::array<std::deque<std::tuple<std::function<void(std::uint32_t, std::uint32_t, std::uint8_t)>, std::uint32_t, std::uint32_t>>, MAX_THREADS> g_workQueueArr = std::array<std::deque<std::tuple<std::function<void(std::uint32_t, std::uint32_t, std::uint8_t)>, std::uint32_t, std::uint32_t>>, MAX_THREADS>{};

struct ThreadArgs
{
    alignas(64)
    std::uint8_t ThreadID = -1;
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
quick_sort(std::uint32_t low, std::uint32_t high, std::uint8_t threadID)
{
    /* no need to sort a vector of zero or one element */
    if (low >= high)
        return;

    /* select the pivot value */
    unsigned int pivot_index = (low + high) / 2;

    /* partition the vector */
    pivot_index = partition(low, high, pivot_index);

    pthread_mutex_lock(&g_queueMutexArr[threadID]);
    if (low < pivot_index)
        g_workQueueArr[threadID].emplace_front(quick_sort, low, pivot_index - 1);
    if (pivot_index < high)
        g_workQueueArr[threadID].emplace_front(quick_sort, pivot_index + 1, high);
    pthread_mutex_unlock(&g_queueMutexArr[threadID]);
}

void*
worker(void* pArgs)
{
    while (true)
    {
        ThreadArgs* tArgs = static_cast<ThreadArgs *>(pArgs);
        std::function<void(std::uint32_t, std::uint32_t, std::uint8_t)> task = nullptr;
        std::uint32_t low = -1;
        std::uint32_t high = -1;

        g_threadStatusArr[tArgs->ThreadID] = Working;

        // Check if own queue has task
        pthread_mutex_lock(&g_queueMutexArr[tArgs->ThreadID]);
        if (!g_workQueueArr[tArgs->ThreadID].empty())
        {
            task = std::get<0>(g_workQueueArr[tArgs->ThreadID].front());
            low = std::get<1>(g_workQueueArr[tArgs->ThreadID].front());
            high = std::get<2>(g_workQueueArr[tArgs->ThreadID].front());

            g_workQueueArr[tArgs->ThreadID].erase(g_workQueueArr[tArgs->ThreadID].begin());
        }
        pthread_mutex_unlock(&g_queueMutexArr[tArgs->ThreadID]);

        // If no task found, steal from other threads
        while (task == nullptr)
        {
            std::uint8_t index = rand() % g_currThreads;

            pthread_mutex_lock(&g_queueMutexArr[index]);
            if (!g_workQueueArr[index].empty())
            {
                task = std::get<0>(g_workQueueArr[tArgs->ThreadID].back());
                low = std::get<1>(g_workQueueArr[tArgs->ThreadID].back());
                high = std::get<2>(g_workQueueArr[tArgs->ThreadID].back());

                g_workQueueArr[index].erase(g_workQueueArr[index].end());
            }
            pthread_mutex_unlock(&g_queueMutexArr[index]);

            std::uint8_t numWaiting = 0;
            for (int i = 0; i < g_currThreads; ++i)
            {
                numWaiting += g_threadStatusArr[i];
            }

            if (numWaiting == g_currThreads)
            {
                return nullptr;
            }

            g_threadStatusArr[tArgs->ThreadID] = Waiting;
        }

        g_threadStatusArr[tArgs->ThreadID] = Working;

        task(low, high, tArgs->ThreadID);
    }
}

int
main(int argc, char** argv)
{
    std::array<pthread_t, MAX_THREADS> threadArr{};
    std::array<ThreadArgs, MAX_THREADS> threadArgs{};

    for (auto& mutex : g_queueMutexArr)
    {
        pthread_mutex_init(&mutex, nullptr);
    }

    //print_array();
    v = static_cast<unsigned int *>(malloc(MAX_ITEMS * sizeof(int)));

    for (int numThreads = 1, iteration = 0;
         iteration < 5 && numThreads <= MAX_THREADS;)
    {
        init_array();
        g_currThreads = numThreads;
        g_workQueueArr[0].emplace_front(quick_sort, 0, MAX_ITEMS);

        auto t1 = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < numThreads; ++i)
        {
            threadArgs[i].ThreadID = i;
            static_cast<void>(pthread_create(&threadArr[i], nullptr, worker, &threadArgs[i]));
        }
        for (int i = 0; i < numThreads; ++i)
        {
            static_cast<void>(pthread_join(threadArr[i], nullptr));
        }

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
