/**
 * @file STScreenBuffer.h
 * @details Class for pixel working with images. One could get access
 * to image bits using this library or create custom image.
 * @author Vassili Philippov (vasja@spbteam.com)
 * @date Created: June 2001
 * @date Last changed: 07 August 2001
 * @version 2.0
 */

#ifndef XCSOAR_SCREEN_RAW_BITMAP_HPP
#define XCSOAR_SCREEN_RAW_BITMAP_HPP

#include "PortableColor.hpp"
#include "OS/ByteOrder.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Features.hpp"
#endif

#ifdef USE_GDI
#include <windef.h>
#include <wingdi.h>
class LKSurface;
using Canvas = LKSurface;
#else
class Canvas;
#endif

#include <stdint.h>
#include <utility>
#include "Compiler.h"



#ifdef ENABLE_OPENGL
class GLTexture;
#endif

/**
 * BGRColor structure encapsulates color information about one point. Color
 * order is Blue, Green, Red (not RGB).
 */
struct BGRColor
{
  BGRColor() = default;

#ifdef GREYSCALE
  Luminosity8 value;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :value(R, G, B) {}

    constexpr explicit BGRColor(Luminosity8&& _value)
    :value(std::forward<Luminosity8>(_value)) {}

#elif defined(HAVE_GLES)
  RGB565Color value;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :value(R, G, B) {}

  constexpr explicit BGRColor(const RGB8Color& _value)
    :value(_value.Red(), _value.Green(), _value.Blue()) {}

#elif defined(USE_MEMORY_CANVAS) || defined(ENABLE_SDL) || defined(USE_EGL)

#if IS_BIG_ENDIAN
  /* big-endian */
  uint8_t dummy;
  RGB8Color value;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :dummy(), value(R, G, B) {}

  constexpr explicit BGRColor(const RGB8Color& _value)
    :dummy(), value(_value.Red(), _value.Green(), _value.Blue()) {}  

#else
  /* little-endian */
  BGR8Color value;
  uint8_t dummy;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :value(R, G, B), dummy() {}

  constexpr explicit BGRColor(const RGB8Color& _value)
    :value(_value.Red(), _value.Green(), _value.Blue()), dummy() {}

#endif

#else /* !SDL */

  BGR8Color value;

  constexpr BGRColor(uint8_t R, uint8_t G, uint8_t B)
    :value(R, G, B) {}

  constexpr explicit BGRColor(const RGB8Color& _value)
    :value(_value.Red(), _value.Green(), _value.Blue()) {}  
#endif
};

/**
 * This class provides fast drawing methods and offscreen buffer.
 */
class RawBitmap {
protected:
  const unsigned width;
  const unsigned height;
  const unsigned corrected_width;
  BGRColor *buffer;

#ifdef ENABLE_OPENGL
  GLTexture *texture;

  /**
   * Has the buffer been modified, and needs to be copied into the
   * texture?
   */
  mutable bool dirty;
#elif defined(USE_GDI)
  BITMAPINFO bi;

#if defined(_WIN32_WCE) && _WIN32_WCE < 0x0400
  HBITMAP bitmap;
#endif
#endif

public:
  /**
   * Creates buffer with the given size and fills it with
   * the given color
   * @param nWidth Width of the buffer
   * @param nHeight Height of the buffer
   * @param clr Fill color of the buffer
   */
  RawBitmap(unsigned width, unsigned height);

  virtual ~RawBitmap();

  /**
   * Returns the Buffer
   * @return The Buffer as BGRColor array
   */
  BGRColor *GetBuffer() const {
    return buffer;
  }

  /**
   * Returns a pointer to the top-most row.
   */
  BGRColor *GetTopRow() const {
#ifndef USE_GDI
    return buffer;
#else
  /* in WIN32 bitmaps, the bottom-most row comes first */
    return buffer + (height - 1) * corrected_width;
#endif
  }

  /**
   * Returns a pointer to the row below the current one.
   */
  BGRColor *GetNextRow(BGRColor *row) const {
#ifndef USE_GDI
    return row + corrected_width;
#else
    return row - corrected_width;
#endif
  }

  void SetDirty() {
#ifdef ENABLE_OPENGL
    dirty = true;
#endif
  }

  /**
   * Returns a pointer to the row 
   */
  BGRColor *GetRow(unsigned row) const {
#ifndef USE_GDI
    return GetTopRow() + corrected_width * row;
#else
    return GetTopRow() - corrected_width * row;
#endif
  }

  /**
   * Returns real width of the screen buffer. It could be slightly more then
   * requested width. This parameter is important only when you work with
   * points array directly (using GetPointsArray function).
   * @return Real width of the screen buffer
   */
  unsigned GetCorrectedWidth() const {
    return corrected_width;
  }

  /**
   * Returns the screen buffer width
   * @return The screen buffer width
   */
  unsigned GetWidth() const {
    return width;
  }

  /**
   * Returns screen buffer height
   * @return The screen buffer height
   */
  unsigned GetHeight() const {
    return height;
  }

#ifdef ENABLE_OPENGL
  /**
   * Bind the texture and return a reference to it.  If the texture is
   * "dirty", then the RAM buffer will be copied to the texture by
   * this method.
   */
  GLTexture &BindAndGetTexture() const;
#endif

  void StretchTo(unsigned width, unsigned height, Canvas &dest_canvas,
                 unsigned dest_x, unsigned dest_y, unsigned dest_width, unsigned dest_height) const;

};

#endif // !defined(XCSOAR_SCREEN_RAW_BITMAP_HPP)
