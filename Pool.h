#include "Slab_Allocator.h"
#include <cstdlib>
#include <vector>
#include <condition_variable>
#include <mutex>

#ifndef POOL_H
#define POOL_H

namespace Promises {

	//Pool is a class to manage the memory resources on a system.
	//the class allocates memory in 1MB chunks using the slab allocator
	class Pool
	{
	public:

		static Pool* Instance();
		~Pool(void);

		template<typename T>
		T* allocate()
		{
			this->_mem_lock.lock();

			void* temp = alloc_mem(_memory, sizeof(T));
			T* mem = (T*)temp;
			new (mem) T();

			this->_mem_lock.unlock();

			return mem;
		}

		template<typename T, typename IN1>
		T* allocate(IN1 arg1)
		{
			this->_mem_lock.lock();

			void* temp = alloc_mem(_memory, sizeof(T));
			T* mem = (T*)temp;
			new (mem) T(arg1);

			this->_mem_lock.unlock();

			return mem;
		}

		template<typename T, typename IN1, typename IN2>
		T* allocate(IN1 arg1, IN2 arg2)
		{
			this->_mem_lock.lock();

			void* temp = alloc_mem(_memory, sizeof(T));
			T* mem = (T*)temp;
			new (mem) T(arg1, arg2);

			this->_mem_lock.unlock();

			return mem;
		}

		void deallocate(void* mem);

		void promise_complete(void);

	private:

		Pool(void);
		Pool(const Pool& p);
		Pool& operator = (const Pool& p);

		struct slab_allocator* _memory;
		static bool _is_created;
		static Pool* _pool;
		std::mutex _mem_lock;

	};

}//namespace has ended

#endif // !POOL_H