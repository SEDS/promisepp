#define BOOST_TEST_MODULE PROMISEPP
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "../Promise.h"

BOOST_AUTO_TEST_SUITE(PROMISE_SUITE)

BOOST_AUTO_TEST_CASE(Promise_Settlement_Constructor_Test) {
    Promises::Settlement settlement(nullptr);

    try {   
        settlement.resolve<int>(10);
    } catch (const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "Settlement.resolve(): internal promise is null") == 0);
    }
}

BOOST_AUTO_TEST_CASE(Settlement_Copy_Constructor_Test) {
    Promises::Settlement settlement(nullptr);
    Promises::Settlement copy_settlement(settlement);

    try {
        copy_settlement.resolve<int>(10);
    } catch (const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "Settlement.resolve(): internal promise is null") == 0);
    }
}

BOOST_AUTO_TEST_CASE(SettlmentLambda_Test) {
    Promises::ILAM_TYPE lam = Promises::settlement_lambda([](Promises::Settlement settle) {
        //success when control enters this scope
        BOOST_CHECK(true);
    });

    Promises::IPromise* prom = nullptr;
    lam->call(prom);
}

BOOST_AUTO_TEST_CASE(Promise_Default_Constructor_Test) {
    Promises::Promise prom;
    BOOST_CHECK(prom.get_state() == nullptr);

    try {
        prom.then([](int num) {}, [](const std::exception &ex) {});
    } catch (const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "Promise.then(): state is null") == 0);
    }

    try {
        prom.then([](int num) {});
    } catch (const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "Promise.then(): state is null") == 0);
    }

    try {
        prom._catch([](const std::exception &ex) {});
    } catch (const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "Promise.catch(): state is null") == 0);
    }

    try {
        prom.finally([]() {});
    } catch (const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "Promise.finally(): state is null") == 0);
    }
}

BOOST_AUTO_TEST_CASE(Settlement_Resolve_Test) {
    Promises::Promise prom;
    Promises::Settlement settlement(&prom);

    settlement.resolve<int>(10);
    
    Promises::STATE_TYPE state = prom.get_state();
    BOOST_CHECK((*(int*)state->get_value()) == 10);
}

BOOST_AUTO_TEST_CASE(Settlement_Reject_Test) {
    Promises::Promise prom;
    Promises::Settlement settlement(&prom);

    settlement.reject(std::logic_error("test"));
    
    Promises::STATE_TYPE state = prom.get_state();
    BOOST_CHECK(strcmp(state->get_reason().what(), "test") == 0);
}

BOOST_AUTO_TEST_CASE(Promise_SettlementLambda_Constructor_Test) {
    Promises::ILAM_TYPE lam = Promises::settlement_lambda([](Promises::Settlement settle) {
        //success if control enters this scope
        BOOST_CHECK(true);
    });

    Promises::PROM_TYPE prom = std::make_shared<Promises::Promise>(lam);
    
    //promise destructor joins on thread
}

BOOST_AUTO_TEST_CASE(Await_Test) {
    Promises::ILAM_TYPE lam1 = Promises::settlement_lambda([](Promises::Settlement settle) {
        settle.resolve<int>(10);
    });

    Promises::PROM_TYPE prom1 = std::make_shared<Promises::Promise>(lam1);
    int* v = Promises::await<int>(prom1);
    BOOST_CHECK(*v == 10);

    Promises::ILAM_TYPE lam2 = Promises::settlement_lambda([](Promises::Settlement settle) {
        settle.reject(std::logic_error("nyalia"));
    });

    Promises::PROM_TYPE prom2 = std::make_shared<Promises::Promise>(lam2);
    try {
        Promises::await<int>(prom2);
    } catch (const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "nyalia") == 0);
    }    
}

BOOST_AUTO_TEST_CASE(Promise_Factory_Method_Test) {
    Promises::PROM_TYPE prom = promise([](Promises::Settlement settle) {
        settle.resolve<int>(10);
    });

    int* v = Promises::await<int>(prom);
    BOOST_CHECK(*v == 10);
}

BOOST_AUTO_TEST_CASE(Double_Lambda_Then_Test) {
    Promises::PROM_TYPE prom1 = promise([](Promises::Settlement settle) {
        settle.resolve<int>(10);
    });

    //tests the resolve handle
    prom1->then([](int value) {
        BOOST_CHECK(value == 10);
    }, [](const std::exception &ex){});
	
    Promises::PROM_TYPE prom2 = promise([](Promises::Settlement settle) {
        settle.reject(std::logic_error("test"));
    });

    //tests the reject handle
    prom2->then([](int value) { }, [](const std::exception &ex){
        BOOST_CHECK(strcmp(ex.what(), "test") == 0);
    });
}

BOOST_AUTO_TEST_CASE(Single_Lambda_Then_Test) {
    Promises::PROM_TYPE prom = promise([](Promises::Settlement settle) {
        settle.resolve<int>(10);
    });

    prom->then([](int value) {
        BOOST_CHECK(value == 10);
    });
}

BOOST_AUTO_TEST_CASE(Lambda_Catch_Test) {
    Promises::PROM_TYPE prom = promise([](Promises::Settlement settle) {
        settle.reject(Promises::Promise_Error("nyalia"));
    });

    prom->_catch([](const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "nyalia") == 0);
    });
}

