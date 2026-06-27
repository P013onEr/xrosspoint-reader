#include "FengyanTheme.h"

#include <Bitmap.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include <algorithm>
#include <string>

#include "RecentBooksStore.h"
#include "components/UITheme.h"
#include "components/icons/book.h"
#include "components/icons/bookmark.h"
#include "components/icons/cover.h"
#include "components/icons/folder.h"
#include "components/icons/hotspot.h"
#include "components/icons/library.h"
#include "components/icons/recent.h"
#include "components/icons/settings2.h"
#include "components/icons/transfer.h"
#include "components/icons/wifi.h"
#include "fontIds.h"

namespace {
constexpr int kCornerRadius = 8;
constexpr int kCoverGap = 16;
constexpr int kCoverTopPadding = 4;
constexpr int kCardGap = 10;
constexpr int kCardPaddingX = 12;
constexpr int kIconSize = 32;
constexpr int kMenuColumns = 3;
constexpr int kMenuGapX = 16;
constexpr int kMenuGapY = 20;

const uint8_t* iconForName(UIIcon icon) {
  switch (icon) {
    case UIIcon::Folder:
      return FolderIcon;
    case UIIcon::Book:
      return BookIcon;
    case UIIcon::Recent:
      return RecentIcon;
    case UIIcon::Settings:
      return Settings2Icon;
    case UIIcon::Transfer:
      return TransferIcon;
    case UIIcon::Library:
      return LibraryIcon;
    case UIIcon::Wifi:
      return WifiIcon;
    case UIIcon::Hotspot:
      return HotspotIcon;
    case UIIcon::Bookmark:
      return BookmarkIcon;
    default:
      return nullptr;
  }
}

void drawCoverPlaceholder(const GfxRenderer& renderer, Rect cover) {
  renderer.drawRect(cover.x, cover.y, cover.width, cover.height, true);
  renderer.fillRect(cover.x, cover.y + cover.height / 3, cover.width, cover.height * 2 / 3, true);
  renderer.drawIcon(CoverIcon, cover.x + std::max(8, (cover.width - kIconSize) / 2), cover.y + 24, kIconSize,
                    kIconSize);
}

void drawCover(GfxRenderer& renderer, const RecentBook& book, Rect cover) {
  bool hasCover = false;
  if (!book.coverBmpPath.empty()) {
    const std::string coverBmpPath = UITheme::getCoverThumbPath(book.coverBmpPath, cover.height);
    HalFile file;
    if (Storage.openFileForRead("HOME", coverBmpPath, file)) {
      Bitmap bitmap(file);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        const int bitmapWidth = bitmap.getWidth();
        const int drawWidth = bitmapWidth > 0 ? std::min(bitmapWidth, cover.width) : cover.width;
        const int drawX = cover.x + (cover.width - drawWidth) / 2;
        renderer.drawBitmap(bitmap, drawX, cover.y, drawWidth, cover.height);
        renderer.drawRect(drawX, cover.y, drawWidth, cover.height, true);
        hasCover = true;
      }
      file.close();
    }
  }

  if (!hasCover) {
    drawCoverPlaceholder(renderer, cover);
  }
}
}  // namespace

