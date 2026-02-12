#include "plugins/MarqueePlugin.h"
#include "screen.h"
#include "signs.h"

// Cyrillic uppercase glyphs (5x7, MSB-left, 5 columns packed in high bits of byte)
// Index 0=А(0x410) .. 31=Я(0x42F), 32=Ё(0x401)
static const uint8_t CYRILLIC_GLYPHS[33][7] = {
    {0x70, 0x88, 0x88, 0x88, 0xF8, 0x88, 0x88}, // А
    {0xF8, 0x80, 0xF0, 0x88, 0x88, 0x88, 0xF0}, // Б
    {0xF0, 0x88, 0x88, 0xF0, 0x88, 0x88, 0xF0}, // В
    {0xF8, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, // Г
    {0x30, 0x50, 0x50, 0x50, 0x50, 0xF8, 0x88}, // Д
    {0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0xF8}, // Е
    {0xA8, 0xA8, 0x70, 0x20, 0x70, 0xA8, 0xA8}, // Ж
    {0x70, 0x88, 0x08, 0x30, 0x08, 0x88, 0x70}, // З
    {0x88, 0x88, 0x98, 0xA8, 0xC8, 0x88, 0x88}, // И
    {0x50, 0x20, 0x98, 0xA8, 0xC8, 0x88, 0x88}, // Й
    {0x88, 0x90, 0xA0, 0xC0, 0xA0, 0x90, 0x88}, // К
    {0x78, 0x48, 0x48, 0x48, 0x48, 0x48, 0x88}, // Л
    {0x88, 0xD8, 0xA8, 0x88, 0x88, 0x88, 0x88}, // М
    {0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x88}, // Н
    {0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70}, // О
    {0xF8, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88}, // П
    {0xF0, 0x88, 0x88, 0xF0, 0x80, 0x80, 0x80}, // Р
    {0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70}, // С
    {0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}, // Т
    {0x88, 0x88, 0x88, 0x78, 0x08, 0x08, 0x70}, // У
    {0x20, 0x70, 0xA8, 0xA8, 0xA8, 0x70, 0x20}, // Ф
    {0x88, 0x88, 0x50, 0x20, 0x50, 0x88, 0x88}, // Х
    {0x90, 0x90, 0x90, 0x90, 0x90, 0xF8, 0x08}, // Ц
    {0x88, 0x88, 0x88, 0x78, 0x08, 0x08, 0x08}, // Ч
    {0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xF8}, // Ш
    {0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xF8, 0x08}, // Щ
    {0xC0, 0x40, 0x40, 0x70, 0x48, 0x48, 0x70}, // Ъ
    {0x88, 0x88, 0xE8, 0xA8, 0xA8, 0xA8, 0xE8}, // Ы
    {0x80, 0x80, 0xE0, 0x90, 0x90, 0x90, 0xE0}, // Ь
    {0x70, 0x88, 0x08, 0x38, 0x08, 0x88, 0x70}, // Э
    {0x90, 0xA8, 0xA8, 0xE8, 0xA8, 0xA8, 0x90}, // Ю
    {0x78, 0x88, 0x88, 0x78, 0x28, 0x48, 0x88}, // Я
    {0x50, 0x00, 0xF8, 0x80, 0xF0, 0x80, 0xF8}, // Ё
};

static uint16_t nextUTF8(const char *str, int &idx)
{
  uint8_t b = (uint8_t)str[idx];
  if (b == 0)
    return 0;
  if (b < 0x80)
  {
    idx++;
    return b;
  }
  if ((b & 0xE0) == 0xC0 && str[idx + 1])
  {
    uint16_t cp = ((b & 0x1F) << 6) | ((uint8_t)str[idx + 1] & 0x3F);
    idx += 2;
    return cp;
  }
  if ((b & 0xF0) == 0xE0 && str[idx + 1] && str[idx + 2])
  {
    idx += 3;
    return '?';
  }
  idx++;
  return '?';
}

void MarqueePlugin::decodeText()
{
  numChars = 0;
  totalWidth = 0;
  int idx = 0;
  while (text[idx] && numChars < 200)
  {
    uint16_t cp = nextUTF8(text, idx);
    if (cp == 0)
      break;
    codepoints[numChars++] = cp;
    totalWidth += glyphWidth(cp) + 1; // +1 spacing
  }
  if (numChars > 0)
    totalWidth--; // no trailing space
}

int MarqueePlugin::glyphWidth(uint16_t cp)
{
  // Cyrillic uppercase or Ё
  if (cp == 0x401 || (cp >= 0x410 && cp <= 0x42F))
    return 5;
  // Cyrillic lowercase → treat as uppercase
  if (cp == 0x451 || (cp >= 0x430 && cp <= 0x44F))
    return 5;
  // Latin/ASCII - use system font
  if (cp >= fonts[0].offset && cp < fonts[0].offset + (int)fonts[0].data.size())
    return fonts[0].sizeX;
  return 5; // fallback
}

void MarqueePlugin::drawGlyph(int x, int y, uint16_t cp)
{
  const uint8_t *glyph = nullptr;
  int width = 5;

  // Map lowercase Cyrillic to uppercase
  if (cp >= 0x430 && cp <= 0x44F)
    cp -= 0x20;
  if (cp == 0x451)
    cp = 0x401;

  // Cyrillic
  if (cp >= 0x410 && cp <= 0x42F)
  {
    glyph = CYRILLIC_GLYPHS[cp - 0x410];
  }
  else if (cp == 0x401)
  {
    glyph = CYRILLIC_GLYPHS[32]; // Ё
  }

  if (glyph)
  {
    // Draw from packed bytes (MSB = leftmost pixel, 5 columns)
    for (int row = 0; row < 7; row++)
    {
      uint8_t rowData = glyph[row];
      for (int col = 0; col < width; col++)
      {
        if (rowData & (0x80 >> col))
        {
          Screen.setPixel(x + col, y + row, 1, MAX_BRIGHTNESS);
        }
      }
    }
    return;
  }

  // Latin/ASCII - use system font
  if (cp >= fonts[0].offset && cp < fonts[0].offset + (int)fonts[0].data.size())
  {
    int fontIdx = cp - fonts[0].offset;
    if (fontIdx >= 0 && fontIdx < (int)fonts[0].data.size())
    {
      std::vector<int> bits = Screen.readBytes(fonts[0].data[fontIdx]);
      int bitCount = 8; // system font uses 8-bit wide rows
      for (int i = 0; i < (int)bits.size(); i += bitCount)
      {
        for (int j = 0; j < fonts[0].sizeX && j < bitCount; j++)
        {
          if (bits[i + j])
          {
            Screen.setPixel(x + j, y + (i / bitCount), 1, MAX_BRIGHTNESS);
          }
        }
      }
    }
    return;
  }

  // Unknown char - draw '?'
  if (fonts[0].data.size() > ('?' - fonts[0].offset))
  {
    int fontIdx = '?' - fonts[0].offset;
    std::vector<int> bits = Screen.readBytes(fonts[0].data[fontIdx]);
    for (int i = 0; i < (int)bits.size(); i += 8)
    {
      for (int j = 0; j < 5 && j < 8; j++)
      {
        if (bits[i + j])
        {
          Screen.setPixel(x + j, y + (i / 8), 1, MAX_BRIGHTNESS);
        }
      }
    }
  }
}

void MarqueePlugin::renderFrame()
{
  Screen.clear();
  int x = -scrollPos;
  int yOffset = 5; // center 7px tall glyphs vertically: (16-7)/2 ≈ 5

  for (int i = 0; i < numChars; i++)
  {
    int w = glyphWidth(codepoints[i]);
    // Only draw if on screen
    if (x + w > 0 && x < 16)
    {
      drawGlyph(x, yOffset, codepoints[i]);
    }
    x += w + 1; // +1 for spacing
  }
}

void MarqueePlugin::setup()
{
  // Initialize text to empty
  memset(text, 0, sizeof(text));
  strcpy(text, "Hello!");  // Default text
  scrollTimer.forceReady();
  decodeText();
  scrollPos = -16;
  Serial.println("[MarqueePlugin] Setup complete");
}

void MarqueePlugin::loop()
{
  if (numChars == 0)
    return;

  if (scrollTimer.isReady(speed))
  {
    renderFrame();
    scrollPos++;
    if (scrollPos > totalWidth)
    {
      scrollPos = -16;
    }
  }
}

void MarqueePlugin::websocketHook(JsonDocument &request)
{
  if (request["event"] == "marquee")
  {
    if (request["text"].is<String>())
    {
      String newText = request["text"].as<String>();
      if (newText.length() > 0)
      {
        strncpy(text, newText.c_str(), sizeof(text) - 1);
        text[sizeof(text) - 1] = '\0';
        Serial.print("[MarqueePlugin] New text: ");
        Serial.println(text);
        decodeText();
        scrollPos = -16;
        scrollTimer.forceReady(); // Force immediate render on next loop
        renderFrame(); // Render immediately to show the new text
      }
      else
      {
        Serial.println("[MarqueePlugin] WARNING: Empty text received!");
      }
    }
    else
    {
      Serial.println("[MarqueePlugin] WARNING: text is not a string!");
    }
    if (request["speed"].is<int>())
    {
      speed = request["speed"].as<int>();
      if (speed < 10)
        speed = 10;
      if (speed > 500)
        speed = 500;
    }
  }
}

const char *MarqueePlugin::getName() const
{
  return "Marquee";
}
