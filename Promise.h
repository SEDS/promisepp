#ifndef PROMISE_H
#define PROMISE_H

#include "IPromise.h"
#include "Lambda.h"
#include "Pool.h"
#include "State.h"
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
#include <type_traits>

namespace Promises {

	static int _promiseID = 0;

	//init the memory pool as a unique ptr so it will be destructed once program terminates
	static std::unique_ptr<Promises::Pool> memory_pool(Promises::Pool::Instance());

	class Settlement {
	public:
		Settlement(IPromise* p)
			: _prom(p)
		{ }

		Settlement(const Settlement &settle)
			: _prom(settle._prom)
		{ }

		~Settlement(void) { }

		template <typename T>
		void resolve(T v) {
			if (_prom == NULL || _prom == nullptr) {
				throw Promise_Error("Settlement.resolve(): internal promise is null");
			}

			T *value = memory_pool->allocate<T>();
			*value = v;
			State* state = memory_pool->allocate<ResolvedState<T>, T*>(value);
			_prom->_resolve(state);
		}

		void reject(const std::exception &e) {
			if (_prom == NULL || _prom == nullptr) {
				throw Promise_Error("Settlement.reject(): internal promise is null");
			}

			State* state = memory_pool->allocate<RejectedState, const std::exception&>(e);
			_prom->_reject(state);
		}
		
		void reject(const std::string &msg) {
			if (_prom == NULL || _prom == nullptr) {
				throw Promise_Error("Settlement.reject(): internal promise is null");
			}

			State* state = memory_pool->allocate<RejectedState, const std::string&>(msg);
			_prom->_reject(state);
		}

	private:
		IPromise *_prom;
	};

	template<typename LAMBDA>
	class SettlementLambda : public ILambda {
	public:
		SettlementLambda(LAMBDA l)
			: _lam(l)
		{ }

		virtual void call(IPromise *prom) {
			Settlement sett(prom);
			_lam(sett);
		}

	private:
		LAMBDA _lam;

		virtual IPromise* call(State* stat) {
			return nullptr;
		}
	};

	//NoArg Promise and Lambda
	//used only for Promise.finally()
	class NoArgPromise : public IPromise {
		public:
			NoArgPromise(State* state)
				:_state(state)
			{ }

			virtual ~NoArgPromise(void) {}

			virtual State* get_state(void) {
				return _state;
			}
		private:
			State* _state;
			virtual void _resolve(State* state) { }
			virtual void _reject(State* state) { }
			virtual void Join(void) { }
	};
	
	template <typename LAMBDA>
	class NoArgLambda : public ILambda {
	public:
		NoArgLambda(LAMBDA l)
			: _lam(l)
		{ }

		virtual IPromise* call(State* stat) {			
			_lam();
			//The state needs to bubble downstream
			NoArgPromise* prom = memory_pool->allocate<NoArgPromise>(stat);
			return prom;
		}

	private:
		LAMBDA _lam;

		virtual void call(IPromise* prom) { }
	};
	
	template<typename LAMBDA>
	SettlementLambda<LAMBDA>* create_settlement_lambda(LAMBDA handler) {
		SettlementLambda<LAMBDA>* lam = nullptr;
		lam = memory_pool->allocate<SettlementLambda<LAMBDA>>(handler);
		return lam;
	}
	
	class Semaphore {
		public:
			Semaphore(void)
				:_value(0)
			{ }
		
			bool test_decrease(){
				std::unique_lock<std::mutex> lock(_lock);
				if(_value > 0) {
					--_value;
					return true;
				}
				return false;
			}
		
			void decrease(void) {
				std::unique_lock<std::mutex> lock(_lock);
				
				while(_value <= 0) {
					_cond.wait(lock);
				}
				
				--_value;
			}
			
			void increase(void) {
				std::unique_lock<std::mutex> lock(_lock);
				++_value;
				_cond.notify_one();
			}
		private:
			size_t _value;
			std::mutex _lock;
			std::condition_variable _cond;
	};

	class Promise : public IPromise {
	
		//pool is friend because
		//it has to be able to use the private constructors
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
			_state(&pending_state),
			_settleHandle(lam),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;
			_settle();
		}

		virtual ~Promise(void) {
			try {

				_th.join();
			} catch (const std::exception &e) {
				//suppress a thread join in destructor
				//because join potentially throws exception
				//and that's not allowed in destructors because
				//a thrown exception can stop the de-allocation
				//of further memory.
			}
		}

