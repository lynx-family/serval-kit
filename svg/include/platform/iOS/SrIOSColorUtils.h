#ifndef SVG_INCLUDE_PLATFORM_IOS_SRIOSCOLORUTILS_H_
#define SVG_INCLUDE_PLATFORM_IOS_SRIOSCOLORUTILS_H_

#import <UIKit/UIKit.h>

#include <cstdint>

namespace serval {
namespace svg {
namespace ios {

class SrIOSColorUtils {
 public:
  static UIColor* UIColorFromARGB(uint32_t color);
  static bool UIColorToARGB(UIColor* color, uint32_t* out_color);
  static CGFloat GetRedFromARGB(uint32_t color);
  static CGFloat GetGreenFromARGB(uint32_t color);
  static CGFloat GetBlueFromARGB(uint32_t color);
  static CGFloat GetAlphaFromARGB(uint32_t color, float opacity);
};

}  // namespace ios
}  // namespace svg
}  // namespace serval

#endif  // SVG_INCLUDE_PLATFORM_IOS_SRIOSCOLORUTILS_H_
