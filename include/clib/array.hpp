#ifndef CLIB_ARRAY_HPP
#define CLIB_ARRAY_HPP

#include <tuple>
#include <cstring>

namespace clib {
    template<class Type>
    class Array {
    private:
        Type* buf;
        size_t capacity;

    private:
        void free() noexcept {
            if (this->buf != nullptr) {
                delete[] this->buf;
            }
            this->buf = nullptr;
            this->capacity = 0;
        }

    public:
        Array() noexcept
            : buf{ nullptr },
              capacity{ 0 } {}

        explicit Array(size_t capacity)
            : buf{ new Type[capacity] },
              capacity{ capacity } {}

        // If isCopy = false, "Array" will manage memory as std::unique_ptr
        // If isCopy = true, "Array" will copy memory
        Array(Type* buf, size_t capacity, bool isCopy = false) noexcept
            : buf{ nullptr },
              capacity{ capacity } {
            if (!isCopy) {
                this->buf = buf;
            }
            else {
                memcpy(this->buf, buf, capacity * sizeof(Type));
            }
        }

        // Support for byte array init from string
        template <typename U,
                  typename std::enable_if<std::is_same<U, char>::value &&
                                          std::is_same<Type, uint8_t>::value>::type* = nullptr>
        explicit Array(const U* str)
            : buf{ nullptr },
              capacity{ strlen(str) + 1 } {
            this->buf = new Type[this->capacity];
            memcpy(this->buf, str, this->capacity);
            this->buf[this->capacity - 1] = 0;
        }

        Array(const Array& other)
            : buf{ nullptr },
              capacity{ 0 } {
            if (other.capacity == 0 || other.buf == nullptr) {
                return;
            }
            this->capacity = other.capacity;
            this->buf = new Type[other.capacity];
            memcpy(this->buf, other.buf, other.capacity * sizeof(Type));
        }

        Array(Array<Type>&& other) noexcept
            : buf{ nullptr },
              capacity{ 0 } {
            std::swap(this->buf, other.buf);
            std::swap(this->capacity, other.capacity);
        }

        ~Array() noexcept {
            if (this->buf != nullptr) {
                delete[] this->buf;
            }
        }

        Array& operator=(const Array& other) {
            reset(other.capacity);
            if (other.capacity > 0) {
                memcpy(this->buf, other.buf, other.capacity * sizeof(Type));
            }
            return *this;
        }

        Array& operator=(Array&& other) noexcept {
            std::tie(this->buf, this->capacity) = other.release();
            return *this;
        }

        inline Type& operator[](size_t index) const {
            return this->buf[index];
        }

        // Check array empty
        bool empty() const noexcept {
            return this->buf == nullptr;
        }

        // Return size array
        size_t size() const noexcept {
            return this->capacity;
        }

        // Return pointer array
        Type* get() const noexcept {
            return this->buf;
        }

        // Reset array
        // If isCopy = false, "Array" will manage memory as std::unique_ptr
        // If isCopy = true, "Array" will copy memory
        void reset(size_t capacity = 0, Type* buf = nullptr, bool isCopy = false) {
            free();
            if (capacity == 0) {
                return;
            }
            if (buf == nullptr) {
                buf = new Type[capacity];
            }

            if (!isCopy) {
                this->buf = buf;
            }
            else {
                memcpy(this->buf, buf, capacity * sizeof(Type));
            }
            this->capacity = capacity;
        }

        // Return pointer + length array
        std::tuple<Type*, size_t> release() noexcept {
            Type* buf = nullptr;
            size_t capacity = 0;
            std::swap(this->buf, buf);
            std::swap(this->capacity, capacity);
            return { buf, capacity };
        }

        // Swap array
        void swap(Array<Type>& other) noexcept {
            std::swap(this->buf, other.buf);
            std::swap(this->capacity, other.capacity);
        }
    };
}

#endif // !CLIB_ARRAY_HPP
