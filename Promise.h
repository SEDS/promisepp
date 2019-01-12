#ifndef PROMISE_H
#define PROMISE_H

#include "Pool.h"
#include <functional>
#include <stdexcept>
#include <thread>
#include <condition_variable>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <iterator>
#include <map>
#include <iostream>
#include <cstdlib>

namespace Promises
{

	static int _promiseID = 0;

	class ILambda
	{
	public:
		virtual ~ILambda(void) {}
		virtual void call(IPromise* prom) = 0;
		virtual IPromise* call(State* stat) = 0;
	};

	//init the memory pool as a unique ptr so it will be destructed once program terminates
	static std::unique_ptr<Promises::Pool> memory_pool(Promises::Pool::Instance());

	//Finisher - a class that safely destroys the environment and waits for all Promises to be fulfilled before termination.
	class Finisher {
		public:
			explicit Finisher(void)
			{
				//nothing to do
			}


			void add_promise(IPromise* prom) {
				_lock.lock();

				_promises.push_back(prom);

				_lock.unlock();
			}

			void finish (void) {
				_lock.lock();

				for(size_t i=0; i<_promises.size(); ++i) {
					_promises[i]->Join();
					// memory_pool->deallocate(_promises[i]);
				}

				_lock.unlock();
			}

		private:
			std::mutex _lock;
			std::vector<IPromise*> _promises;
	};

	static Finisher finisher;
	static bool exit_handle_created = false;
	
	void fin(void) {
		finisher.finish();
	}

	class RejectedLambda : public ILambda
	{
		typedef std::function<IPromise*(std::exception)> lambdatype;

	public:
		RejectedLambda(lambdatype l)
			: _lam(l)
		{
			//do nothing
		}

		virtual IPromise* call(State* stat)
		{
			std::exception reason = stat->getReason();
			IPromise* p = _lam(reason);

			return p;
		}

	private:
		lambdatype _lam;

		virtual void call(IPromise* prom)
		{
			//do nothing
		}
	};

	template <typename T>
	class ResolvedLambda : public ILambda
	{
		typedef std::function<IPromise*(T)> lambdatype;

	public:
		ResolvedLambda(lambdatype l)
			: _lam(l)
		{
			//do nothing
		}

		virtual IPromise* call(State* stat)
		{
			T *value = (T *)stat->getValue();
			IPromise* p = _lam(*value);

			return p;
		}

	private:
		lambdatype _lam;

		virtual void call(IPromise* prom)
		{
			//do nothing
		}
	};

	class Promise : public IPromise
	{
		friend class Pool;

		template <typename T>
		friend T* await(IPromise*);

	public:
		Promise(void)
			: _id(std::to_string(_promiseID + 1)),
			_state(nullptr),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;
		}

