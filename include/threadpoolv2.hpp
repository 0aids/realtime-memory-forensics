#include <iterator>
#include <thread>
#include "log.hpp"
#include <functional>
#include <vector>
#include <optional>

// clang-format off

// How do create a policy to separate allocate work for the threads?
// How does the class receive the policy?


inline size_t niceNumThreads = std::thread::hardware_concurrency() - 1;


// The InputType must be a struct, which will be given as an input
// to thread's functions. These inputs must be generated using
// another function, which based on some index of the thread
// must return an InputType struct.
// WARNING: Do not place any debug statements inside threaded
// functions, unless they are mutex'd
template 
<
    typename ResultType, 
    typename InputType
>
class ThreadPool {
    public:
        using DiscriminatorFunc = std::function<InputType(ssize_t index, std::vector<ResultType> &results_l)>;
        using PerThreadFunc = std::function<void(InputType inputStruct)>;

        // Should return true if we want to add the current for consolidation
        // Otherwise returns false.
        // This function can modify the last result if need be.
        using ConsolidateDiscriminatorFunc = std::function<bool(ResultType &current, ResultType &last)>;

    private:

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

        // Number of threads to use. duh.
        size_t m_numThreads;

    public:
        ThreadPool(DiscriminatorFunc df, PerThreadFunc pf, size_t numThreads,  bool runOnHost = true):
            m_discriminatorFunc(df), 
            m_perThreadFunc(pf), 
            m_runOnHost(runOnHost), 
            m_numThreads(numThreads)
        {}

        ~ThreadPool() = default;

        ThreadPool(ThreadPool&) = default;

        ThreadPool& operator=(const ThreadPool&) = default;

        ThreadPool& operator=(ThreadPool&&) = default;

        void startThreads();
        void joinThreads();

        // The results are moved, and the threadpool will be
        // destroyed afterwards.
        // We do not store the consolidated results
        // Will consolidate based on a lambda that is given the previous and current
        // values being added.
        // Will NOT be called if there our results list is empty.
        std::vector<ResultType> consolidateResults(std::optional<ConsolidateDiscriminatorFunc> f = {});
};



template 
<
    typename ResultType, 
    typename InputType
>
void ThreadPool<ResultType, InputType>::startThreads()
{
    m_threadsList.reserve(m_numThreads);
    m_inputsList.reserve(m_numThreads);
    m_preliminaryResults.resize(m_numThreads);

    for (ssize_t i = 0; i < m_numThreads; i++)
    {
        m_inputsList.push_back(m_discriminatorFunc(i, m_preliminaryResults[i]));
        if (i == m_numThreads - 1 && m_runOnHost) 
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
    typename InputType
>
void ThreadPool<ResultType, InputType>::joinThreads()
{
    Log(Debug, "Joining all threads");
    for (auto &thr:m_threadsList) 
    {
        thr.join();
    }
}



template <typename ResultType, typename InputType>
std::vector<ResultType>
ThreadPool<ResultType, InputType>::consolidateResults(
    std::optional<ConsolidateDiscriminatorFunc> f)
{
    Log(Debug, "Consolidating results");
    size_t totalResults = 0;
    for (auto& resultArr : m_preliminaryResults)
    {
        totalResults += resultArr.size();
    }
    Log(Debug, "Total number before consolidation: " << totalResults);

    std::vector<ResultType> toReturn;
    toReturn.reserve(totalResults);

    for (auto& resultArr : m_preliminaryResults)
    {
        if (!f.has_value())
        {
            std::copy(std::make_move_iterator(resultArr.begin()),
                      std::make_move_iterator(resultArr.end()),
                      std::back_inserter(toReturn));
        }
        else
        {
            size_t numResults = resultArr.size();
            for (size_t i = 0; i < numResults; ++i)
            {
                if (( toReturn.size() > 0 &&
                        f.value()(resultArr[i], toReturn.back()) ) ||
                    toReturn.size() == 0)
                {
                    toReturn.push_back(resultArr[i]);
                }
            }
        }
    }

    Log(Debug, "Total number after consolidation: " << toReturn.size());
    // I hope i get nrvo.
    return std::move(toReturn);
}
