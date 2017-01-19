// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2008-2017 The Antares Authors
//
// This file is part of Antares, a tactical space combat game.
//
// Antares is free software: you can redistribute it and/or modify it
// under the terms of the Lesser GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Antares is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Antares.  If not, see http://www.gnu.org/licenses/

#ifndef ANTARES_UI_EVENT_HPP_
#define ANTARES_UI_EVENT_HPP_

#include <stdint.h>
#include <sfz/sfz.hpp>

#include "math/geometry.hpp"
#include "math/units.hpp"

namespace antares {

class EventReceiver;

enum InputMode {
    KEYBOARD_MOUSE,
    GAMEPAD,
};

// Superclass for all events.
class Event {
  public:
    Event(wall_time at);
    virtual ~Event();

    wall_time at() const;

    // Calls receiver->m(*this), for the appropriate EventReceiver method 'm'.
    virtual void send(EventReceiver* receiver) const = 0;

  private:
    const wall_time _at;

    DISALLOW_COPY_AND_ASSIGN(Event);
};

// Superclass for events involving the keyboard (press, release).
//
// The value provided in `key()` is an ADB key-code, and is a horrible artifact of Ares' origins on
// the classic MacOS.  For the time being, this means we are not tolerant of non-US keyboard
// layouts, although it remains relatively easy to implement within Cocoa.
//
// We will eventually want to use a more cross-platform/international/sane numbering of keys, such
// as perhaps the Cocoa numbering scheme, which uses part of the Unicode E000-F8FF "private use
// area" to represent non-literal characters such as arrows.  It doesn't, however, seem to include
// modifier keys, so maybe xkeysyms would be better, even though it collides with Unicode.
//
// Currently, the best documentation of ADB key-codes is on page 2-43 of "Macintosh Toolbox
// Essentials", a PDF of which is available at http://developer.apple.com/legacy/mac/library/
// documentation/mac/pdf/MacintoshToolboxEssentials.pdf
class KeyEvent : public Event {
  public:
    KeyEvent(wall_time at, uint32_t key) : Event(at), _key(key) {}
    uint32_t key() const { return _key; }

  private:
    uint32_t _key;
};

// Generated when a key is pressed.
//
// This event has one fields:
//  * key(): the key that was pressed, as described in KeyEvent.
class KeyDownEvent : public KeyEvent {
  public:
    KeyDownEvent(wall_time at, uint32_t key) : KeyEvent(at, key) {}
    virtual void send(EventReceiver* receiver) const;
};

// Generated when a key is released.
//
// This event has one fields:
//  * key(): the key that was released, as described in KeyEvent.
class KeyUpEvent : public KeyEvent {
  public:
    KeyUpEvent(wall_time at, uint32_t key) : KeyEvent(at, key) {}
    virtual void send(EventReceiver* receiver) const;
};

class GamepadButtonEvent : public Event {
  public:
    GamepadButtonEvent(wall_time at, uint32_t button) : Event(at), button(button) {}
    const uint32_t button;
};

class GamepadButtonDownEvent : public GamepadButtonEvent {
  public:
    GamepadButtonDownEvent(wall_time at, uint32_t button) : GamepadButtonEvent(at, button) {}
    virtual void send(EventReceiver* receiver) const;
};

class GamepadButtonUpEvent : public GamepadButtonEvent {
  public:
    GamepadButtonUpEvent(wall_time at, uint32_t button) : GamepadButtonEvent(at, button) {}
    virtual void send(EventReceiver* receiver) const;
};

class GamepadStickEvent : public Event {
  public:
    GamepadStickEvent(wall_time at, int stick, double x, double y)
            : Event(at), stick(stick), x(x), y(y) {}
    virtual void send(EventReceiver* receiver) const;
    const int    stick;
    const double x;
    const double y;
};

// Superclass for events involving the mouse (moved, button press, button release).
//
// The location of the mouse event, `where()`, is described in coordinates from the upper-left
// corner of the screen.
class MouseEvent : public Event {
  public:
    MouseEvent(wall_time at, const Point& where) : Event(at), _where(where) {}
    const Point& where() const { return _where; }

  private:
    Point _where;
};

// Superclass for events generated by changes in button state (press, release).
//
// Buttons are numbered from 0 to 2, for the primary, secondary, and tertiary buttons (normally the
// left, right, and middle buttons).  On some systems, the scroll wheel is implemented as
// additional buttons above these, but when we support the scroll wheel, we will probably choose to
// implement it in terms of separate events instead.
class MouseButtonEvent : public MouseEvent {
  public:
    MouseButtonEvent(wall_time at, int button, const Point& where)
            : MouseEvent(at, where), _button(button) {}
    int button() const { return _button; }

  private:
    int _button;
};

// Generated when a mouse button is pressed.
//
// This event has three fields:
//  * button(): the mouse button that was pressed, as described in MouseButtonEvent.
//  * where(): the location of the mouse press, as described in MouseEvent.
//  * count(): the number of clicks, e.g. 2 for a double-click.
class MouseDownEvent : public MouseButtonEvent {
  public:
    MouseDownEvent(wall_time at, int button, int count, const Point& where)
            : MouseButtonEvent(at, button, where), _count(count) {}
    virtual void send(EventReceiver* receiver) const;
    int count() const { return _count; }

  private:
    int _count;
};

// Generated when a mouse button is released.
//
// This event has two fields:
//  * button(): the mouse button that was released, as described in MouseButtonEvent.
//  * where(): the location of the mouse release, as described in MouseEvent.
class MouseUpEvent : public MouseButtonEvent {
  public:
    MouseUpEvent(wall_time at, int button, const Point& where)
            : MouseButtonEvent(at, button, where) {}
    virtual void send(EventReceiver* receiver) const;
};

// Generated when the mouse moves.
//
// This event has two fields:
//  * where(): the new location of the mouse pointer.
class MouseMoveEvent : public MouseEvent {
  public:
    MouseMoveEvent(wall_time at, const Point& where) : MouseEvent(at, where) {}
    virtual void send(EventReceiver* receiver) const;
};

class EventReceiver {
  public:
    virtual ~EventReceiver();

    virtual void key_down(const KeyDownEvent& event);
    virtual void key_up(const KeyUpEvent& event);
    virtual void gamepad_button_down(const GamepadButtonDownEvent& event);
    virtual void gamepad_button_up(const GamepadButtonUpEvent& event);
    virtual void gamepad_stick(const GamepadStickEvent& event);
    virtual void mouse_down(const MouseDownEvent& event);
    virtual void mouse_up(const MouseUpEvent& event);
    virtual void mouse_move(const MouseMoveEvent& event);
};

}  // namespace antares

#endif  // ANTARES_UI_EVENT_HPP_
