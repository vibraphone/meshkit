#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H
#include <assert.h>

#include <string>
#include <vector>

#include <vector>
using namespace std;

template <class T>
class ObjectPool {
    struct Pool;
public:

    ObjectPool() {
        chunkSize = 1;
    }

    virtual ~ObjectPool() {
    }

    void setChunkSize(size_t n) {
        chunkSize = n;
    }

    void reserve(size_t n) {
        Pool *p = new Pool(n);
        vpools.push_back(p);
    }

    T *allocate() {
        Pool *p;
        if (!vpools.empty()) {
            p = vpools.back();
            if (!p->is_filled()) return p->allocate();
        }

        // Resources exhausted, create new pool..
        p = new Pool(chunkSize);
        vpools.push_back(p);

        return p->allocate();
    }

    void release(T *obj) {
    }

    int deleteAll() {
        for (size_t i = 0; i < vpools.size(); i++) {
            vpools[i]->deleteAll();
            delete vpools[i];
        }
        vpools.clear();
        return 0;
    }
private:
    size_t chunkSize;

    struct Pool {

        Pool(size_t n) {
            currpos = 0;
            capacity = n;
            objects = new T[capacity];
            assert(objects != NULL);
        }

        bool is_filled() const {
            return currpos == capacity;
        }

        T * allocate() {
            return &objects[currpos++];
        }

        void deleteAll() {
            capacity = 0;
            currpos = 0;
            delete [] objects;
            objects = NULL;
        }

        size_t capacity, currpos;
        T *objects;
    };

    std::vector<Pool*> vpools;
};

#endif
