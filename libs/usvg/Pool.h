#ifndef _Pool_h
#define _Pool_h

#include <stdint.h>
#include <stddef.h>
#include <new>
#include <limits>
#include <stdlib.h>

#include "Debug.h"

class Pool;

class PoolRegistry {
    // this is a stupid implementation that only admits a single pool.
    Pool* p;

public:
    Pool* get() {
        return p;
    }
    void set(Pool* pool) {
        p = pool;
    }
};

extern PoolRegistry pool_registry;

/** Abstract base memory pool.  Knows how to hand out chunks of memory from linked free-list.
*/
class Pool {
  protected:
    volatile size_t available ;
    void** free_list;
    size_t size; // keep this here to avoid having Pool be a template.
    size_t count; // keep this here to avoid having Pool be a template.
    uint8_t* buffer;
  public:
    typedef uint8_t offset_t;  // parameterize on this?

    Pool(size_t sz, size_t cnt, uint8_t* b) : available(cnt-1), free_list(0), size(sz), count(cnt), buffer(b) {}

    void* allocate(size_t sz);

    void release(size_t sz, void* mem);

    void* ptr(offset_t cnt) {
        if (cnt == 0) return 0;
        return (buffer + size * cnt);
    }
    offset_t off(void* ptr) {
        if (ptr == 0) return 0;
        int loc = ((uint8_t*)ptr) - (uint8_t*)buffer;
        if (loc < 0 || ((loc / size) > std::numeric_limits<offset_t>::max())) {
            // crash.
            NRF51_CRASH("Invalid address.");
        }

        return loc / size;
    }

    void check(void* ptr) {
        int loc = ((uint8_t*)ptr) - (uint8_t*)buffer;
        if (loc < 0 || ((loc / size) > std::numeric_limits<offset_t>::max())) {
            // crash.
            NRF51_CRASH("Invalid address.");
        }
    }

    bool isEmpty() {
        return available == count;
    }

    uint32_t inUse() {
        return count - available - 1;
    }
};

template <uint32_t cnt, uint32_t sz = 20>
class PoolImpl : public Pool{

  public:
    PoolImpl() : Pool(sz, cnt, 0) {
        buffer = new uint8_t[sz*cnt];
	//buffer = (uint8_t*)malloc(sz*cnt*sizeof(uint8_t));
        uint8_t* prev = 0;
        for(int32_t i = count - 1; i > 0; --i) {
            *(uint8_t**)(buffer+i*sz) = prev;
            prev = &buffer[i*sz];
        }
        free_list = (void**)&buffer[sz];
        for(int32_t i = 0; i < sz/2; ++i) {
            *(uint16_t*)(buffer+i*2) = 0xdead;
        }
        pool_registry.set(this);
    }

    ~PoolImpl() {
        delete[] buffer;
    }
};




/** A typed pointer to an object stored in a pool.  */
template<typename T>
class pptr {
  public:
    typedef uint8_t offset_t;  // parameterize on this?

  protected:
    offset_t off;

  public:
    pptr() : off(0) {}
    pptr(T* t) {
        *this = t;
    }
    // autogenerated copy constructor is fine.

    operator const T*() const {
        return (T*)(pool_registry.get()->ptr(off));
    }
    operator T*() {
        return (T*)(pool_registry.get()->ptr(off));
    }

    pptr& operator=(T* t) {
        // self assignment is ok.
        off = pool_registry.get()->off(t);
        return *this;
    }

    T* operator->() {
        return (T*)*this;
    }

    const T* operator->() const {
        return (const T*)*this;
    }

    T& operator*() {
        return *(T*)this;
    }

    const T& operator*() const {
        return *(const T*)this;
    }

    void set(off_t off) {
        *this = (T*)(pool_registry.get()->ptr(off));
    }

    operator bool() {
        return off != 0;
    }

};

#endif /* _Pool_h */
