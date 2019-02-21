#include "Promise_Error.h"
#include <iostream>

#ifndef STATE_H
#define STATE_H

namespace Promises {
    enum Status
	{
		Pending,
		Resolved,
		Rejected
	};

    static std::logic_error noerr("NOERR");

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

		bool operator == (const Status stat) {
			return this->_status == stat;
		}

		bool operator != (const Status stat) {
			return this->_status != stat;
		}

        bool operator == (const State &state) {
            return this->operator==(state._status);
        }

        bool operator != (const State &state) {
            return this->operator!=(state._status);
        }

		virtual void* getValue(void) = 0;
		virtual std::exception& getReason(void) = 0;

	private:
		Status _status;
	};

    template <typename T>
	class ResolvedState : public State
	{
	public:
		ResolvedState(T *v)
			: State(Resolved),
			_value(v)
		{
			//do nothing
		}

		ResolvedState(const ResolvedState<T> &state)
			: State(state),
			_value(new T(*state._value))
		{
			//do nothing
		}

		virtual ~ResolvedState(void)
		{
			delete _value;
		}

		virtual void* getValue(void) {
			return _value;
		}
		virtual std::exception& getReason(void) {
			return noerr;
		}

        bool operator == (const ResolvedState<T> &state) {
            bool ret = (*_value == *state._value);
            return ret;
        }

        bool operator != (const ResolvedState<T> &state) {
            bool ret = (*_value != *state._value);
            return ret;
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

        virtual void* getValue(void) {
			return nullptr;
		}

		virtual std::exception& getReason(void) {
			return noerr;
		}

        bool operator == (const ResolvedState<void> &state) {
            bool ret = (State::operator == (state));
            return ret;
        }

        bool operator != (const ResolvedState<void> &state) {
            bool ret = (State::operator != (state));
            return ret;
        }
	};

	class RejectedState : public State
	{
	public:
		RejectedState(void)
            :_reason(noerr)
		{ }

		RejectedState(const std::exception &e)
			:State(Rejected),
            _reason(e.what())
		{ }

		RejectedState(const RejectedState &state)
			:State(state),
			_reason(state._reason)
		{ }

		virtual ~RejectedState(void)
		{ }

		virtual void* getValue(void) {
			return nullptr;
		}

		virtual std::exception& getReason(void) {
			return _reason;
		}

        bool operator == (const RejectedState &state) {
            bool ret = (_reason == state._reason);
            return ret;
        }

        bool operator != (const RejectedState &state) {
            bool ret = (_reason != state._reason);
            return ret;
        }

	private:
		Promise_Error _reason;
	};

}

#endif // !STATE_H