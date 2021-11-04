#include "clib/util.hpp"
#include "clib/list.hpp"
#include "clib/signal.h"
#include "clib/spinlock.hpp"

using namespace clib;

SpinLock<true> spin{};
List<Signal::Info> subscribers[6];

#if defined(__GNUC__) || defined(__clang__)
void handler(int signalNumber, siginfo_t* siginfo, void* context) {
	List<Info>::Node* node;
	switch (signalNumber) {
	case SIGABRT:
		node = subscribers[Type::SIG_ARBT].getHead();
		break;

	case SIGFPE:
		node = subscribers[Type::SIG_FPE].getHead();
		break;

	case SIGILL:
		node = subscribers[Type::SIG_ILL].getHead();
		break;

	case SIGINT:
		node = subscribers[Type::SIG_INT].getHead();
		break;

	case SIGSEGV:
		node = subscribers[Type::SIG_SEGV].getHead();
		break;

	case SIGTERM:
		node = subscribers[Type::SIG_TERM].getHead();
		break;

	default:
		return;
	}

	while (node) {
		node->get().callback(signalNumber);
		node = node->getNext();
	}
	exit(signalNumber);
}

Error setHandler(int signal, void (*handler)(int, siginfo_t*, void*)) {
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	sigemptyset(&action.sa_mask);
	action.sa_sigaction = handler;
	action.sa_flags = SA_SIGINFO;

	if (sigaction(signalNumber, &action, nullptr) < 0) {
		return Error{ GET_FUNC_NAME(), "Setting handler for the signal is failed" };
	}
	return Error{};
}
#elif defined(_MSC_VER)
void handler(int signalNumber) {
	List<Signal::Info>::Node* node;
	switch (signalNumber) {
	case SIGABRT:
		node = subscribers[Type::SIG_ARBT].getHead();
		break;

	case SIGFPE:
		node = subscribers[Type::SIG_FPE].getHead();
		break;

	case SIGILL:
		node = subscribers[Type::SIG_ILL].getHead();
		break;

	case SIGINT:
		node = subscribers[Type::SIG_INT].getHead();
		break;

	case SIGSEGV:
		node = subscribers[Type::SIG_SEGV].getHead();
		break;

	case SIGTERM:
		node = subscribers[Type::SIG_TERM].getHead();
		break;

	default:
		return;
	}

	while (node) {
		node->get().callback(signalNumber);
		node = node->getNext();
	}
	exit(signalNumber);
}

Error setHandler(int signalNumber, void (*handler)(int)) {
	if (std::signal(signalNumber, handler) == SIG_ERR) {
		return Error{ GET_FUNC_NAME(), "Setting handler for the signal is failed" };
	}
	return Error{};
}
#endif

//==============================
// Class "Signal" implementation
//==============================

Signal Signal::Instance;

void Signal::init() {
	auto err = setHandler(SIGABRT, handler);
	if (!err.nil()) {
		throw err.toString();
	}

	err = setHandler(SIGFPE, handler);
	if (!err.nil()) {
		throw err.toString();
	}

	err = setHandler(SIGILL, handler);
	if (!err.nil()) {
		throw err.toString();
	}

	err = setHandler(SIGINT, handler);
	if (!err.nil()) {
		throw err.toString();
	}

	err = setHandler(SIGSEGV, handler);
	if (!err.nil()) {
		throw err.toString();
	}

	err = setHandler(SIGTERM, handler);
	if (!err.nil()) {
		throw err.toString();
	}
}

Error Signal::reset(Type type) {
	int signalNumber;
	switch (type) {
	case Type::SIG_ARBT: 
		signalNumber = SIGABRT;
		break;

	case Type::SIG_FPE:
		signalNumber = SIGFPE;
		break;

	case Type::SIG_ILL:
		signalNumber = SIGILL;
		break;

	case Type::SIG_INT:
		signalNumber = SIGINT;
		break;

	case Type::SIG_SEGV:
		signalNumber = SIGSEGV;
		break;

	case Type::SIG_TERM:
		signalNumber = SIGTERM;
		break;

	default:
		return Error{ GET_FUNC_NAME(), "UNKNOWN SIGNAL" };
	}

#if defined(__GNUC__) || defined(__clang__)
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	sigemptyset(&action.sa_mask);
	action.sa_handler = SIG_DFL;
	if (sigaction(signalNumber, &action, nullptr) < 0) {
		return Error{ GET_FUNC_NAME(),  "Resetting handler for the signal is failed" };
	}
	return Error{};
#elif defined(_MSC_VER)
	if (std::signal(signalNumber, SIG_DFL) == SIG_ERR) {
		return Error{ GET_FUNC_NAME(), "Resetting handler for the signal is failed" };
	}
	return Error{};
#endif
}

std::tuple<Error, uint64_t> Signal::subscribe(Type type, Order order, std::function<void(int)>&& callback) noexcept {
	if (type < 0 || type > 6) {
		return { Error{ GET_FUNC_NAME(), "UNKNOWN SIGNAL" }, 0 };
	}
	if (order < 0 || order > 3) {
		return { Error{ GET_FUNC_NAME(), "UNKNOWN ORDER" }, 0 };
	}

	auto id = getTimestamp();
	if (order == Order::HEAD) {
		return { pushHead(type, id, order, std::forward<std::function<void(int)>>(callback)), id };
	}
	if (order == Order::TAIL) {
		return { pushTail(type, id, order, std::forward<std::function<void(int)>>(callback)), id };
	}
	return { pushMiddle(type, id, order, std::forward<std::function<void(int)>>(callback)), id };
}

Error Signal::unsubscribe(Type type, uint64_t id) noexcept {
	if (type < 0 || type > 6) {
		return Error{ GET_FUNC_NAME(), "UNKNOWN SIGNAL" };
	}

	spin.lock();
	auto err = Error{};
	auto node = subscribers[type].getHead();
	while (node) {
		if (node->get().id == id) {
			subscribers[type].erase(node);
			goto done;
		}
		node = node->getNext();
	}
	err = Error{ GET_FUNC_NAME(), "UNKNOWN SUBSCRIBER ID" };

done:
	spin.unlock();
	return err;
}

Error Signal::pushHead(Type type, uint64_t id, Order order, std::function<void(int)>&& callback) noexcept {
	auto err = Error{};
	auto& list = subscribers[type];
	spin.lock();
	if (list.getSize() == 0 || list.getHead()->get().order != Order::HEAD) {
		list.pushFront(id, order, std::forward<std::function<void(int)>>(callback));
	}
	else {
		err = Error{ GET_FUNC_NAME(), "Head position is already taken" };
	}
	spin.unlock();
	return err;
}

Error Signal::pushTail(Type type, uint64_t id, Order order, std::function<void(int)>&& callback) noexcept {
	auto err = Error{};
	auto& list = subscribers[type];
	spin.lock();
	if (list.getSize() == 0 || list.getTail()->get().order != Order::TAIL) {
		list.pushBack(id, order, std::forward<std::function<void(int)>>(callback));
	}
	else {
		err = Error{ GET_FUNC_NAME(), "Tail position is already taken" };
	}
	spin.unlock();
	return err;
}

Error Signal::pushMiddle(Type type, uint64_t id, Order order, std::function<void(int)>&& callback) noexcept {
	spin.lock();
	subscribers[type].insertFront(subscribers[type].getTail(), id, order, std::forward<std::function<void(int)>>(callback));
	spin.unlock();
	return Error{};
}