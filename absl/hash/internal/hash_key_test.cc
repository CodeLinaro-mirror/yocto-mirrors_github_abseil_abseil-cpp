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

#include "absl/hash/internal/hash_key.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "gtest/gtest.h"
#include "absl/base/config.h"
#include "absl/hash/hash.h"
#include "absl/strings/string_view.h"

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace hash_internal {
namespace {

TEST(CanonicalHashKeyTest, UniquelyRepresentedTypesShareHashFn) {
  enum class MyEnum : uint32_t { kOne = 1, kTwo = 2 };
  int32_t a = -42;
  uint32_t b = 2;
  MyEnum c = MyEnum::kTwo;
  size_t seed = 100;

  using HashKeyInt32 = CanonicalHashKey<int32_t>;
  using HashKeyUint32 = CanonicalHashKey<uint32_t>;
  using HashKeyEnum = CanonicalHashKey<MyEnum>;

  EXPECT_EQ(HashKeyInt32::get_hash_fn(), HashKeyUint32::get_hash_fn());
  EXPECT_EQ(HashKeyInt32::get_hash_fn(), HashKeyEnum::get_hash_fn());
  EXPECT_NE(HashKeyInt32::get_hash_fn(),
            CanonicalHashKey<int8_t>::get_hash_fn());

  HashKeyInt32 hk_int32{a};
  HashKeyUint32 hk_uint32{b};
  HashKeyEnum hk_enum{c};

  EXPECT_EQ(HashKeyInt32::get_hash_fn()(hk_int32.data(), seed),
            HashWithSeed().hash(absl::Hash<int32_t>{}, a, seed));
  EXPECT_EQ(HashKeyUint32::get_hash_fn()(hk_uint32.data(), seed),
            HashWithSeed().hash(absl::Hash<uint32_t>{}, b, seed));
  EXPECT_EQ(HashKeyEnum::get_hash_fn()(hk_enum.data(), seed),
            HashWithSeed().hash(absl::Hash<MyEnum>{}, c, seed));
}

TEST(CanonicalHashKeyTest, DataPointerForSmallAndLargeKeys) {
  // Small keys are encoded by value.
  int32_t a = 42;
  CanonicalHashKey<int32_t> hk_int32{a};
  EXPECT_EQ(reinterpret_cast<uintptr_t>(hk_int32.data()), 42u);

  // Cheap to copy keys are stored by value.
  absl::string_view sv = "hello";
  CanonicalHashKey<absl::string_view> hk_sv{sv};
  EXPECT_EQ(hk_sv.data(), &hk_sv.key);
  EXPECT_NE(hk_sv.data(), &sv);

  // Expensive to copy keys are stored by reference.
  std::string s = "hello";
  CanonicalHashKey<std::string> hk_str{s};
  EXPECT_EQ(hk_str.data(), &s);
}

}  // namespace
}  // namespace hash_internal
ABSL_NAMESPACE_END
}  // namespace absl
