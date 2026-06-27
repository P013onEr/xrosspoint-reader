#pragma once

namespace UiFonts {

int getUi10FontId();
int getUi12FontId();
int getSmallFontId();

void setActiveCjkFontIds(int smallFontId, int ui10FontId, int ui12FontId);
void clearActiveCjkFontIds();

}  // namespace UiFonts
