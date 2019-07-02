#ifndef PROMISE_H
#define PROMISE_H
#define ARENA_SIZE 256

#include "IPromise.h"
#include "Lambda.h"
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
#include <cstdlib>
#include <type_traits>
#include <iostream>

namespace Promises {

static std::shared_ptr<PendingState> pending_state = std::make_shared<PendingState>();

	template<typename LAMBDA>
	std::shared_ptr<RejectedLambda<LAMBDA>> rejected_lambda(LAMBDA lam) {
		std::shared_ptr<RejectedLambda<LAMBDA>> rejlam = std::make_shared<RejectedLambda<LAMBDA>>(lam);
		
		return rejlam;
	}

	template<typename LAMBDA>
	std::shared_ptr<ResolvedLambda<LAMBDA>> resolved_lambda(LAMBDA lam) {
		std::shared_ptr<ResolvedLambda<LAMBDA>> reslam = std::make_shared<ResolvedLambda<LAMBDA>>(lam);

		return reslam;
	}
	
	class Settlement {
	public:
		Settlement(IPromise* p)
			: _prom(p)
		{ }

		Settlement(const Settlement &settle)
			: _prom(settle._prom)
		{ }

		~Settlement(void) { }

		Settlement& operator = (const Settlement &settle) {
			this->_prom = settle._prom;
			return (*this);
		}

		template <typename T>
		void resolve(T value) {
			if (_prom == NULL || _prom == nullptr) {
				throw Promise_Error("Settlement.resolve(): internal promise is null");
			}
			
			std::shared_ptr<ResolvedState<T>> state = std::make_shared<ResolvedState<T>>(value);

			_prom->_resolve(state);
		}

		void reject(const std::exception &e) {
			if (_prom == NULL || _prom == nullptr) {
				throw Promise_Error("Settlement.reject(): internal promise is null");
			}

			std::shared_ptr<RejectedState> state = std::make_shared<RejectedState>(e);

			_prom->_reject(state);
		}
		
		void reject(const std::string &msg) {
			if (_prom == NULL || _prom == nullptr) {
				throw Promise_Error("Settlement.reject(): internal promise is null");
			}

			std::shared_ptr<RejectedState> state = std::make_shared<RejectedState>(msg);

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

		virtual std::shared_ptr<IPromise> call(std::shared_ptr<State> stat) {
			return nullptr;
		}
	};

	template<typename LAMBDA>
	std::shared_ptr<SettlementLambda<LAMBDA>> settlement_lambda(LAMBDA handler) {
		std::shared_ptr<SettlementLambda<LAMBDA>> settlam = std::make_shared<SettlementLambda<LAMBDA>>(handler);

		return settlam;
	}

	//NoArg Promise and Lambda
	//used only for Promise.finally()
	class NoArgPromise : public IPromise {		
		public:
			NoArgPromise(std::shared_ptr<State> state)
				:_state(state)
			{ }

			virtual ~NoArgPromise(void) {}

			virtual std::shared_ptr<State> get_state(void) {
				return _state;
			}
		private:
			std::shared_ptr<State> _state;
			virtual void _resolve(std::shared_ptr<State> state) { }
			virtual void _reject(std::shared_ptr<State> state) { }
			virtual void Join(void) { }
	};
	
	template <typename LAMBDA>
	class NoArgLambda : public ILambda {
	public:
		NoArgLambda(LAMBDA l)
			: _lam(l)
		{ }

		virtual std::shared_ptr<IPromise> call(std::shared_ptr<State> stat) {			
			_lam();
			//The state needs to bubble downstream
			std::shared_ptr<NoArgPromise> prom = std::make_shared<NoArgPromise>(stat);
			return prom;
		}

	private:
		LAMBDA _lam;

		virtual void call(IPromise* prom) { }
	};

	template<typename LAMBDA>
	std::shared_ptr<NoArgLambda<LAMBDA>> noarg_lambda(LAMBDA handler) {
		std::shared_ptr<NoArgLambda<LAMBDA>> noarglam = std::make_shared<NoArgLambda<LAMBDA>>(handler);
		
		return noarglam;
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

		template <typename T>
		friend T* await(std::shared_ptr<IPromise>);

	public:
		Promise(void)
			:_state(nullptr),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{ }

		Promise(std::shared_ptr<State> stat)
			:_state(stat),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{ }

		Promise(std::shared_ptr<ILambda> lam)
			:_state(pending_state),
			_settleHandle(lam),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			_settle();
		}

