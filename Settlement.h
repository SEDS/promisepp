#ifndef SETTLEMENT_H
#define SETTLEMENT_H

#include "IPromise.h"
#include "Lambda.h"
#include "State.h"

namespace Promises {
	class Settlement {
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

		~Settlement(void) {
			//does nothing
		}

		template <typename T>
		void resolve(T v) {
			T *value = memory_pool->allocate<T>();
			*value = v;
			State* state = memory_pool->allocate<ResolvedState<T>, T*>(value);
			_prom->_resolve(state);
		}

		void reject(const std::exception &e) {
			State* state = memory_pool->allocate<RejectedState, const std::exception&>(e);
			_prom->_reject(state);
		}
		
		void reject(const std::string &msg) {
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
		{
			//do nothing
		}

		virtual void call(IPromise *prom)
		{
			Settlement sett(prom);
			_lam(sett);
		}

	private:
		LAMBDA _lam;

		virtual IPromise* call(State* stat)
		{
			return nullptr;
		}
	};
	
	template<typename LAMBDA>
	SettlementLambda<LAMBDA>* create_settlement_lambda(LAMBDA handler) {
		SettlementLambda<LAMBDA>* lam = nullptr;
		lam = memory_pool->allocate<SettlementLambda<LAMBDA>>(handler);
		return lam;
	}
}

#endif // !SETTLEMENT_H