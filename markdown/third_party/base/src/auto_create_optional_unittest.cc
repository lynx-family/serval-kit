// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdio>
#include <string>
#include <vector>

#define private public

#include "base/include/auto_create_optional.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace test {

struct DataStruct {
  std::vector<std::string> vec1;
  std::vector<std::string> vec2;
};

TEST(AutoCreateOptional, BoolShortCircuit) {
  auto_create_optional<DataStruct> data;
  if (data.has_value() && !data->vec1.empty()) {
    printf("1");
  }
  if (!data.has_value() || data->vec1.empty()) {
    printf("2");
  }
  EXPECT_FALSE(data.has_value());
}

TEST(AutoCreateOptional, CopyConstruct) {
  auto_create_optional<DataStruct> data;
  auto_create_optional<DataStruct> data_empty(data);
  EXPECT_FALSE(data_empty.has_value());
  EXPECT_FALSE(bool(data_empty));
  EXPECT_EQ(data_empty.get(), nullptr);

  data->vec1.push_back("123");
  data->vec2.push_back("abc");

  auto_create_optional<DataStruct> data2(data);
  EXPECT_TRUE(data2.has_value());
  EXPECT_TRUE(bool(data2));

  EXPECT_EQ(&(data2.data_->vec1), &(data2.get()->vec1));
  EXPECT_EQ(&(data2.data_->vec1), &((*data2).vec1));
  EXPECT_TRUE(data2.data_->vec1.size() == 1);
  EXPECT_TRUE(data2.data_->vec1[0] == "123");
  EXPECT_TRUE(data2.data_->vec2.size() == 1);
  EXPECT_TRUE(data2.data_->vec2[0] == "abc");

  EXPECT_TRUE(data.has_value());
  EXPECT_TRUE(bool(data));
  EXPECT_EQ(&(data.data_->vec1), &(data.get()->vec1));
  EXPECT_EQ(&(data.data_->vec1), &((*data).vec1));
  EXPECT_TRUE(data.data_->vec1.size() == 1);
  EXPECT_TRUE(data.data_->vec1[0] == "123");
  EXPECT_TRUE(data.data_->vec2.size() == 1);
  EXPECT_TRUE(data.data_->vec2[0] == "abc");
}

TEST(AutoCreateOptional, CopyAssign) {
  auto_create_optional<DataStruct> data;
  auto_create_optional<DataStruct> data_empty;
  data_empty = data;
  EXPECT_FALSE(data_empty.has_value());
  EXPECT_FALSE(bool(data_empty));
  EXPECT_EQ(data_empty.get(), nullptr);

  data->vec1.push_back("123");
  data->vec2.push_back("abc");

  auto_create_optional<DataStruct> data2;
  EXPECT_FALSE(data2.has_value());
  EXPECT_FALSE(bool(data2));
  EXPECT_EQ(data2.get(), nullptr);

  data2 = data;
  EXPECT_TRUE(data2.has_value());
  EXPECT_TRUE(bool(data2));

  EXPECT_EQ(&(data2.data_->vec1), &(data2.get()->vec1));
  EXPECT_EQ(&(data2.data_->vec1), &((*data2).vec1));
  EXPECT_TRUE(data2.data_->vec1.size() == 1);
  EXPECT_TRUE(data2.data_->vec1[0] == "123");
  EXPECT_TRUE(data2.data_->vec2.size() == 1);
  EXPECT_TRUE(data2.data_->vec2[0] == "abc");

  EXPECT_TRUE(data.has_value());
  EXPECT_TRUE(bool(data));
  EXPECT_EQ(&(data.data_->vec1), &(data.get()->vec1));
  EXPECT_EQ(&(data.data_->vec1), &((*data).vec1));
  EXPECT_TRUE(data.data_->vec1.size() == 1);
  EXPECT_TRUE(data.data_->vec1[0] == "123");
  EXPECT_TRUE(data.data_->vec2.size() == 1);
  EXPECT_TRUE(data.data_->vec2[0] == "abc");

  data2 = data_empty;
  EXPECT_FALSE(data2.has_value());
  EXPECT_FALSE(bool(data2));
  EXPECT_EQ(data2.get(), nullptr);
}

TEST(AutoCreateOptional, MoveConstruct) {
  auto_create_optional<DataStruct> data;
  auto_create_optional<DataStruct> data_empty(std::move(data));
  EXPECT_FALSE(data_empty.has_value());
  EXPECT_FALSE(bool(data_empty));
  EXPECT_EQ(data_empty.get(), nullptr);

  data->vec1.push_back("123");
  data->vec2.push_back("abc");

  auto_create_optional<DataStruct> data2(std::move(data));
  EXPECT_FALSE(data.has_value());
  EXPECT_FALSE(bool(data));
  EXPECT_EQ(data.get(), nullptr);
  EXPECT_TRUE(data2.has_value());
  EXPECT_TRUE(bool(data2));

  EXPECT_EQ(&(data2.data_->vec1), &(data2.get()->vec1));
  EXPECT_EQ(&(data2.data_->vec1), &((*data2).vec1));
  EXPECT_TRUE(data2.data_->vec1.size() == 1);
  EXPECT_TRUE(data2.data_->vec1[0] == "123");
  EXPECT_TRUE(data2.data_->vec2.size() == 1);
  EXPECT_TRUE(data2.data_->vec2[0] == "abc");
}

