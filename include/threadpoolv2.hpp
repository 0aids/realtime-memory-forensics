#include <iterator>
#include <thread>
#include "log.hpp"
#include <functional>
#include <vector>

// clang-format off

// How do create a policy to separate allocate work for the threads?
// How does the class receive the policy?


// The InputType must be a struct, which will be given as an input
// to thread's functions. These inputs must be generated using
// another function, which based on some index of the thread
// must return an InputType struct.
// WARNING: Do not place any debug statements inside threaded
// functions, unless they are mutex'd
template 
<
    typename ResultType, 
    typename InputType,
    size_t numThreads
>
class ThreadPool {
    private:
        using DiscriminatorFunc = std::function<InputType(ssize_t index, std::vector<ResultType> &results_l)>;
        using PerThreadFunc = std::function<ResultType(InputType &inputStruct)>;

        std::vector<std::vector<ResultType>> m_preliminaryResults;
        std::vector<std::thread> m_threadsList;
        std::vector<InputType> m_inputsList;
        DiscriminatorFunc m_discriminatorFunc;
        PerThreadFunc m_perThreadFunc;

        // If true, then the last "thread" will actually be run blocking on the thread that called "startThreads".
        bool m_runOnHost;

        // True when the results have been consolidated
        // This means all the values in any of our vectors
        // has been moved, and thus all our results are empty.
        bool m_finished;


    public:
        ThreadPool(DiscriminatorFunc df, PerThreadFunc pf, bool runOnHost = true):
            m_discriminatorFunc(df), 
            m_perThreadFunc(pf), 
            m_runOnHost(runOnHost) 
        {}

        ~ThreadPool() = default;

        ThreadPool(ThreadPool&) = default;

        ThreadPool& operator=(const ThreadPool&) = default;

        ThreadPool& operator=(ThreadPool&&) = default;

        // Is blocking if the option "runOnHost" is true.
        void startThreads();

        void joinThreads();

        // The results are moved, and the threadpool will be
        // destroyed afterwards.
        // We do not store the consolidated results
        // If there are overlaps in data, the user must be
        // responsible for tidying up the returned values.
        std::vector<ResultType> consolidateResults();
};



template 
<
    typename ResultType, 
    typename InputType,
    size_t numThreads
>
void ThreadPool<ResultType, InputType, numThreads>::startThreads()
{
    m_threadsList.reserve(numThreads);
    m_inputsList.reserve(numThreads);
    m_preliminaryResults.resize(numThreads);

    for (ssize_t i = 0; i < numThreads; i++)
    {
        m_inputsList.push_back(m_discriminatorFunc(i, m_preliminaryResults[i]));
        if (i == numThreads - 1 && m_runOnHost) 
        {
            Log(Debug, "Running last thread on host...");
            m_perThreadFunc(m_inputsList.back());
        } 

        else 
        {
            Log(Debug, "Starting thread: " << i + 1);
            m_threadsList.push_back(std::thread(m_perThreadFunc, m_inputsList.back()));
        }
    }
}

template 
<
    typename ResultType, 
    typename InputType,
    size_t numThreads
>
void ThreadPool<ResultType, InputType, numThreads>::joinThreads()
{
    Log(Debug, "Joining all threads");
    for (auto &thr:m_threadsList) 
    {
        thr.join();
    }
}



template 
<
    typename ResultType, 
    typename InputType,
    size_t numThreads
>
std::vector<ResultType> ThreadPool<ResultType, InputType, numThreads>::consolidateResults()
{
    Log(Debug, "Consolidating results");
    size_t totalResults = 0;
    for (auto &resultArr : m_preliminaryResults) 
    {
        totalResults += resultArr.size();
    }

    std::vector<ResultType> toReturn;
    toReturn.reserve(totalResults);

    for (auto &resultArr : m_preliminaryResults) 
    {
        std::copy(
            std::make_move_iterator(resultArr.begin()),
            std::make_move_iterator(resultArr.end()),
            std::back_inserter(toReturn)
        );
    }

    // I hope i get nrvo.
    return toReturn;
}
