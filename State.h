#include <stdexcept>

#ifndef STATE_H
#define STATE_H

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

    template <typename T>
	class ResolvedState : public State
	{
	public:
		ResolvedState(void)
			: _value(nullptr)
		{
			//do nothing
		}

		ResolvedState(T *v)
			: State(Resolved),
			_value(v)
		{
			//do nothing
		}

		ResolvedState(const ResolvedState<T> &state)
			: State(state),
			_value(state._value)
		{
			//do nothing
		}

		virtual ~ResolvedState(void)
		{
			delete _value;
		}

		virtual void *getValue(void) {
			return _value;
		}
		virtual std::exception getReason(void) {
			return std::logic_error("NO ERR");
		}

	private:
		T *_value;
	};

    template <>
    class ResolvedState <void> : public State
	{
	public:
		ResolvedState(void)
			:State(Resolved)
		{
			//do nothing
		}

		ResolvedState(const ResolvedState<void> &state)
			: State(state)
		{
			//do nothing
		}

		virtual ~ResolvedState(void) {
            //do nothing
		}

	private:
		virtual void *getValue(void) {
			return nullptr;
		}
		virtual std::exception getReason(void) {
			return std::logic_error("NO ERR");
		}
	};

	class RejectedState : public State
	{
	public:
		RejectedState(void)
			: _reason(std::logic_error("NO ERR"))
		{
			//do nothing
		}

		RejectedState(std::exception e)
			: State(Rejected),
			_reason(e)
		{
			//do nothing
		}

		RejectedState(const RejectedState &state)
			: State(state),
			_reason(state._reason)
		{
			//do nothing
		}

		virtual ~RejectedState(void)
		{
			//do nothing
		}

		virtual void *getValue(void)
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

}

#endif // !STATE_H