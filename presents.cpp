#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace std;

const int TOTAL_PRESENTS = 500000;
const int THREAD_COUNT = 4;

struct Present {
    int tag;
    Present *next;

    Present(int tag) : tag(tag), next(nullptr) {}
};

class ConcurrentLinkedList {
private:
    Present *head;
    mutex mtx;
    int size;

public:
    ConcurrentLinkedList() : head(nullptr), size(0) {}

    void addPresent(int tag) {
        unique_lock<mutex> lock(mtx);
        Present *newPresent = new Present(tag);
        Present **pp = &head;
        while (*pp && (*pp)->tag < tag) {
            pp = &((*pp)->next);
        }
        newPresent->next = *pp;
        *pp = newPresent;
        size++;
    }

    int popHeadPresent() {
        unique_lock<mutex> lock(mtx);
        if (head) {
            Present *temp = head;
            head = head->next;
            int tag = temp->tag;
            delete temp;
            size--;
            return tag;
        }
        return -1;
    }

    bool hasPresent(int tag) {
        unique_lock<mutex> lock(mtx);
        Present *current = head;
        while (current) {
            if (current->tag == tag) {
                return true;
            }
            current = current->next;
        }
        return false;
    }

    int getSize() {
        unique_lock<mutex> lock(mtx);
        return size;
    }
};

void addOrRemovePresents(ConcurrentLinkedList &list, atomic<int> &thankYouCount, mutex &coutMutex, const vector<int> &presents, int startIdx, int endIdx) {
    default_random_engine generator;
    uniform_int_distribution<int> distribution(0, 1);
    int offset = 0;
    while (true) {
        int action = distribution(generator); 
        if (list.getSize() == 0 || (action == 0 && offset < (endIdx - startIdx))) {
            // Add present
            int idx = startIdx + offset;
            list.addPresent(presents[idx]);
            offset++;
        } else {
            // Remove present
            int tag = list.popHeadPresent();
            if (tag != -1) {
                thankYouCount++;
                {
                    lock_guard<mutex> guard(coutMutex);
                    // cout << "Server wrote a 'Thank you' note for present with tag: " << tag << endl;
                    cout << "Thank you count is " << thankYouCount << endl;
                }
            }
        }
        if (thankYouCount == TOTAL_PRESENTS) {
            break;
        }
    }
}

int main() {
    ConcurrentLinkedList list;
    atomic<int> thankYouCount(0);
    mutex coutMutex;

    vector<int> presents;
    for (int i = 1; i <= TOTAL_PRESENTS; ++i) {
        presents.push_back(i);
    }
    shuffle(presents.begin(), presents.end(), mt19937{random_device{}()});

    auto start = chrono::steady_clock::now();

    thread threads[THREAD_COUNT];

    int presentsPerThread = TOTAL_PRESENTS / THREAD_COUNT;
    int remainder = TOTAL_PRESENTS % THREAD_COUNT;
    int startIdx = 0;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        int endIdx = startIdx + presentsPerThread + (i < remainder ? 1 : 0);
        threads[i] = thread(addOrRemovePresents, ref(list), ref(thankYouCount), ref(coutMutex), cref(presents), startIdx, endIdx);
        startIdx = endIdx;
    }

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads[i].join();
    }

    auto end = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::seconds>(end - start);
    cout << "Adding and removing presents took " << elapsed.count() << " seconds" << endl;

    cout << "Total 'Thank you' notes sent: " << thankYouCount << endl;

    return 0;
}
