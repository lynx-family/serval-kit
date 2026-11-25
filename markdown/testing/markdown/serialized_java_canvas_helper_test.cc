// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "markdown/platform/android/internal/serialized_java_canvas_helper.h"
#include "mock_platform/mock_markdown_shaper.h"

namespace serval {
namespace markdown {

namespace {
class CommandReader {
 public:
  explicit CommandReader(const BufferOutputStream& buffer)
      : data_(buffer.GetBuffer()), size_(buffer.GetSize()) {}

  template <typename T>
  T Read() {
    T value{};
    EXPECT_LE(offset_ + sizeof(T), size_);
    if (offset_ + sizeof(T) <= size_) {
      std::memcpy(&value, data_ + offset_, sizeof(T));
      offset_ += sizeof(T);
    }
    return value;
  }

  void ExpectDefaultPaint(uint8_t flags = 0) {
    EXPECT_FLOAT_EQ(Read<float>(), 0);
    EXPECT_EQ(Read<uint32_t>(), 0u);
    EXPECT_EQ(Read<uint32_t>(), 0u);
    EXPECT_FLOAT_EQ(Read<float>(), 14);
    EXPECT_FLOAT_EQ(Read<float>(), 0);
    EXPECT_EQ(Read<uint8_t>(), flags);
  }

  void ExpectEnd() { EXPECT_EQ(offset_, size_); }