		template <typename RESLAM, typename REJLAM>
		Promise* then(RESLAM resolver, REJLAM rejecter) {
			if (_state == NULL || _state == nullptr) {
				throw Promise_Error("Promise.then(): state is null");
			}

			Promise* continuation = nullptr;
			ILambda* reslam = nullptr;
			ILambda* rejlam = nullptr;

			//this check is necessary so the proper nullptr's are passed into continuation promise
			reslam = memory_pool->allocate<ResolvedLambda<RESLAM>>(resolver);
			rejlam = memory_pool->allocate<RejectedLambda<REJLAM>>(rejecter);

			_stateLock.lock();

			if (*_state == Pending) {
				continuation = memory_pool->allocate<Promise>(reslam, rejlam);
				_Promises.push_back(continuation);
			} else if (*_state == Resolved) {
				continuation = memory_pool->allocate<Promise>(reslam, this->_state);
			} else {
				continuation = memory_pool->allocate<Promise>(rejlam, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}
		
		template <typename LAMBDA>
		Promise* then(LAMBDA resolver) {
			if (_state == NULL || _state == nullptr) {
				throw Promise_Error("Promise.then(): state is null");
			}

			Promise* continuation = nullptr;
			ILambda* lam = nullptr;
			
			_stateLock.lock();

			if (*_state == Pending) {
				lam = memory_pool->allocate<ResolvedLambda<LAMBDA>>(resolver);
				ILambda* fake = nullptr;
				continuation = memory_pool->allocate<Promise>(lam, fake);
				_Promises.push_back(continuation);
			} else if (*_state == Resolved) {
				lam = memory_pool->allocate<ResolvedLambda<LAMBDA>>(resolver);
				continuation = memory_pool->allocate<Promise>(lam, this->_state);
			} else {
				ILambda* fake = nullptr;
				continuation = memory_pool->allocate<Promise>(fake, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}
	
		template<typename REJLAM>
		Promise* _catch(REJLAM rejecter) {
			if (_state == NULL || _state == nullptr) {
				throw Promise_Error("Promise.catch(): state is null");
			}

			Promise* continuation = nullptr;
			ILambda* lam = nullptr;
						
			_stateLock.lock();
			if (*_state == Pending) {
				ILambda* fake = nullptr;
				lam = memory_pool->allocate<RejectedLambda<REJLAM>>(rejecter);
				continuation = memory_pool->allocate<Promise>(fake, lam);
				_Promises.push_back(continuation);
			} else if (*_state == Resolved) {
				ILambda* fake = nullptr;
				continuation = memory_pool->allocate<Promise>(fake, this->_state);
			} else {
				lam = memory_pool->allocate<RejectedLambda<REJLAM>>(rejecter);
				continuation = memory_pool->allocate<Promise>(lam, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}

		template <typename LAMBDA>
		Promise* finally(LAMBDA handler) {
			if (_state == NULL || _state == nullptr) {
				throw Promise_Error("Promise.finally(): state is null");
			}

			Promise* continuation = nullptr;
			ILambda* lam = memory_pool->allocate<NoArgLambda<LAMBDA>>(handler);

			_stateLock.lock();

			if (*_state == Pending) {
				ILambda* aux = memory_pool->allocate<NoArgLambda<LAMBDA>>(handler);
				continuation = memory_pool->allocate<Promise, ILambda*, ILambda*>(lam, aux);
				_Promises.push_back(continuation);
			} else {
				continuation = memory_pool->allocate<Promise, ILambda*, State*>(lam, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}

		virtual State* get_state(void) {
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
			_state(&pending_state),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			++_promiseID;

			if (*parentState == Resolved) {
				_resolveHandle = lam;
				this->_settle(parentState, nullptr);
			} else if (*parentState == Rejected) {
				_rejectHandle = lam;
				this->_settle(nullptr, parentState);
			}
		}

		Promise(ILambda* res, ILambda* rej)
			: _id(std::to_string(_promiseID + 1)),
			_state(&pending_state),
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
		Semaphore _semp;
		std::mutex _stateLock;
		std::thread _th;
		std::vector<Promise*> _Promises;

		virtual void _resolve(State* state) {
			_stateLock.lock();
			_state = state;
			_stateLock.unlock();

			_semp.increase();

			for (size_t i = 0; i < _Promises.size(); i++) {
				_Promises[i]->_settle(_state, nullptr);
			}
		}

		virtual void _reject(State* state) {
			_stateLock.lock();
			_state = state;
			_stateLock.unlock();

			_semp.increase();

			for (size_t i = 0; i < _Promises.size(); i++) {
				_Promises[i]->_settle(nullptr, _state);
			}
		}

		virtual void Join(void) {
			//wait for promise to have state.
			if (_state == nullptr || *_state == Pending) {
				if (!_semp.test_decrease()) {
					_semp.decrease();
				}
			}

			if (this->_th.joinable()) {
				this->_th.join();
			}
		}

		void _withSettleHandle(void) {
			_settleHandle->call(this);
		}

		void _withResolveHandle(State* input) {
			//surround this in a try block
			//so if an exception happens, then the promise is rejected instead.

			//we need 2 seperate functions between this and _withRejectHandle
			//because we need to call two different
			IPromise* parent = _resolveHandle->call(input);
			
			//parent will only be nullptr on chain end
			if(parent != nullptr) {
				State* state = parent->get_state();

				//if the resolve handle succeeds, the the promise chain
				//is stil continuing with successful runs
				if (*state == Resolved)
					_resolve(state);
				else
					_reject(state);
			} else {
				_stateLock.lock();
				_state = nullptr;
				_stateLock.unlock();

				_semp.increase();
			}
		}

		void _withRejectHandle(State* input) {
			//surroung this in a try block
			//so if an exception happens, then the promise is rejected instead.
			IPromise* parent = _rejectHandle->call(input);
			
			//parent will only be nullptr on chain end
			if(parent != nullptr) {
				State* state = parent->get_state();

				//if the resolve handle succeeds, the the promise chain
				//is stil continuing with successful runs
				if (*state == Resolved)
					_resolve(state);
				else
					_reject(state);
			} else {
				_stateLock.lock();
				_state = nullptr;
				_stateLock.unlock();

				_semp.increase();
			}
		}

		void _settle(void) {
			_th = std::thread(&Promise::_withSettleHandle, this);
		}

		void _settle(State* withValue, State* withReason) {
			if (withValue != nullptr) {
				//run resolveHandle if this promise has one
				if (_resolveHandle != nullptr) {
					_th = std::thread(&Promise::_withResolveHandle, this, withValue);
				}

				//otherwise this promise doesn't have a resolve handle
				//and the withValue needs to be percolated down.
				else {
					_resolve(withValue);
				}
			}
			else if (withReason != nullptr) {
				//run rejectHandle if this promise has one
				if (_rejectHandle != nullptr) {
					_th = std::thread(&Promise::_withRejectHandle, this, withReason);
				}

				//otherwise this promise doesn't have a reject handle
				//and the exception needs to be percolated down.
				else {
					_reject(withReason);
				}
			}
		}
	};

	Promise* Reject(const std::exception &e) {
		State* state = memory_pool->allocate<RejectedState, const std::exception&>(e);

		Promise* prom = memory_pool->allocate<Promise, State*>(state);

		return prom;
	}

	template <typename T>
	Promise* Resolve(T v) {
		T *value = memory_pool->allocate<T>();
		*value = v;
		State* state = memory_pool->allocate<ResolvedState<T>, T*>(value);

		Promise* prom = memory_pool->allocate<Promise, State*>(state);

		return prom;
	}
	
	//await - suspend execution until the given promise is settled.
	//If promise failed, the reject reason is thrown.
	template <typename T>
	T* await(IPromise* prom) {
		prom->Join();
		State* s = prom->get_state();
		
		T* value = nullptr;

		if (s != nullptr && *s == Rejected) {
			const std::exception &e = s->get_reason();
			throw Promises::Promise_Error(e.what());
		} else if (s != nullptr && *s == Resolved) {
			value = (T*)s->get_value();
		}
		
		return value;
	}

	template<typename COMMONTYPE>
	Promise* all(std::vector<IPromise*> &promises) {

		Promise* continuation = nullptr;

		ILambda* lam = create_settlement_lambda([&promises](Settlement settle) {
			//create the list for results
			std::vector<COMMONTYPE> results;

			//loop through the promises
			size_t i = 0;
			for (i = 0; i < promises.size(); ++i) {
				try {
					COMMONTYPE* value = await<COMMONTYPE>(promises[i]);
					results.push_back(*value);
				} catch (const std::exception &ex) {
					settle.reject(Promise_Error(ex.what()));
					i = promises.size()+1;
				}
			}
			
			if (i < (promises.size()+1)) {
				settle.resolve<std::vector<COMMONTYPE>>(results);
			}
		});

		continuation = memory_pool->allocate<Promise, ILambda*>(lam);

		return continuation;
	}

	template<typename KEYTYPE, typename COMMONTYPE>
	Promise* hash(std::map<KEYTYPE, Promises::IPromise*> &promises) {

		Promise* continuation = nullptr;

		ILambda* lam = create_settlement_lambda([&promises](Settlement settle) {
			std::map<KEYTYPE, COMMONTYPE> results;
			bool early_termination = false;
			
			//loop through the promises
			for (auto it = promises.begin(); it != promises.end(); it++) {
				try {
					COMMONTYPE* value = await<COMMONTYPE>(it->second);
					results.insert(std::pair<KEYTYPE, COMMONTYPE>(it->first, *value));
				} catch (const std::exception &ex) {
					settle.reject(Promise_Error(ex.what()));
					it = promises.end();
					it++;
					early_termination = true;
				}
			}
			
			if (!early_termination) {
				settle.resolve<std::map<KEYTYPE, COMMONTYPE>>(results);
			}
		});

		continuation = memory_pool->allocate<Promise, ILambda*>(lam);

		return continuation;
	}
} // namespace Promises

template<typename LAMBDA>
Promises::Promise* promise(LAMBDA handle) {
	Promises::SettlementLambda<LAMBDA> *l = Promises::create_settlement_lambda<LAMBDA>(handle);
	Promises::Promise *prom = Promises::memory_pool->allocate<Promises::Promise, Promises::SettlementLambda<LAMBDA>*>(l);

	return prom;
}

#endif // !PROMISE_H
