#ifndef PROMISE_H
#define PROMISE_H

#include <functional>
#include <stdexcept>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <iterator>
#include <map>
#include <iostream>

namespace Promises {

	static int _promiseID = 0;

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
			:_status(Pending)
		{
			//do nothing
		}

		State(Status stat)
			:_status(stat)
		{
			//do nothing
		}

		State(const State &state)
			:_status(state._status)
		{
			//do nothing
		}

		virtual ~State(void) { }

		bool operator == (Status stat)
		{
			return this->_status == stat;
		}

		bool operator != (Status stat)
		{
			return this->_status != stat;
		}

		virtual void* getValue(void) = 0;
		virtual std::exception getReason(void) = 0;

	private:
		Status _status;
	};

	template<typename T>
	class ResolveState : public State
	{
	public:
		ResolveState(void)
			:_value(nullptr)
		{
			//do nothing
		}

		ResolveState(T* v)
			:State(Resolved),
			_value(v)
		{
			//do nothing
		}

		ResolveState(const ResolveState &state)
			:State(state),
			_value(state._value)
		{
			//do nothing
		}

		virtual ~ResolveState(void)
		{
			delete _value;
		}

		virtual void* getValue(void)
		{
			return _value;
		}
		virtual std::exception getReason(void)
		{
			return std::logic_error("NO ERR");
		}
	private:
		T* _value;
	};

	class RejectState : public State
	{
	public:
		RejectState(void)
			:_reason(std::logic_error("NO ERR"))
		{
			//do nothing
		}

		RejectState(std::exception e)
			:State(Rejected),
			_reason(e)
		{
			//do nothing
		}

		RejectState(const RejectState &state)
			:State(state),
			_reason(state._reason)
		{
			//do nothing
		}

		virtual ~RejectState(void)
		{
			//do nothing
		}

		virtual void* getValue(void)
		{
			return nullptr;
		}
		virtual std::exception getReason(void)
		{
			return _reason;
		}
	private:
		std::exception _reason;
	};

	class ReturnablePromise
	{
	public:
		virtual ~ReturnablePromise(void) {}
		virtual void _resolve(std::shared_ptr<State> state) = 0;
		virtual void _reject(std::shared_ptr<State> state) = 0;
		virtual std::shared_ptr<State> getState(void) = 0;
	};

	class LambdaBase
	{
	public:
		virtual ~LambdaBase(void) {}
		virtual void call(ReturnablePromise* prom) = 0;
		virtual std::shared_ptr<ReturnablePromise> call(std::shared_ptr<State> stat) = 0;
	};

	class LambdaExcept : public LambdaBase
	{
		typedef std::function<std::shared_ptr<ReturnablePromise>(std::exception)> lambdatype;

	public:
		LambdaExcept(lambdatype l)
			:_lam(l)
		{
			//do nothing
		}

		virtual std::shared_ptr<ReturnablePromise> call(std::shared_ptr<State> stat)
		{
			std::exception reason = stat->getReason();
			std::shared_ptr<ReturnablePromise> p = _lam(reason);

			return p;
		}

	private:
		lambdatype _lam;

		virtual void call(ReturnablePromise* prom)
		{
			//do nothing
		}
	};

	template<typename T>
	class Lambda : public LambdaBase
	{
		typedef std::function<std::shared_ptr<ReturnablePromise>(T)> lambdatype;

	public:
		Lambda(lambdatype l)
			:_lam(l)
		{
			//do nothing
		}

		virtual std::shared_ptr<ReturnablePromise> call(std::shared_ptr<State> stat)
		{
			T* value = (T*)stat->getValue();
			std::shared_ptr<ReturnablePromise> p = _lam(*value);

			return p;
		}

	private:
		lambdatype _lam;

		virtual void call(ReturnablePromise* prom)
		{
			//do nothing
		}
	};

	class PendingPromise : public ReturnablePromise
	{
	public:
		PendingPromise(void)
			:_id(std::to_string(_promiseID + 1)),
			_state(nullptr),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;
		}

		PendingPromise(std::shared_ptr<LambdaBase> lam)
			:_id(std::to_string(_promiseID + 1)),
			_state(nullptr),
			_settleHandle(lam),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;
			_settle();
		}

		PendingPromise(std::shared_ptr<State> stat)
			:_id(std::to_string(_promiseID + 1)),
			_state(stat),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;
		}

		PendingPromise(std::shared_ptr<LambdaBase> lam, std::shared_ptr<State> parentState)
			:_id(std::to_string(_promiseID + 1)),
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

		PendingPromise(std::shared_ptr<LambdaBase> res, std::shared_ptr<LambdaBase> rej)
			:_id(std::to_string(_promiseID + 1)),
			_state(nullptr),
			_settleHandle(nullptr),
			_resolveHandle(res),
			_rejectHandle(rej)
		{
			++_promiseID;
		}

		virtual ~PendingPromise(void)
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

			// if (_settleHandle != nullptr)
			// 	delete _settleHandle;

			// if (_resolveHandle != nullptr)
			// 	delete _resolveHandle;

			// if (_rejectHandle != nullptr)
			// 	delete _rejectHandle;
		}

		template<typename T>
		std::shared_ptr<PendingPromise> then(std::function<std::shared_ptr<PendingPromise>(T)> resolver, std::function<std::shared_ptr<PendingPromise>(std::exception)> rejecter = nullptr)
		{
			std::shared_ptr<PendingPromise> continuation = nullptr;
			std::shared_ptr<LambdaBase> reslam = nullptr;
			std::shared_ptr<LambdaBase> rejlam = nullptr;

			//this check is necessary so the proper nullptr's are passed into continuation promise
			if (resolver != nullptr)
				reslam = std::make_shared<Lambda<T>>(resolver);

			if (rejecter != nullptr)
				rejlam = std::make_shared<LambdaExcept>(rejecter);

			_stateLock.lock();

			if (_state == nullptr || *_state == Pending)
			{
				continuation = std::make_shared<PendingPromise>(reslam, rejlam);
				_pendingPromises.push_back(continuation);
			}
			else if (*_state == Resolved)
			{
				continuation = std::make_shared<PendingPromise>(reslam, this->_state);
			}
			else
			{
				continuation = std::make_shared<PendingPromise>(rejlam, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}

		std::shared_ptr<PendingPromise> _catch(std::function<std::shared_ptr<PendingPromise>(std::exception)> rejecter)
		{
			return then<char>(nullptr, rejecter);
		}

		template<typename T>
		std::shared_ptr<PendingPromise> finally(std::function<std::shared_ptr<PendingPromise>(T)> handler)
		{
			std::shared_ptr<PendingPromise> continuation = nullptr;
			std::shared_ptr<LambdaBase> lam = std::make_shared<Lambda<T>>(handler);

			_stateLock.lock();

			if (_state == nullptr || *_state == Pending)
			{
				std::shared_ptr<LambdaBase> auxilliaryLam = std::make_shared<Lambda<T>>(handler);
				continuation = std::make_shared<PendingPromise>(lam, auxilliaryLam);
				_pendingPromises.push_back(continuation);
			}
			else
			{
				continuation = std::make_shared<PendingPromise>(lam, _state);
			}

			_stateLock.unlock();

			return continuation;
		}

		virtual void _resolve(std::shared_ptr<State> state)
		{
			_stateLock.lock();
			_state = state;
			_stateLock.unlock();

			for (size_t i = 0; i < _pendingPromises.size(); i++)
			{
				_pendingPromises[i]->_settle(_state, nullptr);
			}
		}

		virtual void _reject(std::shared_ptr<State> state)
		{
			_stateLock.lock();
			_state = state;
			_stateLock.unlock();

			for (size_t i = 0; i < _pendingPromises.size(); i++)
			{
				_pendingPromises[i]->_settle(nullptr, _state);
			}
		}

		virtual std::shared_ptr<State> getState(void)
		{
			return this->_state;
		}

	private:
		std::string _id;
		std::shared_ptr<State> _state;
		std::shared_ptr<LambdaBase> _settleHandle;
		std::shared_ptr<LambdaBase> _resolveHandle;
		std::shared_ptr<LambdaBase> _rejectHandle;
		std::mutex _stateLock;
		std::thread _th;
		//is this good design? A vector of shared pointers that are on the stack?
		std::vector<std::shared_ptr<PendingPromise>> _pendingPromises;

		void _withSettleHandle(void)
		{
			_settleHandle->call(this);
		}

		void _withResolveHandle(std::shared_ptr<State> input)
		{
			//surround this in a try block
			//so if an exception happens, then the promise is rejected instead.

			//we need 2 seperate functions between this and _withRejectHandle
			//because we need to call two different
			std::shared_ptr<ReturnablePromise> parent = _resolveHandle->call(input);
			std::shared_ptr<State> stat = parent->getState();

			//if the resolve handle succeeds, the the promise chain
			//is stil continuing with successful runs
			if (*stat == Resolved)
				_resolve(stat);
			else
				_reject(stat);
		}

		void _withRejectHandle(std::shared_ptr<State> input)
		{
			//surroung this in a try block
			//so if an exception happens, then the promise is rejected instead.
			std::shared_ptr<ReturnablePromise> parent = _rejectHandle->call(input);
			std::shared_ptr<State> stat = parent->getState();

			//if the reject handle succeeds, then the promise chain
			//has been recovered and resolved
			if (*stat == Resolved)
				_resolve(stat);
			else
				_reject(stat);
		}

		void _settle(void)
		{
			_th = std::thread(&PendingPromise::_withSettleHandle, this);
		}

		void _settle(std::shared_ptr<State> withValue, std::shared_ptr<State> withReason)
		{
			if (withValue != nullptr)
			{
				//run resolveHandle if this promise has one
				if (_resolveHandle != nullptr)
				{
					_th = std::thread(&PendingPromise::_withResolveHandle, this, withValue);
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
					_th = std::thread(&PendingPromise::_withRejectHandle, this, withReason);
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
			:_prom(nullptr)
		{
			//do nothing
		}

		Settlement(ReturnablePromise* p)
			:_prom(p)
		{
			//doe nothing
		}

		Settlement(const Settlement &settle)
			:_prom(settle._prom)
		{
			//do nothing
		}

		~Settlement(void)
		{
			//does nothing
		}

		template<typename T>
		void resolve(T v)
		{
			T* value = new T;
			*value = v;
			std::shared_ptr<State> state = std::make_shared<ResolveState<T>>(value);
			_prom->_resolve(state);
		}

		void reject(std::exception e)
		{
			std::shared_ptr<State> state = std::make_shared<RejectState>(e);
			_prom->_reject(state);
		}

	private:
		ReturnablePromise* _prom;
	};

	class LambdaSettle : public LambdaBase
	{
	public:
		LambdaSettle(std::function<void(Settlement)> l)
			:_lam(l)
		{
			//do nothing
		}

		virtual void call(ReturnablePromise* prom)
		{
			Settlement sett(prom);
			_lam(sett);
		}

	private:
		std::function<void(Settlement)> _lam;

		virtual std::shared_ptr<ReturnablePromise> call(std::shared_ptr<State> stat)
		{
			return nullptr;
		}
	};

	std::shared_ptr<PendingPromise> Reject(std::exception e)
	{
		std::shared_ptr<State> state = std::make_shared<RejectState>(e);

		std::shared_ptr<PendingPromise> prom = std::make_shared<PendingPromise>(state);

		return prom;
	}

	template <typename T>
	std::shared_ptr<PendingPromise> Resolve(T v)
	{
		T* value = new T;
		*value = v;
		std::shared_ptr<State> state = std::make_shared<ResolveState<T>>(value);

		std::shared_ptr<PendingPromise> prom = std::make_shared<PendingPromise>(state);

		return prom;
	}

	std::shared_ptr<PendingPromise> all(std::vector<std::shared_ptr<PendingPromise>> &promises)
	{

		std::shared_ptr<PendingPromise> continuation = nullptr;

		//create the continuation promise as a user defined promise
		//and begin going through the given list of promises

		std::shared_ptr<LambdaSettle> l = std::make_shared<LambdaSettle>([&](Settlement settle) {
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
			for (int i = 0; i<promises.size(); ++i)
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

		continuation = std::make_shared<PendingPromise>(l);

		return continuation;
	}

	std::shared_ptr<PendingPromise> hash(std::map<std::string, std::shared_ptr<Promises::PendingPromise>> &promises)
	{

		std::shared_ptr<PendingPromise> continuation = nullptr;

		//create the continuation promise as a user defined promise
		//and begin going through the given list of promises

		std::shared_ptr<LambdaSettle> l = std::make_shared<LambdaSettle>([&](Settlement settle) {
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

					return Promises::Resolve<bool>(true);
				}, [&](std::exception e) {

					std::unique_lock<std::mutex> rejlock(checkLock);

					if (statflag == Pending)
					{
						reason = e;
						statflag = Rejected;
					}

					return Promises::Reject(e);
				});
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

		continuation = std::make_shared<PendingPromise>(l);
		return continuation;
	}

} //namespace has ended

std::shared_ptr<Promises::PendingPromise> promise(std::function<void(Promises::Settlement)> func)
{
	std::shared_ptr<Promises::LambdaSettle> l = std::make_shared<Promises::LambdaSettle>(func);
	std::shared_ptr<Promises::PendingPromise> prom = std::make_shared<Promises::PendingPromise>(l);

	return prom;
}

#endif // !PROMISE_H
