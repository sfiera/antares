// Ares, a tactical space combat game.
// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef ANTARES_PIX_MAP_HPP_
#define ANTARES_PIX_MAP_HPP_

#include "sfz/SmartPtr.hpp"
#include "Geometry.hpp"

namespace sfz { class BinaryReader; }
namespace sfz { class BinaryWriter; }

namespace antares {

class ColorTable;
class RgbColor;

// A representation of pixel data.
//
// Defines an interface for objects which store pixel data, as well as some utility methods for
// manipulating the pixels.
//
// PixMap objects can be written to BinaryWriter objects, but not read back in, as that would
// potentially require resizing the PixMap, which is not a required part of the interface.
// However, ArrayPixMap provides both a `read()` and a `resize()` method.
class PixMap {
  public:
    virtual ~PixMap();

    // Core methods.
    //
    // These methods must be implemented by all subclasses and provide the basic interface to
    // PixMap.  Other, higher-level methods are listed below, under "Utility methods".

    // @returns             the size of the PixMap.  `left` and `top` must both be 0.
    virtual const Rect& bounds() const = 0;

    // The color table to interpret bytes according to.
    //
    // In order to determine the actual appearance of this PixMap, this color table should be used
    // to map the values in `bytes()` to the appropriate RgbColor.
    //
    // @returns             a read-only reference to this PixMap's color table.
    virtual const ColorTable& colors() const = 0;

    // Returns a pointer to the pixel at location (0, 0).
    //
    // A PixMap must be stored in a contiguous block of memory, and individual rows within the
    // PixMap are also required to occupy contiguous blocks.  Successive rows, however, may be
    // separated; see the documentation for `row_bytes()`.
    //
    // @returns             a read-only pointer to the first pixel in the PixMap.
    virtual const RgbColor* bytes() const = 0;

    // Returns the offset between successive rows in `bytes()`.
    //
    // Given a pixel at location `(x, y)` addressed by `p`, the pixels at `(x + 1, y)` and
    // `(x - 1, y)` are addressed by `p + 1` and `p - 1`, respectively (except at the edges).
    // Similarly, the pixels at `(x, y + 1)` and `(x, y - 1)` are addressed by `p + row_bytes()`
    // and `p - row_bytes()`.  However, `row_bytes()` is very much not required to be equal to
    // `bounds().width()`.
    //
    // @returns             the offset between rows in `bytes()`.
    virtual int row_bytes() const = 0;

    // @returns             a mutable version of `colors()`.
    virtual ColorTable* mutable_colors() = 0;

    // @returns             a mutable version of `bytes()`.
    virtual RgbColor* mutable_bytes() = 0;

    // XXX: hacks for transition
    virtual double transition_fraction() const = 0;
    virtual void set_transition_fraction(double fraction) = 0;
    virtual const RgbColor& transition_to() const = 0;
    virtual void set_transition_to(const RgbColor& color) = 0;

    // Utility methods.
    //
    // The following methods all have default implementations provided, as they can be implemented
    // purely in terms of the above methods.  Depending on the subclass, though, it may be possible
    // to implement them in a more efficient manner.

    // @param [in] y        a row.  Must be in the range [0, bounds().height()).
    // @returns             a read-only pointer to the first pixel in row `y`.
    virtual const RgbColor* row(int y) const;

    // @param [in] y        a row.  Must be in the range [0, bounds().height()).
    // @returns             a mutable pointer to the first pixel in row `y`.
    virtual RgbColor* mutable_row(int y);

    // @param [in] x        a column.  Must be in the range [0, bounds().width()).
    // @param [in] y        a row.  Must be in the range [0, bounds().height()).
    // @returns             the pixel at location (x, y).
    virtual const RgbColor& get(int x, int y) const;

    // @param [in] x        a column.  Must be in the range [0, bounds().width()).
    // @param [in] y        a row.  Must be in the range [0, bounds().height()).
    // @param [in] color    the color to set the pixel at location (x, y) to.
    virtual void set(int x, int y, const RgbColor& color);

    // Fills the entirety of the PixMap with a single color.
    //
    // Note the absence of a `Rect` parameter as part of `fill()`: this is intentional.  If you
    // want to fill a portion of the PixMap, then use a view.  For example, what might otherwise be
    // implemented as `pix->fill(rect, color)` should be instead `pix->view(rect).fill(color)`.
    //
    // @param [in] color    the color which should fill the PixMap.
    virtual void fill(const RgbColor& color);

    // Copies the entirety of another PixMap into this one.
    //
    // The sizes of the two PixMap objects must be identical.  As with `fill()`, there are no
    // `Rect` parameters given by this method; use views instead.
    //
    // @param [in] pix      the PixMap to copy from.
    // @throws Exception if `this->bounds()` and `pix.bounds()` are not equal.
    virtual void copy(const PixMap& pix);

