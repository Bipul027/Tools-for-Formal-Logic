#include "cdcl.h"

struct Score
{
    float score;
    int propId;
    bool operator<(const Score &other) const
    {
        if (score != other.score)
            return score < other.score;
        return propId > other.propId;
    }
};

namespace CDCLnamespace
{
    using Clause = std::vector<int>;
    using Clauses = std::vector<Clause>;
    using Model = std::vector<int>;
    using Level = std::vector<int>;
    using Reason = std::vector<int>;
    using Dstack = std::vector<int>;
    using Dstacklevel = std::vector<int>;
    using Watches = std::vector<std::vector<Watcher>>;

}

struct VSIDSHeap
{
    std::vector<int> heap;
    std::vector<int> position;
    std::vector<double> score;
    double var_inc = 1.0;
    static constexpr double decay = 0.95;

    VSIDSHeap(int propNums) : position(propNums + 1, -1), score(propNums + 1, 0.0)
    {
        heap.reserve(propNums);
        for (int i = 1; i <= propNums; i++)
        {
            position[i] = static_cast<int>(heap.size());
            heap.push_back(i);
        }
    }
    bool empty() const { return heap.empty(); }

    void siftUp(int idx)
    {
        const int v = heap[idx];
        const double act = score[v];
        while (idx > 0)
        {
            const int parent = (idx - 1) >> 1;
            const int pv = heap[parent];
            if (act <= score[pv])
                break;
            heap[idx] = pv;
            position[pv] = idx;
            idx = parent;
        }
        heap[idx] = v;
        position[v] = idx;
    }

    void siftDown(int idx)
    {
        const int v = heap[idx];
        const double act = score[v];
        const int n = static_cast<int>(heap.size());
        while (true)
        {
            const int left = (idx << 1) + 1;
            if (left >= n)
                break;
            const int right = left + 1;
            const int child = (right < n && score[heap[right]] > score[heap[left]]) ? right : left;
            const int cv = heap[child];
            if (score[cv] <= act)
                break;
            heap[idx] = cv;
            position[cv] = idx;
            idx = child;
        }
        heap[idx] = v;
        position[v] = idx;
    }
    void insert(int v)
    {
        if (position[v] != -1)
            return;
        const int idx = static_cast<int>(heap.size());
        heap.push_back(v);
        position[v] = idx;
        siftUp(idx);
    }
    int pop()
    {
        const int top = heap[0];
        position[top] = -1;
        const int last = heap.back();
        heap.pop_back();
        if (!heap.empty())
        {
            heap[0] = last;
            position[last] = 0;
            siftDown(0);
        }
        return top;
    }
    void rescale()
    {
        for (double &a : score)
            a *= 1e-100;
        var_inc *= 1e-100;
    }
    void increaseScore(int v)
    {
        score[v] += var_inc;
        if (score[v] > 1e100)
            rescale();
        if (position[v] != -1)
            siftUp(position[v]);
    }
    void decayInc()
    {
        var_inc *= (1.0 / decay);
    }
};
