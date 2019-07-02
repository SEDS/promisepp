#include <stdexcept>
#include <string>

#ifndef PROMISE_ERR_H
#define PROMISE_ERR_H

namespace Promises {

    //Promise_Error - used to capture exceptions
    //defined our own exception because exceptions in std do not have copy constructors
    class Promise_Error : public std::exception {
        public:
            explicit Promise_Error(const std::string &msg)
                :_msg(msg)
            { }

            explicit Promise_Error(const char *msg)
                :_msg(msg)
            { }

            Promise_Error(const std::exception &e) 
                :_msg(e.what())
            { }

            Promise_Error(const Promise_Error &err)
                :_msg(err._msg)
            { }

            virtual const char* what() const noexcept {
                return _msg.c_str();
            }

            virtual ~Promise_Error(void)
            { }
			
			Promise_Error& operator = (const std::exception &err) {
				if (this == &err) {
					return (*this);
				}
				
				_msg = err.what();
                return (*this);
			}

            bool operator == (const Promise_Error &err) {
                return (_msg.compare(err._msg) == 0);
            }

            bool operator != (const Promise_Error &err) {
                return (_msg.compare(err._msg) != 0);
            }

        private:
            std::string _msg;
    };

}

#endif // !PROMISE_ERR_H