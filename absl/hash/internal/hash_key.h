// Copyright 2026 The Abseil Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ABSL_HASH_INTERNAL_HASH_KEY_H_
#define ABSL_HASH_INTERNAL_HASH_KEY_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "absl/base/casts.h"
#include "absl/base/config.h"
#include "absl/hash/hash.h"
#include "absl/meta/type_traits.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace hash_internal {

using HashForKeyFn = size_t (*)(const void* hasher, size_t seed);

// Returns true if `key` is passed by value inside the `const void*` argument of
// `HashForKeyFn`, which avoids spilling it to the stack at the call site.
template <class T>
constexpr bool PassKeyByValueInVoidPtr() {
  return FitsIn64BitsPowerOf2<T>::value && sizeof(T) <= sizeof(uintptr_t) &&
         std::is_trivially_copyable_v<T>;
}

template <class T>
const void* KeyToVoidPtr(const T& key) {
  if constexpr (PassKeyByValueInVoidPtr<T>()) {
    return reinterpret_cast<const void*>(
        uintptr_t{absl::bit_cast<UintOfSizeT<sizeof(T)>>(key)});
  } else {
    return &key;
  }
}

// Type erased function to apply `CanonicalHasher` to the key encoded by
// `KeyToVoidPtr`.
template <class CanonicalHasher>
size_t TypeErasedApplyAbslHashToKeyFn(const void* key, size_t seed) {
  using T = typename CanonicalHasher::KeyType;
  if constexpr (PassKeyByValueInVoidPtr<T>()) {
    const auto val =
        static_cast<UintOfSizeT<sizeof(T)>>(reinterpret_cast<uintptr_t>(key));
    return HashWithSeed().hash(CanonicalHasher(), absl::bit_cast<T>(val), seed);
  } else {
    return HashWithSeed().hash(CanonicalHasher(), *static_cast<const T*>(key),
                               seed);
  }
}

// No arguments hash function for a specific key that uses
// `CanonicalHasher<Key>`, so that `get_hash_fn()` is shared across all key
// types with the same canonical hasher.
template <class Key>
struct CanonicalHashKey {
  using NonConstK = absl::remove_cvref_t<Key>;
  using Canonical = decltype(CanonicalHasher<NonConstK>());

  explicit CanonicalHashKey(const NonConstK& k) : key(k) {}

  const void* data() const { return KeyToVoidPtr(key); }
  static constexpr HashForKeyFn get_hash_fn() {
    return &TypeErasedApplyAbslHashToKeyFn<Canonical>;
  }

  // Small keys are stored by value so that inlined caller keys stay in
  // registers instead of becoming address-taken via `data()`.
  type_traits_internal::AddConstRefIfExpensiveToCopy<Key> key;
};

}  // namespace hash_internal
ABSL_NAMESPACE_END
}  // namespace absl

#endif  // ABSL_HASH_INTERNAL_HASH_KEY_H_
