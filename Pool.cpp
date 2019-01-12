#include "Pool.h"
#include "Slab_Allocator.h"
#include "IPromise.h"
#include <mutex>
#include <iostream>
#define MEGA_BYTE 1048576

namespace Promises {
    bool Pool::_is_created = false;
    Pool* Pool::_pool = nullptr;

    Pool::Pool(void)
        :_memory(new slab_allocator)
    {
        init_slab_allocator (_memory, 41*MEGA_BYTE);
    }

    //user should not need to pass memory pool around
    Pool::Pool(const Pool& p)
        :_memory(p._memory)
    {
        //do nothing
    }

    Pool::~Pool(void)
    {
        if (_memory != NULL || _memory != nullptr)
        {
            free_pool(_memory);
            delete _memory;
        }

        _is_created = false;
    }

    Pool* Pool::Instance()
    {
        if (!_is_created)
        {
            _pool = new Pool();
            _is_created = true;
            return _pool;
        }
        else
            return _pool;
    }

    void Pool::deallocate(void* mem)
    {
        free_mem(_memory, mem);
    }

    Pool& Pool::operator = (const Pool& p)
    {
        if (this == &p)
            return *this;
        
        this->_memory = p._memory;

        return *this;
    }

}//namespace has ended