		Promise(ILambda* lam)
			: _id(std::to_string(_promiseID + 1)),
			_state(nullptr),
			_settleHandle(lam),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;
			_settle();
		}

		virtual ~Promise(void)
		{
			try
			{

				_th.join();
			}
			catch (const std::exception &e)
			{
				//suppress a thread join in destructor
				//because join potentially throws exception
				//and that's not allowed in destructors because
				//a thrown exception can stop the de-allocation
				//of further memory.
			}

			if (_settleHandle != nullptr)
				delete _settleHandle;

			if (_resolveHandle != nullptr)
				delete _resolveHandle;

			if (_rejectHandle != nullptr)
				delete _rejectHandle;
		}

		template <typename T>
		Promise* then(std::function<Promise*(T)> resolver, std::function<Promise*(std::exception)> rejecter = nullptr)
		{
			Promise* continuation = nullptr;
			ILambda* reslam = nullptr;
			ILambda* rejlam = nullptr;

			//this check is necessary so the proper nullptr's are passed into continuation promise
			if (resolver != nullptr)
				reslam = memory_pool->allocate<ResolvedLambda<T>>(resolver);

			if (rejecter != nullptr)
				rejlam = memory_pool->allocate<RejectedLambda>(rejecter);

			_stateLock.lock();

			if (_state == nullptr || *_state == Pending)
			{
				continuation = memory_pool->allocate<Promise>(reslam, rejlam);
				_Promises.push_back(continuation);
			}
			else if (*_state == Resolved)
			{
				continuation = memory_pool->allocate<Promise>(reslam, this->_state);
			}
			else
			{
				continuation = memory_pool->allocate<Promise>(rejlam, this->_state);
			}

			_stateLock.unlock();

			finisher.add_promise(continuation);

			return continuation;
		}

		Promise* _catch(std::function<Promise*(std::exception)> rejecter)
		{
			return then<char>(nullptr, rejecter);
		}

		template <typename T>
		Promise* finally(std::function<Promise*(T)> handler)
		{
			Promise* continuation = nullptr;
			ILambda* lam = memory_pool->allocate<ResolvedLambda<T>>(handler);

			_stateLock.lock();

			if (_state == nullptr || *_state == Pending)
			{
				ILambda* auxilliaryLam = memory_pool->allocate<ResolvedLambda<T>>(handler);
				continuation = memory_pool->allocate<Promise, ILambda*, ILambda*>(lam, auxilliaryLam);
				_Promises.push_back(continuation);
			}
			else
			{
				continuation = memory_pool->allocate<Promise, ILambda*, State*>(lam, _state);
			}

			_stateLock.unlock();

			finisher.add_promise(continuation);

			return continuation;
		}

		virtual State* getState(void)
		{
			return this->_state;
		}

	private:
		Promise(State* stat)
			: _id(std::to_string(_promiseID + 1)),
			_state(stat),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;
		}

		Promise(ILambda* lam, State* parentState)
			: _id(std::to_string(_promiseID + 1)),
			_state(nullptr),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;

			if (*parentState == Resolved)
			{
				_resolveHandle = lam;
				this->_settle(parentState, nullptr);
			}
			else if (*parentState == Rejected)
			{
				_rejectHandle = lam;
				this->_settle(nullptr, parentState);
			}
		}

		Promise(ILambda* res, ILambda* rej)
			: _id(std::to_string(_promiseID + 1)),
			_state(nullptr),
			_settleHandle(nullptr),
			_resolveHandle(res),
			_rejectHandle(rej)
		{
			++_promiseID;
		}

		std::string _id;
		State* _state;
		ILambda* _settleHandle;
		ILambda* _resolveHandle;
		ILambda* _rejectHandle;
		std::condition_variable _cv;
		std::mutex _stateLock;
		std::thread _th;
		std::vector<Promise*> _Promises;

		virtual void _resolve(State* state)
		{
			_stateLock.lock();
			_state = state;
			_stateLock.unlock();

			_cv.notify_all();

			for (size_t i = 0; i < _Promises.size(); i++)
			{
				_Promises[i]->_settle(_state, nullptr);
			}
		}

		virtual void _reject(State* state)
		{
			_stateLock.lock();
			_state = state;
			_stateLock.unlock();

			_cv.notify_all();

			for (size_t i = 0; i < _Promises.size(); i++)
			{
				_Promises[i]->_settle(nullptr, _state);
			}
		}

		virtual void Join(void)
		{
			this->_th.join();
			std::unique_lock<std::mutex> lk(this->_stateLock);

			if(*_state == Pending)
				this->_cv.wait(lk);
		}

		void _withSettleHandle(void)
		{
			_settleHandle->call(this);
		}

		void _withResolveHandle(State* input)
		{
			//surround this in a try block
			//so if an exception happens, then the promise is rejected instead.

			//we need 2 seperate functions between this and _withRejectHandle
			//because we need to call two different
			IPromise* parent = _resolveHandle->call(input);
			State* stat = parent->getState();

			//if the resolve handle succeeds, the the promise chain
			//is stil continuing with successful runs
			if (*stat == Resolved)
				_resolve(stat);
			else
				_reject(stat);
		}

		void _withRejectHandle(State* input)
		{
			//surroung this in a try block
			//so if an exception happens, then the promise is rejected instead.
			IPromise* parent = _rejectHandle->call(input);
			State* stat = parent->getState();

			//if the reject handle succeeds, then the promise chain
			//has been recovered and resolved
			if (*stat == Resolved)
				_resolve(stat);
			else
				_reject(stat);
		}

		void _settle(void)
		{
			_th = std::thread(&Promise::_withSettleHandle, this);
		}

		void _settle(State* withValue, State* withReason)
		{
			if (withValue != nullptr)
			{
				//run resolveHandle if this promise has one
				if (_resolveHandle != nullptr)
				{
					_th = std::thread(&Promise::_withResolveHandle, this, withValue);
				}

				//otherwise this promise doesn't have a resolve handle
				//and the withValue needs to be percolated down.
				else
				{
					_resolve(withValue);
				}
			}
			else if (withReason != nullptr)
			{
				//run rejectHandle if this promise has one
				if (_rejectHandle != nullptr)
				{
					_th = std::thread(&Promise::_withRejectHandle, this, withReason);
				}

				//otherwise this promise doesn't have a reject handle
				//and the exception needs to be percolated down.
				else
				{
					_reject(withReason);
				}
			}
		}
	};

	class Settlement
	{
	public:
		Settlement(void)
			: _prom(nullptr)
		{
			//do nothing
		}

		Settlement(IPromise* p)
			: _prom(p)
		{
			//doe nothing
		}

		Settlement(const Settlement &settle)
			: _prom(settle._prom)
		{
			//do nothing
		}

		~Settlement(void)
		{
			//does nothing
		}

		template <typename T>
		void resolve(T v)
		{
			T *value = memory_pool->allocate<T>();
			*value = v;
			State* state = memory_pool->allocate<ResolvedState<T>, T*>(value);
			_prom->_resolve(state);
		}

		void reject(std::exception e)
		{
			State* state = memory_pool->allocate<RejectedState, std::exception>(e);
			_prom->_reject(state);
		}

	private:
		IPromise *_prom;
	};

	class SettlementLambda : public ILambda
	{
	public:
		SettlementLambda(std::function<void(Settlement)> l)
			: _lam(l)
		{
			//do nothing
		}

		virtual void call(IPromise *prom)
		{
			Settlement sett(prom);
			_lam(sett);
		}

	private:
		std::function<void(Settlement)> _lam;

		virtual IPromise* call(State* stat)
		{
			return nullptr;
		}
	};

	Promise* Reject(std::exception e)
	{
		State* state = memory_pool->allocate<RejectedState>(e);

		Promise* prom = memory_pool->allocate<Promise, State*>(state);

		return prom;
	}

	template <typename T>
	Promise* Resolve(T v)
	{
		T *value = memory_pool->allocate<T>();
		*value = v;
		State* state = memory_pool->allocate<ResolvedState<T>>(value);

		Promise* prom = memory_pool->allocate<Promise, State*>(state);

		return prom;
	}
	

	Promise* all(std::vector<Promise*> &promises)
	{

		Promise* continuation = nullptr;

		//create the continuation promise as a user defined promise
		//and begin going through the given list of promises

		SettlementLambda* l = memory_pool->allocate<SettlementLambda>([&](Settlement settle) {
			//create the list for results and an
			//exceptin for rejected reason
			std::vector<int> results;
			std::exception reason;
			Status statflag = Pending;

			//lock for adding to list or creating the rejected reason
			//using mutex instead of semaphore because we need
			//ownership to be released when control exits scope
			//not before the return statement.
			std::mutex checkLock;

			//create resolve and reject handles for the list
			auto res = [&](int value) {
				std::unique_lock<std::mutex> reslock(checkLock);

				if (statflag == Pending)
					results.push_back(value);

				return Promises::Resolve<bool>(true);
			};

			auto rej = [&](std::exception e) {
				std::unique_lock<std::mutex> rejlock(checkLock);

				if (statflag == Pending)
				{
					reason = e;
					statflag = Rejected;
				}

				return Promises::Reject(e);
			};

			//loop through the promises
			for (int i = 0; i < promises.size(); ++i)
				promises[i]->then<int>(res, rej);

			while (statflag == Pending)
			{
				checkLock.lock();

				if (results.size() >= promises.size())
				{
					settle.resolve<std::vector<int>>(results);
					statflag = Resolved;
				}

				if (statflag == Rejected)
					settle.reject(reason);

				checkLock.unlock();
			}
		});

		continuation = memory_pool->allocate<Promise, SettlementLambda*>(l);
		finisher.add_promise(continuation);

		return continuation;
	}

	Promise* hash(std::map<std::string, Promises::Promise*> &promises)
	{

		Promise* continuation = nullptr;

		//create the continuation promise as a user defined promise
		//and begin going through the given list of promises

		SettlementLambda* l = memory_pool->allocate<SettlementLambda>([&](Settlement settle) {
			//create the map for results and an
			//exceptin for rejected reason
			std::map<std::string, int> results;
			std::exception reason;
			Status statflag = Pending;

			//still using our synchronization mechanisms because internally
			//a map is a red-black tree, so the system can potentially
			//look at same memory location to place a key-value pair.

			//may not need synchronization methods actually because
			//apparently all STL containers, such as a map, are required
			//to be thread safe by C++ docs.
			std::mutex checkLock;

			//loop through the promises
			for (auto it = promises.begin(); it != promises.end(); it++)
			{
				it->second->then<int>([&](int value) {

					std::unique_lock<std::mutex> reslock(checkLock);

					if (statflag == Pending)
						results.insert(std::pair<std::string, int>(it->first, value));

					return Promises::Resolve<bool>(true); }, [&](std::exception e) {

						std::unique_lock<std::mutex> rejlock(checkLock);

						if (statflag == Pending)
						{
							reason = e;
							statflag = Rejected;
						}

						return Promises::Reject(e); });
			}

			while (statflag == Pending)
			{
				checkLock.lock();

				if (results.size() >= promises.size())
				{
					settle.resolve<std::map<std::string, int>>(results);
					statflag = Resolved;
				}

				if (statflag == Rejected)
					settle.reject(reason);

				checkLock.unlock();
			}
		});

		continuation = memory_pool->allocate<Promise, SettlementLambda*>(l);
		finisher.add_promise(continuation);

		return continuation;
	}


	//await - suspend execution until the given promise is settled.
	//If promise failed, the reject reason is thrown.
	template <typename T>
	T* await(IPromise* prom) {
		prom->Join();
		std::cout << "in await" << std::endl;
		State* s = prom->getState();

		if (*s == Rejected) {
			throw s->getReason();
		}

		T* value = (T*)s->getValue();
		return value;
	}

} // namespace Promises

Promises::Promise* promise(std::function<void(Promises::Settlement)> func)
{
	Promises::SettlementLambda *l = Promises::memory_pool->allocate<Promises::SettlementLambda, std::function<void(Promises::Settlement)>>(func);
	Promises::Promise *prom = Promises::memory_pool->allocate<Promises::Promise, Promises::SettlementLambda*>(l);

	Promises::finisher.add_promise(prom);

	if (!Promises::exit_handle_created) {
		std::atexit(Promises::fin);
		Promises::exit_handle_created = true;
	}

	return prom;
}

#endif // !PROMISE_H