TEST(AutoCreateOptional, MoveAssign) {
  auto_create_optional<DataStruct> data;
  auto_create_optional<DataStruct> data_empty;
  data_empty = std::move(data);
  EXPECT_FALSE(data_empty.has_value());
  EXPECT_FALSE(bool(data_empty));
  EXPECT_EQ(data_empty.get(), nullptr);

  data->vec1.push_back("123");
  data->vec2.push_back("abc");

  auto_create_optional<DataStruct> data2;
  EXPECT_FALSE(data2.has_value());
  EXPECT_FALSE(bool(data2));
  EXPECT_EQ(data2.get(), nullptr);

  data2 = std::move(data);
  EXPECT_TRUE(data2.has_value());
  EXPECT_TRUE(bool(data2));

  EXPECT_EQ(&(data2.data_->vec1), &(data2.get()->vec1));
  EXPECT_EQ(&(data2.data_->vec1), &((*data2).vec1));
  EXPECT_TRUE(data2.data_->vec1.size() == 1);
  EXPECT_TRUE(data2.data_->vec1[0] == "123");
  EXPECT_TRUE(data2.data_->vec2.size() == 1);
  EXPECT_TRUE(data2.data_->vec2[0] == "abc");

  EXPECT_FALSE(data.has_value());
  EXPECT_FALSE(bool(data));
  EXPECT_EQ(data.get(), nullptr);

  data2 = std::move(data_empty);
  EXPECT_FALSE(data2.has_value());
  EXPECT_FALSE(bool(data2));
  EXPECT_EQ(data2.get(), nullptr);
}

TEST(AutoCreateOptional, CreateByArrow) {
  auto_create_optional<DataStruct> data;
  EXPECT_FALSE(data.has_value());
  EXPECT_FALSE(bool(data));
  EXPECT_EQ(data.get(), nullptr);

  data->vec1.push_back("123");
  data->vec2.push_back("abc");

  EXPECT_TRUE(data.has_value());
  EXPECT_TRUE(bool(data));

  EXPECT_EQ(&(data.data_->vec1), &(data.get()->vec1));
  EXPECT_EQ(&(data.data_->vec1), &((*data).vec1));
  EXPECT_TRUE(data.data_->vec1.size() == 1);
  EXPECT_TRUE(data.data_->vec1[0] == "123");
  EXPECT_TRUE(data.data_->vec2.size() == 1);
  EXPECT_TRUE(data.data_->vec2[0] == "abc");

  data.reset();
  EXPECT_FALSE(data.has_value());
  EXPECT_FALSE(bool(data));
  EXPECT_EQ(data.get(), nullptr);

  data->vec1.push_back("123");
  data->vec2.push_back("abc");

  EXPECT_TRUE(data.has_value());
  EXPECT_TRUE(bool(data));

  EXPECT_EQ(&(data.data_->vec1), &(data.get()->vec1));
  EXPECT_EQ(&(data.data_->vec1), &((*data).vec1));
  EXPECT_TRUE(data.data_->vec1.size() == 1);
  EXPECT_TRUE(data.data_->vec1[0] == "123");
  EXPECT_TRUE(data.data_->vec2.size() == 1);
  EXPECT_TRUE(data.data_->vec2[0] == "abc");
}

TEST(AutoCreateOptional, CreateByAsterisk) {
  auto_create_optional<std::vector<std::string>> data;
  EXPECT_FALSE(data.has_value());
  EXPECT_FALSE(bool(data));
  EXPECT_EQ(data.get(), nullptr);

  (*data).push_back("123");
  (*data).push_back("abc");

  EXPECT_TRUE(data.has_value());
  EXPECT_TRUE(bool(data));

  EXPECT_EQ(data.data_.get(), &(*data));
  EXPECT_TRUE(data.data_->size() == 2);
  EXPECT_TRUE((*data.data_)[0] == "123");
  EXPECT_TRUE((*data.data_)[1] == "abc");

  data.reset();
  EXPECT_FALSE(data.has_value());
  EXPECT_FALSE(bool(data));
  EXPECT_EQ(data.get(), nullptr);

  (*data).push_back("123");
  (*data).push_back("abc");

  EXPECT_TRUE(data.has_value());
  EXPECT_TRUE(bool(data));

  EXPECT_EQ(data.data_.get(), &(*data));
  EXPECT_TRUE(data.data_->size() == 2);
  EXPECT_TRUE((*data.data_)[0] == "123");
  EXPECT_TRUE((*data.data_)[1] == "abc");
}

}  // namespace test
}  // namespace base
}  // namespace lynx
