#include "clib/map/segment.h"

using namespace clib;

Segment::Segment() noexcept
	: timestamp{ 0U },
	  mutex{} {}

void Segment::lock() noexcept {
	this->mutex.lock();
}

void Segment::unlock() noexcept {
	this->mutex.unlock();
}

void Segment::addTimestamp() noexcept {
	this->timestamp++;
}

uint32_t Segment::getTimestamp() noexcept {
	return this->timestamp.load(std::memory_order_acquire);
}