		Promise(std::shared_ptr<ILambda> lam, std::shared_ptr<State> parentState)
			:_state(pending_state),
			_settleHandle(nullptr),
			_resolveHandle(nullptr),
			_rejectHandle(nullptr)
		{
			if (*parentState == Resolved) {
				_resolveHandle = lam;
				this->_settle(parentState, nullptr);
			} else if (*parentState == Rejected) {
				_rejectHandle = lam;
				this->_settle(nullptr, parentState);
			}
		}

		Promise(std::shared_ptr<ILambda> res, std::shared_ptr<ILambda> rej)
			:_state(pending_state),
			_settleHandle(nullptr),
			_resolveHandle(res),
			_rejectHandle(rej)
		{ }

		Promise(const Promise &other)
			:_state(other._state),
			_settleHandle(other._settleHandle),
			_resolveHandle(other._resolveHandle),
			_rejectHandle(other._rejectHandle),
			_Promises(other._Promises)
		{ }

		virtual ~Promise(void) {
			try {
				if (this->_th.joinable()) {
					this->_th.join();
				}
			} catch (const std::exception &ex) {
				std::cout << ex.what() << std::endl;
			}
		}

		Promise& operator = (const Promise &other) {
			this->_state = other._state;
			this->_settleHandle = other._settleHandle;
			this->_resolveHandle = other._resolveHandle;
			this->_rejectHandle = other._rejectHandle;
			this->_Promises = 	other._Promises;

			return (*this);
		}

