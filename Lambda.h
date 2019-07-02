#include "IPromise.h"
#include "State.h"

#ifndef LAMBDA_H
#define LAMBDA_H

namespace Promises {
	//lambda traits was modified and influenced from
    //https://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda
    template<typename LAMBDA>
    struct lambda_traits : public lambda_traits<decltype(&LAMBDA::operator())>
    {};

    template<typename OBJ, typename RET, typename ARG>
    struct lambda_traits <RET(OBJ::*)(ARG) const> {
        typedef RET result_type;
        typedef ARG arg_type;
    };

    //enable if modified from https://en.cppreference.com/w/cpp/types/enable_if
    //we create our own version because std::enable_if doesn't set type on B = false.
    template<bool B, class T = void>
    struct enable_if { typedef void type; };
    
    template<class T>
    struct enable_if<true, T> { typedef T type; };

    //if the lambda's return type is not void,
    //then type is LAMBDA, otherwise type is void.
    template<typename LAMBDA>
    struct lambda_if_not_void {
        typedef typename lambda_traits<LAMBDA>::result_type result_type;
        typedef typename enable_if<!std::is_same<result_type, void>::value, LAMBDA>::type type;
    };
	
	template<typename Continue>
    struct Chain {

        template<typename LAMBDA, typename T>
        std::shared_ptr<IPromise> chain (LAMBDA lam, T &value) {
			std::shared_ptr<IPromise> prom = lam(value);
            return prom;
        }

		template<typename LAMBDA>
		std::shared_ptr<IPromise> chain (LAMBDA lam) {
			std::shared_ptr<IPromise> prom = lam();
            return prom;
        }
    };

    template<>
    struct Chain <void> {

        template<typename LAMBDA, typename T>
        std::shared_ptr<IPromise> chain (LAMBDA lam, T &value) {
			lam(value);
            return nullptr;
        }

		template<typename LAMBDA>
		std::shared_ptr<IPromise> chain (LAMBDA lam) {
			lam();
            return nullptr;
        }
    };

	class ILambda {
	public:
		virtual ~ILambda(void) {}
		virtual void call(IPromise* prom) = 0;
		virtual std::shared_ptr<IPromise> call(std::shared_ptr<State> stat) = 0;
	};

	template<typename LAMBDA>
	class RejectedLambda : public ILambda {
	public:
		RejectedLambda(LAMBDA l)
			: _lam(l)
		{ }

		RejectedLambda(const RejectedLambda<LAMBDA> &other)
			:_lam(other._lam)
		{ }

		~RejectedLambda(void) { }

		RejectedLambda<LAMBDA>& operator = (const RejectedLambda<LAMBDA> &other) {
			this->_lam = other._lam;
			return (*this);
		}

		virtual std::shared_ptr<IPromise> call(std::shared_ptr<State> stat) {
			if (stat == NULL | stat == nullptr) {
				throw std::logic_error("RejectedLambda.call(): state is null");
			}
			
			typedef typename lambda_if_not_void<LAMBDA>::type chain_type;
			Chain<chain_type> chainer;
			const std::exception& reason = stat->get_reason();
			std::shared_ptr<IPromise> p = chainer.template chain<LAMBDA, const std::exception>(_lam, reason);

			return p;
		}

	private:
		LAMBDA _lam;

		virtual void call(IPromise* prom) { }
	};

	template <typename LAMBDA>
	class ResolvedLambda : public ILambda {
	public:
		ResolvedLambda(LAMBDA l)
			: _lam(l)
		{ }

		ResolvedLambda(const ResolvedLambda<LAMBDA> &other)
			:_lam(other._lam)
		{ }

		~ResolvedLambda(void) { }

		ResolvedLambda<LAMBDA>& operator = (const ResolvedLambda<LAMBDA> &other) {
			this->_lam = other._lam;
			return (*this);
		}

		virtual std::shared_ptr<IPromise> call(std::shared_ptr<State> stat) {
			if (stat == NULL | stat == nullptr) {
				throw std::logic_error("ResolvedLambda.call(): state is null");
			}
			
			typedef typename lambda_if_not_void<LAMBDA>::type chain_type;
			typedef typename lambda_traits<LAMBDA>::arg_type arg_type;
			Chain<chain_type> chainer;
			arg_type *value = (arg_type *)stat->get_value();
			std::shared_ptr<IPromise> p = chainer.template chain<LAMBDA, arg_type>(_lam, *value);

			return p;
		}

	private:
		LAMBDA _lam;

		virtual void call(IPromise* prom) { }
	};
}

#endif // !LAMBDA_H