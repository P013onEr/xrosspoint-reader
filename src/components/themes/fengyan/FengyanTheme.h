#pragma once

#include "components/themes/lyra/LyraTheme.h"

namespace FengyanMetrics {
constexpr ThemeMetrics values = [] {
  ThemeMetrics v = LyraMetrics::values;
  v.homeRecentBooksCount = 3;
  v.homeTopPadding = 50;
  v.homeCoverHeight = 226;
  v.homeCoverTileHeight = 322;
  v.homeMenuTopOffset = 12;
  v.menuRowHeight = 92;
  v.menuSpacing = 20;
  v.contentSidePadding = 20;
  return v;
}();
}  // namespace FengyanMetrics

class FengyanTheme : public LyraTheme {
 public:
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           std::function<bool()> storeCoverBuffer) const override;
  void drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                      const std::function<std::string(int index)>& buttonLabel,
                      const std::function<UIIcon(int index)>& rowIcon) const override;
};
