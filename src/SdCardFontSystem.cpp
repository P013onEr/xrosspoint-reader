#include "SdCardFontSystem.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "UiFonts.h"

namespace {

static uint8_t fontSizeEnumFromSettings() {
  uint8_t e = SETTINGS.fontSize;
  if (e >= CrossPointSettings::FONT_SIZE_COUNT) e = 1;  // default to MEDIUM
  return e;
}

const char* uiFamilyForLanguage(Language lang) {
  switch (lang) {
    case Language::ZH_CN:
      return "NotoSansCJKscUI";
    case Language::ZH_TW:
      return "NotoSansCJKtcUI";
    case Language::JA:
      return "NotoSansCJKjpUI";
    case Language::KO:
      return "NotoSansCJKkrUI";
    case Language::EN:
    default:
      return nullptr;
  }
}

static constexpr const char* kLanguagePickerUiFamilies[] = {
    "NotoSansCJKscUI",
    "NotoSansCJKtcUI",
    "NotoSansCJKjpUI",
    "NotoSansCJKkrUI",
};

}  // namespace

void SdCardFontSystem::begin(GfxRenderer& renderer) {
  registry_.discover();

  // Register this system as the SD font ID resolver in settings.
  // Uses a static trampoline since CrossPointSettings stores a plain function pointer.
  SETTINGS.sdFontIdResolver = [](void* ctx, const char* familyName, uint8_t fontSizeEnum) -> int {
    return static_cast<SdCardFontSystem*>(ctx)->resolveFontId(familyName, fontSizeEnum);
  };
  SETTINGS.sdFontResolverCtx = this;

  ensureUiLoaded(renderer);

  // If user has a saved SD font selection, load it
  if (SETTINGS.sdFontFamilyName[0] != '\0') {
    const auto* family = registry_.findFamily(SETTINGS.sdFontFamilyName);
    if (family) {
      if (manager_.loadFamily(*family, renderer, fontSizeEnumFromSettings())) {
        LOG_DBG("SDFS", "Loaded SD card font family: %s", SETTINGS.sdFontFamilyName);
      } else {
        LOG_ERR("SDFS", "Failed to load SD font family: %s (clearing)", SETTINGS.sdFontFamilyName);
        SETTINGS.sdFontFamilyName[0] = '\0';
      }
    } else {
      LOG_DBG("SDFS", "SD font family not found on card: %s (clearing)", SETTINGS.sdFontFamilyName);
      SETTINGS.sdFontFamilyName[0] = '\0';
    }
  }

  LOG_DBG("SDFS", "SD font system ready (%d families discovered)", registry_.getFamilyCount());
}