BOOST_AUTO_TEST_CASE(Bubble_Resolve_Test) {
    Promises::PROM_TYPE prom = promise([](Promises::Settlement settle) {
        settle.resolve<int>(10);
    });

    //tests the resolve handle when value must be bubbled downstream
    prom->_catch([](const std::exception &ex) { })
    ->_catch([](const std::exception &ex) { })
    ->_catch([](const std::exception &ex) { })
    ->then([](int value) {
        BOOST_CHECK(value == 10);
    });
}

BOOST_AUTO_TEST_CASE(Bubble_Reject_Test) {
    Promises::PROM_TYPE prom = promise([](Promises::Settlement settle) {
        settle.reject(Promises::Promise_Error("nyalia"));
    });

    prom->then([](int num){})
    ->then([](int num){})
    ->then([](int num){})
    ->_catch([](const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "nyalia") == 0);
    });
}

BOOST_AUTO_TEST_CASE(PreResolved_Test) {
    auto prom = Promises::Resolve<int>(10);

    prom->then([](int num) {
        BOOST_CHECK(num == 10);
    });

    auto v = Promises::await<int>(prom);
    BOOST_CHECK(*v == 10);
}

BOOST_AUTO_TEST_CASE(PreRejected_Test) {
    auto prom = Promises::Reject(Promises::Promise_Error("IUPUI"));

    prom->_catch([](const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "IUPUI") == 0);
    });

    try {
        Promises::await<int>(prom);
    } catch (const std::exception &ex) {
        BOOST_CHECK(strcmp(ex.what(), "IUPUI") == 0);
    }
}

BOOST_AUTO_TEST_CASE(Finally_Test) {
    Promises::PROM_TYPE prom = promise([](Promises::Settlement settle) {
        settle.reject(Promises::Promise_Error("nyalia"));
    });

    prom->then([](int num){})
    ->then([](int num){})
    ->then([](int num){})
    ->finally([](){ } )
    ->_catch([](const std::exception &ex) { 
        BOOST_CHECK(strcmp(ex.what(), "nyalia") == 0);
    });

    prom->then([](int num){})
    ->then([](int num){})
    ->then([](int num){})
    ->_catch([](const std::exception &ex) { 
        return Promises::Resolve<int>(10);
    })
    ->finally([](){
        BOOST_CHECK(true);
    });
}

BOOST_AUTO_TEST_CASE(Promise_All_Test) {
	std::vector<Promises::PROM_TYPE> promises;
	promises.push_back(Promises::Resolve<int>(10));
	promises.push_back(promise([](Promises::Settlement settle) {
		settle.resolve<int>(20);
	}));
	promises.push_back(promise([](Promises::Settlement settle) {
		settle.resolve<int>(30);
	}));
	
	Promises::PROM_TYPE prom1 = Promises::all<int>(promises);
	
	auto values = Promises::await<std::vector<int>>(prom1);
	
	BOOST_CHECK((*values)[0] == 10);
	BOOST_CHECK((*values)[1] == 20);
	BOOST_CHECK((*values)[2] == 30);
	
	//make sure the 1st rejected reason is used
	promises.push_back(Promises::Reject(Promises::Promise_Error("IUPUI")));
	promises.push_back(Promises::Reject(Promises::Promise_Error("nyalia")));
	
	Promises::PROM_TYPE prom2 = Promises::all<int>(promises);
	
	try {
		Promises::await<std::vector<int>>(prom2);
	} catch (const std::exception &ex) {
		BOOST_CHECK(strcmp(ex.what(), "IUPUI") == 0);
	}
}

BOOST_AUTO_TEST_CASE(Promise_Hash_Test) {
	typedef std::pair<std::string, Promises::PROM_TYPE> prom_pair;
	
	std::map<std::string, Promises::PROM_TYPE> promises;
	promises.insert(prom_pair("promise1", Promises::Resolve<int>(10)));
	promises.insert(prom_pair("promise2", promise([](Promises::Settlement settle) {
		settle.resolve<int>(20);
	})));
	promises.insert(prom_pair("promise3", promise([](Promises::Settlement settle) {
		settle.resolve<int>(30);
	})));
	
	Promises::PROM_TYPE prom1 = Promises::hash<std::string, int>(promises);
	
	auto values = Promises::await<std::map<std::string, int>>(prom1);

	BOOST_CHECK((*values)["promise1"] == 10);
	BOOST_CHECK((*values)["promise2"] == 20);
	BOOST_CHECK((*values)["promise3"] == 30);
	
	//make sure the 1st rejected reason is used
	promises.insert(prom_pair("promise4", Promises::Reject(Promises::Promise_Error("IUPUI"))));
	promises.insert(prom_pair("promise5", Promises::Reject(Promises::Promise_Error("nyalia"))));
	Promises::PROM_TYPE prom2 = Promises::hash<std::string, int>(promises);
	
	try {
		Promises::await<std::map<std::string, Promises::IPROM_TYPE>>(prom2);
	} catch (const std::exception &ex) {
		BOOST_CHECK(strcmp(ex.what(), "IUPUI") == 0);
	}
}

BOOST_AUTO_TEST_SUITE_END()