#ifndef CLIB_MAP_HASHF_HPP
#define CLIB_MAP_HASHF_HPP

#include <string>
#include <cstdint>
#include <xxhash.h>
#include <type_traits>

#define HASH_SEED 3266489917U

namespace clib {
	template<class Type>
	class Hashf {
	public:
		// STRING ....................
		template<bool TString = std::is_same<Type, std::string>::value,
			     typename std::enable_if<TString>::type* = nullptr>
		uint32_t operator() (const Type& v) noexcept {
			return XXH32((void*)v.c_str(), v.length(), HASH_SEED);
		}

		// OTHER ......................
		template<bool TString = std::is_same<Type, std::string>::value,
				 typename std::enable_if<!TString>::type* = nullptr>
			uint32_t operator() (const Type& v) noexcept {
			return XXH32((void*)&v, sizeof(Type), HASH_SEED);
		}
	};
}

#endif // !CLIB_MAP_HASHF_HPP