void FengyanTheme::drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                                       int selectorIndex, bool& coverRendered, bool& coverBufferStored,
                                       bool& bufferRestored, std::function<bool()> storeCoverBuffer) const {
  (void)bufferRestored;

  const int visibleCount = std::min(static_cast<int>(recentBooks.size()), FengyanMetrics::values.homeRecentBooksCount);
  if (visibleCount == 0) {
    drawEmptyRecents(renderer, rect);
    return;
  }

  const int sidePadding = FengyanMetrics::values.contentSidePadding;
  const int totalGap = kCoverGap * (FengyanMetrics::values.homeRecentBooksCount - 1);
  const int tileWidth = (rect.width - sidePadding * 2 - totalGap) / FengyanMetrics::values.homeRecentBooksCount;
  const int coverHeight = FengyanMetrics::values.homeCoverHeight;
  const int coverWidth = std::min(tileWidth - 8, static_cast<int>(coverHeight * 0.6f));
  const int coverY = rect.y + kCoverTopPadding;

  if (!coverRendered) {
    for (int i = 0; i < visibleCount; ++i) {
      const int tileX = rect.x + sidePadding + i * (tileWidth + kCoverGap);
      const Rect cover{tileX + (tileWidth - coverWidth) / 2, coverY, coverWidth, coverHeight};
      drawCover(renderer, recentBooks[i], cover);
    }
    coverBufferStored = storeCoverBuffer();
    coverRendered = coverBufferStored;
  }

  const bool bookSelected = selectorIndex >= 0 && selectorIndex < visibleCount;
  const int selectedBookIndex = bookSelected ? selectorIndex : 0;
  if (bookSelected) {
    const int tileX = rect.x + sidePadding + selectedBookIndex * (tileWidth + kCoverGap);
    const Rect cover{tileX + (tileWidth - coverWidth) / 2, coverY, coverWidth, coverHeight};
    renderer.drawRoundedRect(cover.x - 3, cover.y - 3, cover.width + 6, cover.height + 6, 1, kCornerRadius, true);
  }

  const int cardY = coverY + coverHeight + kCardGap;
  const int cardHeight = std::max(0, rect.y + rect.height - cardY);
  if (cardHeight <= 0) return;

  renderer.fillRoundedRect(rect.x + sidePadding, cardY, rect.width - sidePadding * 2, cardHeight, kCornerRadius,
                           Color::LightGray);

  const RecentBook& book = recentBooks[selectedBookIndex];
  const int textX = rect.x + sidePadding + kCardPaddingX;
  const int textWidth = rect.width - sidePadding * 2 - kCardPaddingX * 2;
  const std::string title = renderer.truncatedText(UI_12_FONT_ID, book.title.c_str(), textWidth, EpdFontFamily::BOLD);
  const int titleY = cardY + 10;
  renderer.drawText(UI_12_FONT_ID, textX, titleY, title.c_str(), true, EpdFontFamily::BOLD);

  if (!book.author.empty()) {
    const std::string author = renderer.truncatedText(UI_10_FONT_ID, book.author.c_str(), textWidth);
    renderer.drawText(UI_10_FONT_ID, textX, titleY + renderer.getLineHeight(UI_12_FONT_ID) + 6, author.c_str(), true);
  }
}

void FengyanTheme::drawButtonMenu(GfxRenderer& renderer, Rect rect, int buttonCount, int selectedIndex,
                                  const std::function<std::string(int index)>& buttonLabel,
                                  const std::function<UIIcon(int index)>& rowIcon) const {
  const int sidePadding = FengyanMetrics::values.contentSidePadding;
  const int gridWidth = rect.width - sidePadding * 2;
  const int cellWidth = (gridWidth - kMenuGapX * (kMenuColumns - 1)) / kMenuColumns;
  const int cellHeight = FengyanMetrics::values.menuRowHeight;
  const int labelLineHeight = renderer.getLineHeight(UI_10_FONT_ID);

  for (int i = 0; i < buttonCount; ++i) {
    const int row = i / kMenuColumns;
    const int col = i % kMenuColumns;
    const int cellX = rect.x + sidePadding + col * (cellWidth + kMenuGapX);
    const int cellY = rect.y + row * (cellHeight + kMenuGapY);
    const bool selected = selectedIndex == i;

    if (selected) {
      renderer.fillRoundedRect(cellX, cellY, cellWidth, cellHeight, kCornerRadius, Color::LightGray);
    }

    const int iconX = cellX + (cellWidth - kIconSize) / 2;
    const int iconY = cellY + 12;
    const uint8_t* icon = rowIcon ? iconForName(rowIcon(i)) : nullptr;
    if (icon) {
      renderer.drawIcon(icon, iconX, iconY, kIconSize, kIconSize);
    } else {
      renderer.fillRect(iconX, iconY, kIconSize, kIconSize);
    }

    const std::string label = renderer.truncatedText(UI_10_FONT_ID, buttonLabel(i).c_str(), cellWidth - 8);
    const int textWidth = renderer.getTextWidth(UI_10_FONT_ID, label.c_str());
    const int textX = cellX + (cellWidth - textWidth) / 2;
    const int textY = iconY + kIconSize + 8;
    if (textY + labelLineHeight <= cellY + cellHeight) {
      renderer.drawText(UI_10_FONT_ID, textX, textY, label.c_str(), true);
    }
  }
}
