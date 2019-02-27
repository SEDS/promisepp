#ifndef IPROMISE_H
#define IPROMISE_H

#include "State.h"

namespace Promises {
    class IPromise
	{

	public:
		virtual ~IPromise(void) {}
		virtual State* get_state(void) = 0;

	protected:
		virtual void _resolve(State* state) = 0;
		virtual void _reject(State* state) = 0;
		virtual void Join(void) = 0;

		friend class Settlement;
		friend class Pool;

		template <typename T>
		friend T* await(IPromise*);
	};
}

#endif // !IPROMISE_H