		template <typename RESLAM, typename REJLAM>
		std::shared_ptr<Promise> then(RESLAM resolver, REJLAM rejecter) {
			if (_state == NULL || _state == nullptr) {
				throw Promise_Error("Promise.then(): state is null");
			}

			std::shared_ptr<Promise> continuation = nullptr;
			std::shared_ptr<ILambda> reslam = nullptr;
			std::shared_ptr<ILambda> rejlam = nullptr;

			_stateLock.lock();

			if (*_state == Pending) {
				reslam = resolved_lambda<RESLAM>(resolver);
				rejlam = rejected_lambda<REJLAM>(rejecter);
				continuation = std::make_shared<Promise>(reslam, rejlam);
				_Promises.push_back(continuation);
			} else if (*_state == Resolved) {
				reslam = resolved_lambda<RESLAM>(resolver);
				continuation = std::make_shared<Promise>(reslam, this->_state);
			} else {
				rejlam = rejected_lambda<REJLAM>(rejecter);
				continuation = std::make_shared<Promise>(rejlam, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}
		
		template <typename LAMBDA>
		std::shared_ptr<Promise> then(LAMBDA resolver) {
			if (_state == NULL || _state == nullptr) {
				throw Promise_Error("Promise.then(): state is null");
			}

			std::shared_ptr<Promise> continuation = nullptr;
			std::shared_ptr<ILambda> lam = nullptr;
			
			_stateLock.lock();

			if (*_state == Pending) {
				std::shared_ptr<ILambda> fake = nullptr;
				lam = resolved_lambda<LAMBDA>(resolver);
				continuation = std::make_shared<Promise>(lam, fake);
				_Promises.push_back(continuation);
			} else if (*_state == Resolved) {
				lam = resolved_lambda<LAMBDA>(resolver);
				continuation = std::make_shared<Promise>(lam, this->_state);
			} else {
				std::shared_ptr<ILambda> fake = nullptr;
				continuation = std::make_shared<Promise>(fake, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}
	
		template<typename REJLAM>
		std::shared_ptr<Promise> _catch(REJLAM rejecter) {
			if (_state == NULL || _state == nullptr) {
				throw Promise_Error("Promise.catch(): state is null");
			}

			std::shared_ptr<Promise> continuation = nullptr;
			std::shared_ptr<ILambda> lam = nullptr;
						
			_stateLock.lock();
			if (*_state == Pending) {
				std::shared_ptr<ILambda> fake = nullptr;
				lam = rejected_lambda<REJLAM>(rejecter);
				continuation = std::make_shared<Promise>(fake, lam);
				_Promises.push_back(continuation);
			} else if (*_state == Resolved) {
				std::shared_ptr<ILambda> fake = nullptr;
				continuation = std::make_shared<Promise>(fake, this->_state);
			} else {
				lam = rejected_lambda<REJLAM>(rejecter);
				continuation = std::make_shared<Promise>(lam, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}

		template <typename LAMBDA>
		std::shared_ptr<Promise> finally(LAMBDA handler) {
			if (_state == NULL || _state == nullptr) {
				throw Promise_Error("Promise.finally(): state is null");
			}

			std::shared_ptr<Promise> continuation = nullptr;
			std::shared_ptr<ILambda> lam = noarg_lambda<LAMBDA>(handler);

			_stateLock.lock();

			if (*_state == Pending) {
				std::shared_ptr<ILambda> aux = noarg_lambda<LAMBDA>(handler);
				continuation = std::make_shared<Promise>(lam, aux);
				_Promises.push_back(continuation);
			} else {
				continuation = std::make_shared<Promise>(lam, this->_state);
			}

			_stateLock.unlock();

			return continuation;
		}

		virtual std::shared_ptr<State> get_state(void) {
			return this->_state;
		}

	private:
		std::shared_ptr<State> _state;
		std::shared_ptr<ILambda> _settleHandle;
		std::shared_ptr<ILambda> _resolveHandle;
		std::shared_ptr<ILambda> _rejectHandle;
		Semaphore _semp;
		std::mutex _stateLock;
		std::thread _th;
		std::vector<std::shared_ptr<Promise>> _Promises;

		virtual void _resolve(std::shared_ptr<State> state) {
			_stateLock.lock();
			_state = state;
			_stateLock.unlock();

			_semp.increase();
			for (size_t i = 0; i < _Promises.size(); i++) {
				_Promises[i]->_settle(_state, nullptr);
			}
		}

		virtual void _reject(std::shared_ptr<State> state) {
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

		void _withResolveHandle(std::shared_ptr<State> input) {
			//surround this in a try block
			//so if an exception happens, then the promise is rejected instead.

			//we need 2 seperate functions between this and _withRejectHandle
			//because we need to call two different
			std::shared_ptr<IPromise> parent = _resolveHandle->call(input);
			
			//parent will only be nullptr on chain end
			if(parent != nullptr) {
				std::shared_ptr<State> state = parent->get_state();

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

		void _withRejectHandle(std::shared_ptr<State> input) {
			//surroung this in a try block
			//so if an exception happens, then the promise is rejected instead.
			std::shared_ptr<IPromise> parent = _rejectHandle->call(input);
			
			//parent will only be nullptr on chain end
			if(parent != nullptr) {
				std::shared_ptr<State> state = parent->get_state();

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

		void _settle(std::shared_ptr<State> withValue, std::shared_ptr<State> withReason) {
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
	
	typedef std::shared_ptr<Promise> PROM_TYPE;

	std::shared_ptr<Promise> Reject(const std::exception &e) {
		std::shared_ptr<RejectedState> state = std::make_shared<RejectedState>(e);
		std::shared_ptr<Promise> prom = std::make_shared<Promise>(state);

		return prom;
	}

	template <typename T>
	std::shared_ptr<Promise> Resolve(T value) {
		std::shared_ptr<ResolvedState<T>> state = std::make_shared<ResolvedState<T>>(value);
		std::shared_ptr<Promise> prom = std::make_shared<Promise>(state);

		return prom;
	}
	
	//await - suspend execution until the given promise is settled.
	//If promise failed, the reject reason is thrown.
	template <typename T>
	T* await(IPROM_TYPE prom) {
		prom->Join();
		std::shared_ptr<State> s = prom->get_state();
		
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
	std::shared_ptr<Promise> all(std::vector<PROM_TYPE> &promises) {

		std::shared_ptr<Promise> continuation = nullptr;

		std::shared_ptr<ILambda> lam = settlement_lambda([&promises](Settlement settle) {
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

		continuation = std::make_shared<Promise>(lam);

		return continuation;
	}

	template<typename KEYTYPE, typename COMMONTYPE>
	std::shared_ptr<Promise> hash(std::map<KEYTYPE, PROM_TYPE> &promises) {

		std::shared_ptr<Promise> continuation = nullptr;

		std::shared_ptr<ILambda> lam = settlement_lambda([&promises](Settlement settle) {
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

		continuation = std::make_shared<Promise>(lam);

		return continuation;
	}

	typedef std::shared_ptr<Promise> PROMTYPE;
} // namespace Promises

template<typename LAMBDA>
std::shared_ptr<Promises::Promise> promise(LAMBDA handle) {
	std::shared_ptr<Promises::ILambda> l = Promises::settlement_lambda<LAMBDA>(handle);
	std::shared_ptr<Promises::Promise> prom = std::make_shared<Promises::Promise>(l);

	return prom;
}

#endif // !PROMISE_H