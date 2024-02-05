#include <iostream>
#include <pthread.h>
#include <unistd.h>

using namespace std;

struct HeapBlock {
    int id;
    int size;
    int index;
    HeapBlock *next;

    HeapBlock(int id, int size, int start) : id(id), size(size), index(start), next(NULL) {} // constructor
};

class HeapManager {
private:
    HeapBlock *root; // head of linked list
    pthread_mutex_t mutex;

public:
    HeapManager() : root(NULL) {
        pthread_mutex_init(&mutex, NULL);
    }
    ~HeapManager(); // destructor

    int initHeap(int size);
    int myMalloc(int id, int size);
    int myFree(int id, int start);
    void print();
};

HeapManager::~HeapManager() { // destructor implementation
    pthread_mutex_lock(&mutex);
    while (root != NULL) {
        HeapBlock *temp = root;
        root = root->next;
        delete temp;
    }
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
}

int HeapManager::initHeap(int size) {

    int freeID = -1;
    int initIdx = 0;

    pthread_mutex_lock(&mutex);
    root = new HeapBlock(freeID, size, initIdx); // initialize the heap
    pthread_mutex_unlock(&mutex);
    print();
    return 1; // successful initialization
}

int HeapManager::myMalloc(int id, int size) {
    pthread_mutex_lock(&mutex);
    int freeID = -1;
    HeapBlock *prev = NULL;
    HeapBlock *current = root;

    while (current != NULL) {
        if (freeID == current->id) { // if current id is -1
            if (size <= current->size){ // if requested size <= current size
                // search for a block
                if (size == current->size) { // do not split
                    current->id = id;
                    cout << "Allocated for thread ";
                    cout << id << endl;
                    pthread_mutex_unlock(&mutex);
                    print();
                    return current->index; // start idx of allocated block
                }

                else if (size != current->size) { // current size > requested size, so split
                    HeapBlock *newBlock = new HeapBlock(id, size, current->index);
                    current->index += size;
                    current->size -= size;
                    newBlock->next = current;
                    if (prev != NULL) prev->next = newBlock;
                    else root = newBlock;
                    cout << "Allocated for thread ";
                    cout << id << endl;
                    pthread_mutex_unlock(&mutex);
                    print();
                    return newBlock->index; // start idx of allocated block
                }
            }
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    cout << "Can not allocate, requested size " << size << " for thread " << id << " is bigger than remaining size" << endl;
    print();
    return -1;
}

int HeapManager::myFree(int id, int start) {
    pthread_mutex_lock(&mutex);
    HeapBlock *current = root;
    HeapBlock *prev = NULL;
    int freeID = -1;

    while (current != NULL) {
        if (id == current->id) {
            if (start == current->index){
                current->id = freeID; // block free
                // coalesce with next block if free
                while ((current->next != NULL) && (current->next->id == freeID)) {
                    HeapBlock * next = current->next;
                    current->size = current->size + next->size;
                    current->next = next->next;
                    delete next; // delete merged block
                }
                // coalesce with prev block if free
                if (prev != NULL) {
                    if (prev->id == freeID){
                        prev->size = prev->size + current->size;
                        HeapBlock *tempNext = current->next;
                        delete current;
                        prev->next = tempNext;
                        current = prev; // current point to prev
                    }   
                }
                
                cout << "Freed for thread " << id << endl;
                pthread_mutex_unlock(&mutex);
                print();
                return 1; // successful deallocation
            }
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&mutex);
    cout << "Cannot free for thread " << id << endl;
    print();
    return -1; // deallocation failure !!
}

void HeapManager::print() {
    pthread_mutex_lock(&mutex);
    HeapBlock *current = root;
    while (current != NULL) { // print id, size, idx
        cout << "[" << current->id << "][" << current->size << "][" << current->index << "]";
        current = current->next;
        if (current){
            cout << "---";
        }
    }
    cout << endl;
    pthread_mutex_unlock(&mutex);
}

