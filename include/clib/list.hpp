#ifndef CLIB_LIST_HPP
#define CLIB_LIST_HPP

#include <functional>
#include <type_traits>

namespace clib {
	template<typename Type>
	class List {
	public:
		class Node {
		private:
			friend class List;
			using Storage = typename std::aligned_storage<sizeof(Type), alignof(Type)>::type;
			Storage storage{};
			Node* prev{ nullptr };
			Node* next{ nullptr };

		public:
			template<typename... Args>
			Node(Args&&... args) noexcept {
				new (&this->storage) Type(std::forward<Args>(args)...);
			}

			~Node() noexcept {
				reinterpret_cast<Type*>(std::addressof(this->storage))->~Type();
			}

			Node* getPrev() const noexcept {
				return this->prev;
			}

			Node* getNext() const noexcept {
				return this->next;
			}

			Type& get() noexcept {
				return *reinterpret_cast<Type*>(std::addressof(this->storage));
			}
		};

	private:
		uint32_t size;
		Node* head;
		Node* tail;

	public:
		List(const List& other) = delete;
		List& operator=(const List& other) = delete;

		List() noexcept
			: size{ 0 },
			  head{ nullptr },
			  tail{ nullptr }{}

		List(List&& other) noexcept
			: size{ other.size },
			  head{ other.head },
			  tail{ other.tail } {
			other.size = 0;
			other.head = nullptr;
			other.tail = nullptr;
		}

		~List() noexcept {
			clear();
		}

		List& operator=(List&& other) noexcept {
			this->head = other.head;
			this->tail = other.tail;
			other.head = nullptr;
			other.tail = nullptr;
		}

		bool empty() const noexcept {
			return this->head == nullptr;
		}

		uint32_t getSize() const noexcept {
			return this->size;
		}

		Node* getHead() const noexcept {
			return this->head;
		}

		Node* getTail() const noexcept {
			return this->tail;
		}

		template<typename T = Type, typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type* = nullptr>
		T popFront() noexcept {
			auto node = this->head;
			this->head = this->head->next;
			this->size--;

			auto val{ std::move(node->get()) };
			delete node;
			return val;
		}

		template<typename T = Type, typename std::enable_if<std::is_nothrow_copy_constructible<T>::value>::type* = nullptr>
		T popFront() noexcept {
			auto node = this->head;
			this->head = this->head->next;
			this->size--;

			auto val{ node->get() };
			delete node;
			return val;
		}

		template<typename T = Type, typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type* = nullptr>
		T popBack() noexcept {
			auto node = this->tail;
			this->tail = this->tail->next;
			this->size--;

			auto val{ std::move(node->get) };
			delete node;
			return val;
		}

		template<typename T = Type, typename std::enable_if<std::is_nothrow_copy_constructible<T>::value>::type* = nullptr>
		T popBack() noexcept {
			auto node = this->tail;
			this->tail = this->tail->next;
			this->size--;

			auto val{ node->get };
			delete node;
			return val;
		}

		template<typename... Args>
		void pushFront(Args&&... args) noexcept {
			Node* node = new Node(std::forward<Args>(args)...);
			if (this->head) {
				node->next = this->head;
				this->head->prev = node;
			}
			else {
				this->tail = node;
			}
			this->head = node;
			this->size++;
		}

		template<typename... Args>
		void pushBack(Args&&... args) noexcept {
			Node* node = new Node(std::forward<Args>(args)...);
			if (this->tail) {
				node->prev = this->tail;
				this->tail->next = node;
			}
			else {
				this->head = node;
			}
			this->tail = node;
			this->size++;
		}

		template<typename... Args>
		void insertFront(Node* node, Args&&... args) noexcept {
			if (!node) {
				return;
			}
			Node* newNode = new Node(std::forward<Args>(args)...);
			if (node->prev) {
				node->prev->next = newNode;
			}
			newNode->prev = node->prev;
			newNode->next = node;
			node->prev = newNode;
		}

		template<typename... Args>
		void insertBack(Node* node, Args&&... args) noexcept {
			if (!node) {
				return;
			}
			Node* newNode = new Node(std::forward<Args>(args)...);
			if (node->next) {
				node->next->prev = newNode;
			}
			newNode->prev = node;
			newNode->next = node->next;
			node->next = newNode;
		}

		bool contains(const Type& value) noexcept {
			Node* node = this->head;
			while (node) {
				if (node->get() == value) {
					return true;
				}
			}
			return false;
		}

		bool erase(Node* node) noexcept {
			if (!node || !this->head) {
				return false;
			}
			if (node == this->head) {
				this->head = this->head->next;
				if (!this->head) {
					this->tail = nullptr;
				}
			}
			else if (node == this->tail) {
				this->tail = this->tail->prev;
			}
			else {
				node->prev->next = node->next;
				node->next->prev = node->prev;
			}
			this->size--;
			delete node;
			return true;
		}

		void clear() noexcept {
			Node* node = nullptr;
			while (this->head) {
				node = this->head;
				this->head = this->head->next;
				delete node;
			}
			this->size = 0;
			this->tail = nullptr;
		}
	};
}

#endif // !CLIB_LIST_HPP