 private:
  const uint8_t* data_;
  size_t size_;
  size_t offset_ = 0;
};
}  // namespace

TEST(SerializedJavaCanvasHelper, KeepsExistingBufferDefaultByteOrder) {
  BufferOutputStream buffer;
  buffer.WriteInt32(0x01020304);
  const std::vector<uint8_t> expected = {1, 2, 3, 4};
  EXPECT_EQ(std::vector<uint8_t>(buffer.GetBuffer(),
                                 buffer.GetBuffer() + buffer.GetSize()),
            expected);
}

TEST(SerializedJavaCanvasHelper,
     PreservesJavaTransformOpcodesAndFloatEncoding) {
  internal::SerializedJavaCanvasHelper canvas;
  canvas.Save();
  canvas.Translate(1.f, 2.f);
  canvas.Restore();
  const std::vector<uint8_t> expected = {2, 4, 0, 0,    0x80, 0x3f,
                                         0, 0, 0, 0x40, 3};
  auto& buffer = canvas.GetBuffer();
  EXPECT_EQ(std::vector<uint8_t>(buffer.GetBuffer(),
                                 buffer.GetBuffer() + buffer.GetSize()),
            expected);
  canvas.ClearBuffer();
  EXPECT_EQ(buffer.GetSize(), 0u);
  canvas.Save();
  ASSERT_EQ(buffer.GetSize(), 1u);
  EXPECT_EQ(buffer.GetBuffer()[0], 2u);
}

TEST(SerializedJavaCanvasHelper, PreservesPaintAndLinearGradientEncoding) {
  using ttoffice::tttext::Painter;
  using ttoffice::tttext::ResolvedGradient;
  internal::SerializedJavaCanvasHelper canvas;
  Painter paint;
  paint.SetFillColor(0xff030201);
  paint.SetStrokeWidth(1.f);
  paint.SetTextSize(16.f);
  canvas.DrawRect(1, 2, 3, 4, &paint);
  std::vector<uint8_t> expected = {
      14, 0, 0,    0x80, 0x3f, 0,    0,    0,    0x40, 0, 0, 0x40, 0x40,
      0,  0, 0x80, 0x40, 0,    0,    0x80, 0x3f, 1,    2, 3, 0xff, 0,
      0,  0, 0,    0,    0,    0x80, 0x41, 0,    0,    0, 0, 0};
  auto& buffer = canvas.GetBuffer();
  EXPECT_EQ(std::vector<uint8_t>(buffer.GetBuffer(),
                                 buffer.GetBuffer() + buffer.GetSize()),
            expected);

  auto gradient = std::make_shared<ResolvedGradient>();
  gradient->end = {1, 2};
  gradient->colors = {0xff030201, 0xff060504};
  gradient->positions = {0, 1};
  paint.SetFillGradient(gradient);
  canvas.ClearBuffer();
  canvas.DrawRect(1, 2, 3, 4, &paint);
  expected.back() = 0x20;
  const std::vector<uint8_t> gradient_bytes = {
      1, 0, 0, 0, 0, 0,    0, 0, 0, 0,    0, 0x80, 0x3f, 0, 0, 0, 0x40, 2,   0,
      0, 0, 1, 2, 3, 0xff, 4, 5, 6, 0xff, 0, 0,    0,    0, 0, 0, 0x80, 0x3f};
  expected.insert(expected.end(), gradient_bytes.begin(), gradient_bytes.end());
  EXPECT_EQ(std::vector<uint8_t>(buffer.GetBuffer(),
                                 buffer.GetBuffer() + buffer.GetSize()),
            expected);
}

TEST(SerializedJavaCanvasHelper, PreservesClipAndShapeCommands) {
  internal::SerializedJavaCanvasHelper canvas;
  auto paint = canvas.CreatePainter();
  canvas.ClipRect(1, 2, 3, 4, true);
  canvas.DrawLine(5, 6, 7, 8, paint.get());
  paint->SetBold(true);
  paint->SetItalic(true);
  paint->SetUnderLine(true);
  paint->SetCap(tttext::Cap::kRound_Cap);
  canvas.DrawCircle(9, 10, 11, paint.get());
  canvas.DrawRoundRect(12, 13, 14, 15, 16, paint.get());

  CommandReader reader(canvas.GetBuffer());
  EXPECT_EQ(reader.Read<uint8_t>(), 8u);
  for (float value : {1.f, 2.f, 3.f, 4.f}) {
    EXPECT_FLOAT_EQ(reader.Read<float>(), value);
  }
  EXPECT_EQ(reader.Read<uint8_t>(), 13u);
  for (float value : {5.f, 6.f, 7.f, 8.f}) {
    EXPECT_FLOAT_EQ(reader.Read<float>(), value);
  }
  reader.ExpectDefaultPaint();
  EXPECT_EQ(reader.Read<uint8_t>(), 16u);
  for (float value : {9.f, 10.f, 11.f}) {
    EXPECT_FLOAT_EQ(reader.Read<float>(), value);
  }
  reader.ExpectDefaultPaint(0x1d);
  EXPECT_EQ(reader.Read<uint8_t>(), 27u);
  for (float value : {12.f, 13.f, 14.f, 15.f, 16.f}) {
    EXPECT_FLOAT_EQ(reader.Read<float>(), value);
  }
  reader.ExpectDefaultPaint(0x1d);
  reader.ExpectEnd();
}

TEST(SerializedJavaCanvasHelper, PreservesGlyphOrderAndOrigin) {
  internal::SerializedJavaCanvasHelper canvas;
  testing::MockMarkdownTypefaceHelper font;
  auto paint = canvas.CreatePainter();
  const uint16_t glyphs[] = {0x1234, 0x5678};
  float x[] = {2, 5};
  float y[] = {0, 0};
  canvas.DrawGlyphs(&font, 0, nullptr, nullptr, 0, 0, 0, nullptr, nullptr,
                    paint.get());
  EXPECT_EQ(canvas.GetBuffer().GetSize(), 0u);
  for (uint32_t text_bytes : {0u, 1u}) {
    canvas.ClearBuffer();
    canvas.DrawGlyphs(&font, 2, glyphs, nullptr, text_bytes, 10, 20, x, y,
                      paint.get());
    CommandReader reader(canvas.GetBuffer());
    EXPECT_EQ(reader.Read<uint8_t>(), 20u);
    EXPECT_EQ(reader.Read<uint32_t>(), font.GetUniqueId());
    EXPECT_EQ(reader.Read<int32_t>(), 2);
    EXPECT_EQ(reader.Read<uint16_t>(), glyphs[text_bytes == 1 ? 1 : 0]);
    EXPECT_EQ(reader.Read<uint16_t>(), glyphs[text_bytes == 1 ? 0 : 1]);
    EXPECT_FLOAT_EQ(reader.Read<float>(), 12);
    EXPECT_FLOAT_EQ(reader.Read<float>(), 20);
    reader.ExpectDefaultPaint();
    reader.ExpectEnd();
  }
}

TEST(SerializedJavaCanvasHelper, PreservesRadialAndConicGradientEncoding) {
  for (auto type :
       {tttext::GradientType::kRadial, tttext::GradientType::kConic}) {
    internal::SerializedJavaCanvasHelper canvas;
    auto paint = canvas.CreatePainter();
    auto gradient = std::make_shared<tttext::ResolvedGradient>();
    gradient->type = type;
    gradient->center = {3, 4};
    gradient->radius_x = 5;
    gradient->radius_y = 6;
    gradient->start_angle = 7;
    gradient->colors = {0xff010203, 0xff040506};
    gradient->positions = {0.25f, 0.75f};
    paint->SetFillGradient(gradient);
    canvas.DrawCircle(0, 1, 2, paint.get());

    CommandReader reader(canvas.GetBuffer());
    EXPECT_EQ(reader.Read<uint8_t>(), 16u);
    for (float value : {0.f, 1.f, 2.f}) {
      EXPECT_FLOAT_EQ(reader.Read<float>(), value);
    }
    reader.ExpectDefaultPaint(0x20);
    EXPECT_EQ(reader.Read<int8_t>(), static_cast<int8_t>(type));
    EXPECT_FLOAT_EQ(reader.Read<float>(), 3);
    EXPECT_FLOAT_EQ(reader.Read<float>(), 4);
    if (type == tttext::GradientType::kRadial) {
      EXPECT_FLOAT_EQ(reader.Read<float>(), 5);
      EXPECT_FLOAT_EQ(reader.Read<float>(), 6);
    } else {
      EXPECT_FLOAT_EQ(reader.Read<float>(), 7);
    }
    EXPECT_EQ(reader.Read<int32_t>(), 2);
    EXPECT_EQ(reader.Read<uint32_t>(), 0xff010203u);
    EXPECT_EQ(reader.Read<uint32_t>(), 0xff040506u);
    EXPECT_FLOAT_EQ(reader.Read<float>(), 0.25f);
    EXPECT_FLOAT_EQ(reader.Read<float>(), 0.75f);
    reader.ExpectEnd();
  }
}

}  // namespace markdown
}  // namespace serval
