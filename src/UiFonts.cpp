#include "UiFonts.h"

#include "fontIds.h"

namespace {
int activeSmallFontId = 0;
int activeUi10FontId = 0;
int activeUi12FontId = 0;
}  // namespace

namespace UiFonts {

int getUi10FontId() { return activeUi10FontId != 0 ? activeUi10FontId : UI_10_BUILTIN_FONT_ID; }

int getUi12FontId() { return activeUi12FontId != 0 ? activeUi12FontId : UI_12_BUILTIN_FONT_ID; }

int getSmallFontId() { return activeSmallFontId != 0 ? activeSmallFontId : SMALL_BUILTIN_FONT_ID; }

void setActiveCjkFontIds(int smallFontId, int ui10FontId, int ui12FontId) {
  activeSmallFontId = smallFontId;
  activeUi10FontId = ui10FontId;
  activeUi12FontId = ui12FontId;
}

void clearActiveCjkFontIds() { setActiveCjkFontIds(0, 0, 0); }

}  // namespace UiFonts
