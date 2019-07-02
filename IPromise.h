#ifndef IPROMISE_H
#define IPROMISE_H

#include "State.h"
#include <memory>

namespace Promises {
	
    class IPromise {

	public:
		virtual ~IPromise(void) {}
		virtual std::shared_ptr<State> get_state(void) = 0;

	protected:
		virtual void _resolve(std::shared_ptr<State> state) = 0;
		virtual void _reject(std::shared_ptr<State> state) = 0;
		virtual void Join(void) = 0;

		friend class Settlement;

		template <typename T>
		friend T* await(std::shared_ptr<IPromise>);
	};
	
	typedef std::shared_ptr<IPromise> IPROM_TYPE;
}

#endif // !IPROMISE_H