void SdCardFontSystem::ensureLoaded(GfxRenderer& renderer) {
  // If the web server (or another task) installed/deleted fonts, re-discover.
  // Track whether we just re-discovered so we can force a reload below even
  // when the wanted family/size still maps to the same point size — the file
  // contents on disk may have changed (e.g. user re-uploaded a new build).
  const bool registryWasDirty = registryDirty_.exchange(false, std::memory_order_acquire);
  if (registryWasDirty) {
    LOG_DBG("SDFS", "Registry dirty — re-discovering fonts");
    registry_.discover();
  }

  ensureUiLoaded(renderer);

  const char* wantedFamily = SETTINGS.sdFontFamilyName;
  const std::string& currentFamily = manager_.currentFamilyName();
  const uint8_t sizeEnum = fontSizeEnumFromSettings();

  if (wantedFamily[0] == '\0') {
    if (!currentFamily.empty()) {
      manager_.unloadAll(renderer);
    }
    return;
  }

  // Reload if family changed OR if the user-selected size maps to a
  // different file than what's currently loaded OR if the registry was
  // just rediscovered (file may have been replaced on disk).
  bool familyMatches = (currentFamily == wantedFamily);
  if (familyMatches) {
    const auto* family = registry_.findFamily(wantedFamily);
    if (!family) {
      LOG_DBG("SDFS", "SD font family disappeared: %s (clearing)", wantedFamily);
      manager_.unloadAll(renderer);
      SETTINGS.sdFontFamilyName[0] = '\0';
      return;
    }
    const auto* selected = family->findClosestReaderSize(sizeEnum);
    const uint8_t wantedPt = selected ? selected->pointSize : 0;
    if (!registryWasDirty && wantedPt == manager_.currentPointSize()) return;
    LOG_DBG("SDFS", "Reloading %s: size %u -> %u (enum %u)%s", wantedFamily, manager_.currentPointSize(), wantedPt,
            sizeEnum, registryWasDirty ? " [registry dirty]" : "");
  }

  if (!currentFamily.empty()) {
    manager_.unloadAll(renderer);
  }

  const auto* family = registry_.findFamily(wantedFamily);
  if (family) {
    if (manager_.loadFamily(*family, renderer, sizeEnum)) {
      LOG_DBG("SDFS", "Loaded SD font family: %s", wantedFamily);
    } else {
      LOG_ERR("SDFS", "Failed to load SD font family: %s (clearing)", wantedFamily);
      SETTINGS.sdFontFamilyName[0] = '\0';
    }
  } else {
    LOG_DBG("SDFS", "SD font family not found: %s (clearing)", wantedFamily);
    SETTINGS.sdFontFamilyName[0] = '\0';
  }
}

void SdCardFontSystem::ensureUiLoaded(GfxRenderer& renderer) {
  loadUiFamily(renderer, uiFamilyForLanguage(I18N.getLanguage()));
}

void SdCardFontSystem::ensureLanguagePickerUiLoaded(GfxRenderer& renderer) {
  const char* currentLanguageFamily = uiFamilyForLanguage(I18N.getLanguage());
  if (currentLanguageFamily) {
    loadUiFamily(renderer, currentLanguageFamily);
    return;
  }

  for (const char* familyName : kLanguagePickerUiFamilies) {
    if (registry_.findFamily(familyName)) {
      loadUiFamily(renderer, familyName);
      return;
    }
  }

  loadUiFamily(renderer, nullptr);
}

bool SdCardFontSystem::loadUiFamily(GfxRenderer& renderer, const char* wantedFamily) {
  const std::string& currentFamily = uiManager_.currentFamilyName();

  if (!wantedFamily || wantedFamily[0] == '\0') {
    if (!currentFamily.empty()) {
      uiManager_.unloadAll(renderer);
    }
    UiFonts::clearActiveCjkFontIds();
    return false;
  }

  if (currentFamily == wantedFamily) return true;

  if (!currentFamily.empty()) {
    uiManager_.unloadAll(renderer);
  }
  UiFonts::clearActiveCjkFontIds();

  const auto* family = registry_.findFamily(wantedFamily);
  if (!family) {
    LOG_DBG("SDFS", "CJK UI font family not found: %s", wantedFamily);
    return false;
  }

  static constexpr uint8_t kUiPointSizes[] = {8, 10, 12};
  if (!uiManager_.loadFamilyPointSizes(*family, renderer, kUiPointSizes, sizeof(kUiPointSizes))) {
    LOG_ERR("SDFS", "Failed to load CJK UI font family: %s", wantedFamily);
    return false;
  }

  UiFonts::setActiveCjkFontIds(uiManager_.getFontId(wantedFamily, 8), uiManager_.getFontId(wantedFamily, 10),
                               uiManager_.getFontId(wantedFamily, 12));
  LOG_DBG("SDFS", "Loaded CJK UI font family: %s", wantedFamily);
  return true;
}

int SdCardFontSystem::resolveFontId(const char* familyName, uint8_t /*fontSizeEnum*/) const {
  // The manager loads exactly one size (closest to SETTINGS.fontSize), so the
  // enum is implicit — always return the single loaded font ID for this family.
  // ensureLoaded() must have been called with the current settings before this.
  return manager_.getFontId(familyName);
}
