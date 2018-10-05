#include <cstdlib>
#include <vector>
#include "Slab_Allocator.h"

#ifndef POOL_H
#define POOL_H

namespace Promises {

	enum Status
	{
		Pending,
		Resolved,
		Rejected
	};

	class State
	{
	public:
		State(void)
			: _status(Pending)
		{
			//do nothing
		}

		State(Status stat)
			: _status(stat)
		{
			//do nothing
		}

		State(const State &state)
			: _status(state._status)
		{
			//do nothing
		}

		virtual ~State(void) {}

		bool operator==(Status stat)
		{
			return this->_status == stat;
		}

		bool operator!=(Status stat)
		{
			return this->_status != stat;
		}

		virtual void *getValue(void) = 0;
		virtual std::exception getReason(void) = 0;

	private:
		Status _status;
	};

	class IPromise
	{

	public:
		virtual ~IPromise(void) {}
		virtual State* getState(void) = 0;

	protected:
		virtual void _resolve(State* state) = 0;
		virtual void _reject(State* state) = 0;
		virtual void Join(void) = 0;

		friend class Settlement;
		friend class Pool;
	};

	//Pool is a class to manage the memory resources on a system.
	//the class allocates memory in 1MB chunks using the slab allocator
	class Pool
	{
	public:
		static Pool* Instance();
		~Pool(void);

		template<typename T>
		T* allocate(bool joinable = false)
		{
			void* temp = alloc_mem(_memory, sizeof(T));
			T* mem = (T*)temp;
			new (mem) T();

			if (joinable)
				this->_promises.push_back(mem);

			return mem;
		}

		template<typename T, typename IN1>
		T* allocate(IN1 arg1, bool joinable = false)
		{
			void* temp = alloc_mem(_memory, sizeof(T));
			T* mem = (T*)temp;
			new (mem) T(arg1);

			if (joinable)
				this->_promises.push_back(mem);

			return mem;
		}

		template<typename T, typename IN1, typename IN2>
		T* allocate(IN1 arg1, IN2 arg2, bool joinable = false)
		{
			void* temp = alloc_mem(_memory, sizeof(T));
			T* mem = (T*)temp;
			new (mem) T(arg1, arg2);

			if (joinable)
				this->_promises.push_back(mem);

			return mem;
		}

		void deallocate(void* mem);

	private:
		Pool(void);
		Pool(const Pool& p);
		Pool& operator = (const Pool& p);

		struct slab_allocator* _memory;
		static bool _is_created;
		static Pool* _pool;
		std::vector<void*> _promises;

	};

}//namespace has ended

#endif // !POOL_H