    // Serializes this PixMap to a BinaryWriter.
    //
    // The format exported by this method is a custom binary format, but can be read by
    // `ArrayPixMap::read()`.  Additionally, there is a script provided as scripts/bintopng which
    // can convert the format to PNG images.
    virtual void write(sfz::BinaryWriter* bin) const;

    // See class documentation below.
    class View;

    // Returns a clipped view of this PixMap.
    //
    // Views are used when doing operations which only should consider part of a PixMap.  Memory is
    // shared with the parent PixMap, so they are lightweight for reading, and affect the parent
    // pixmap for writing.
    //
    // @param [in] bounds   the region to make a view of.  Must be enclosed by `this->bounds()`.
    View view(const Rect& bounds);
};

// PixMap subclass which provides its own storage.
//
// Implements the PixMap interface in terms of an array of pixel data stored in memory.  In
// addition to the methods inherited from PixMap, implements the `resize()` and `read()` methods.
class ArrayPixMap : public PixMap {
  public:
    // Creates a new PixMap with storage.
    //
    // Allocates `width * height` bytes of storage for pixels.  The pixels are uninitialized at
    // creation.
    //
    // @param [in] width    the desired width of the PixMap.
    // @param [in] height   the desired height of the PixMap.
    ArrayPixMap(int32_t width, int32_t height);

    // Deletes memory associated with the PixMap.
    ~ArrayPixMap();

    // Resizes the PixMap to the given size.
    //
    // All pixels which are within both the current and new bounds of this PixMap are copied over.
    // Other pixels are left uninitialized.
    //
    // @param [in] new_bounds a rect with the desired size of `bounds()` after resizing.
    void resize(const Rect& new_bounds);

    // Reads in pixel data via a BinaryReader.
    //
    // @param [in,out] bin  used to read binary data.
    void read(sfz::BinaryReader* bin);

    // Implementations of the core PixMap methods.
    virtual const Rect& bounds() const;
    virtual const ColorTable& colors() const;
    virtual const RgbColor* bytes() const;
    virtual int row_bytes() const;

    virtual ColorTable* mutable_colors();
    virtual RgbColor* mutable_bytes();

    // XXX: hacks for transition
    virtual double transition_fraction() const;
    virtual void set_transition_fraction(double fraction);
    virtual const RgbColor& transition_to() const;
    virtual void set_transition_to(const RgbColor& color);

  private:
    double _transition_fraction;
    sfz::scoped_ptr<RgbColor> _transition_to;
  public:

    // Uses default implementations of all utility PixMap methods.

  private:
    // The size of this PixMap.
    Rect _bounds;

    // The color table returned by `colors()`.
    sfz::scoped_ptr<ColorTable> _colors;

    // An array of pixel data.  Although not required to by the PixMap interface, ArrayPixMap
    // stores all rows of pixels contiguously.  This permits the optimization of `fill()` provided
    // above.
    sfz::scoped_array<RgbColor> _bytes;

    DISALLOW_COPY_AND_ASSIGN(ArrayPixMap);
};

// A clipped view of another PixMap.
//
// This class is lightweight, since it does not store any of its own pixel or color data.  It is
// accordingly copyable, for ease of use.
//
// In general, it is considered preferable to create views by using the `view()` method of
// PixMap, rather than invoking the constructor directly.  `PixMap::View(pix, r)` is equivalent to
// `pix->view(r)`.
class PixMap::View : public PixMap {
  public:
    // @param [in] pix      the PixMap to create a view of.
    // @param [in] bounds   the region to make a view of.  Must be enclosed by `pix->bounds()`.
    View(PixMap* pix, const Rect& bounds);

    // Implementations of the core PixMap methods.
    virtual const Rect& bounds() const;
    virtual const ColorTable& colors() const;
    virtual const RgbColor* bytes() const;
    virtual int row_bytes() const;

    virtual ColorTable* mutable_colors();
    virtual RgbColor* mutable_bytes();

    // XXX: hacks for transition
    virtual double transition_fraction() const;
    virtual void set_transition_fraction(double fraction);
    virtual const RgbColor& transition_to() const;
    virtual void set_transition_to(const RgbColor& color);

    // Uses default implementations of all utility PixMap methods.

  private:
    // The PixMap that this is a view of.
    PixMap* const _parent;

    // The offset from the upper left corner of `_parent` to the upper left corner of `this`.
    const Point _offset;

    // The size of this view.
    //
    // TODO(sfiera): change the type to `const Rect`.
    Rect _bounds;

    // ALLOW_COPY_AND_ASSIGN(View);
};

}  // namespace antares

#endif  // ANTARES_PIX_MAP_HPP_
