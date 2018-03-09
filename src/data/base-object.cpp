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

// Space Object Handling >> MUST BE INITED _AFTER_ SCENARIOMAKER << (uses Ares Scenarios file)

#include "data/base-object.hpp"

#include "data/level.hpp"

namespace antares {

bool read_from(pn::file_view in, BaseObject* object) {
    int32_t  offense, max_velocity, warp_speed, mass, max_thrust;
    int32_t  initial_age, age_range, initial_velocity, initial_velocity_range;
    int32_t  initial_direction, initial_direction_range, icon;
    pn::data frame;
    frame.resize(32);
    int32_t  friend_deficit, danger_threshold, build_ratio;
    uint32_t build_time;
    if (!in.read(
                &object->attributes, &object->baseClass, pn::pad(4), &object->price, &offense,
                &object->destinationClass, &max_velocity, &warp_speed, &object->warpOutDistance,
                &initial_velocity, &initial_velocity_range, &mass, &max_thrust, &object->health,
                &object->damage, &object->energy, &initial_age, &age_range, &object->naturalScale,
                &object->pixLayer, &object->pixResID, &icon, &object->shieldColor, pn::pad(1),
                &initial_direction, &initial_direction_range, pn::pad(96), &friend_deficit,
                &danger_threshold, &object->specialDirection, &object->arriveActionDistance,
                pn::pad(48), &frame, &object->buildFlags, &object->orderFlags, &build_ratio,
                &build_time, &object->skillNum, &object->skillDen, &object->skillNumAdj,
                &object->skillDenAdj, &object->pictPortraitResID, pn::pad(10))) {
        return false;
    }

    if (object->attributes & kIsSelfAnimated) {
        object->attributes &= ~(kShapeFromDirection | kIsVector);
    } else if (object->attributes & kShapeFromDirection) {
        object->attributes &= ~kIsVector;
    }

    object->offenseValue     = Fixed::from_val(offense);
    object->maxVelocity      = Fixed::from_val(max_velocity);
    object->warpSpeed        = Fixed::from_val(warp_speed);
    object->mass             = Fixed::from_val(mass);
    object->maxThrust        = Fixed::from_val(max_thrust);
    object->initial_velocity = {
            Fixed::from_val(initial_velocity),
            Fixed::from_val(initial_velocity + std::max(0, initial_velocity_range))};
    if (initial_age >= 0) {
        object->initial_age =
                Range<ticks>{ticks(initial_age), ticks(initial_age + std::max(0, age_range))};
    } else {
        object->initial_age = {ticks(-1), ticks(-1)};
    }

    if (object->attributes & kNeutralDeath) {
        object->occupy_count = age_range;
    } else {
        object->occupy_count = -1;
    }

    object->initial_direction = {initial_direction,
                                 initial_direction + std::max(0, initial_direction_range)};
    if ((0 < icon) && (icon < 0x50)) {
        object->icon.size = icon & 0x0F;
        switch (icon & 0xF0) {
            case 0x00: object->icon.shape = IconShape::SQUARE; break;
            case 0x10: object->icon.shape = IconShape::TRIANGLE; break;
            case 0x20: object->icon.shape = IconShape::DIAMOND; break;
            case 0x30: object->icon.shape = IconShape::PLUS; break;
            case 0x40: object->icon.shape = IconShape::SQUARE; break;
        }
    } else {
        object->icon = {IconShape::SQUARE, 0};
    }

    if ((object->shieldColor != 0xFF) && (object->shieldColor != 0)) {
        object->shieldColor = GetTranslateColorShade(static_cast<Hue>(object->shieldColor), 15);
    }

    object->friendDefecit   = Fixed::from_val(friend_deficit);
    object->dangerThreshold = Fixed::from_val(danger_threshold);
    object->buildRatio      = Fixed::from_val(build_ratio);
    object->buildTime       = 3 * ticks(build_time / 10);

    static const char hex[]      = "0123456789abcdef";
    int               level_tag  = (object->buildFlags & kLevelKeyTag) >> kLevelKeyTagShift;
    int               engage_tag = (object->buildFlags & kEngageKeyTag) >> kEngageKeyTagShift;
    int               order_tag  = (object->orderFlags & kOrderKeyTag) >> kOrderKeyTagShift;
    object->levelKeyTag          = level_tag ? pn::rune(hex[level_tag]).copy() : "";
    object->engageKeyTag         = engage_tag ? pn::rune(hex[engage_tag]).copy() : "";
    object->orderKeyTag          = order_tag ? pn::rune(hex[order_tag]).copy() : "";

    if (object->attributes & kShapeFromDirection) {
        read_from(frame.open(), &object->frame.rotation);
    } else if (object->attributes & kIsSelfAnimated) {
        read_from(frame.open(), &object->frame.animation);
    } else if (object->attributes & kIsVector) {
        read_from(frame.open(), &object->frame.vector);
    } else {
        read_from(frame.open(), &object->frame.weapon);
    }
    return true;
}

bool read_from(pn::file_view in, objectFrameType::Rotation* rotation) {
    int32_t turn_rate;
    if (!in.read(&rotation->shapeOffset, &rotation->rotRes, &turn_rate, pn::pad(4))) {
        return false;
    }
    rotation->maxTurnRate = Fixed::from_val(turn_rate);
    return true;
}

bool read_from(pn::file_view in, objectFrameType::Animation* animation) {
    int32_t begin, end, dir, dir_range, speed, first, first_range;
    if (!in.read(&begin, &end, &dir, &dir_range, &speed, pn::pad(4), &first, &first_range)) {
        return false;
    }

    // TODO(sfiera): use Fixed::from_long(1) instead of Fixed::from_val(1). This causes
    // arescentral/antares#163.
    animation->shapes = {Fixed::from_long(begin), Fixed::from_long(end) + Fixed::from_val(1)};
    animation->speed  = Fixed::from_val(speed);
    animation->first  = {Fixed::from_long(first), Fixed::from_long(first + first_range)};

    if ((dir == 0) && (dir_range == 0)) {
        animation->direction = AnimationDirection::NONE;
    } else if ((dir == +1) && (dir_range == 0)) {
        animation->direction = AnimationDirection::PLUS;
    } else if ((dir == -1) && (dir_range == 0)) {
        animation->direction = AnimationDirection::MINUS;
    } else if ((dir == -1) && (dir_range == -1)) {
        animation->direction = AnimationDirection::RANDOM;
    } else {
        animation->direction = AnimationDirection::NONE;
    }

    return true;
}

bool read_from(pn::file_view in, objectFrameType::Vector* vector) {
    uint8_t kind, color;
    if (!in.read(&color, &kind, &vector->accuracy, &vector->range)) {
        return false;
    }
    switch (kind) {
        default: vector->kind = VectorKind::BOLT; break;
        case 1: vector->kind = VectorKind::BEAM_TO_OBJECT; break;
        case 2: vector->kind = VectorKind::BEAM_TO_COORD; break;
        case 3: vector->kind = VectorKind::BEAM_TO_OBJECT_LIGHTNING; break;
        case 4: vector->kind = VectorKind::BEAM_TO_COORD_LIGHTNING; break;
    }
    if (color <= 16) {
        vector->visible = 0;
    } else if (vector->kind == VectorKind::BOLT) {
        vector->visible    = true;
        vector->bolt_color = GetRGBTranslateColor(color);
    } else {
        vector->visible  = true;
        vector->beam_hue = static_cast<Hue>(color >> 4);
    }
    return true;
}

bool read_from(pn::file_view in, objectFrameType::Weapon* weapon) {
    int32_t fire_time;
    if (!(in.read(&weapon->usage, &weapon->energyCost, &fire_time, &weapon->ammo,
                  &weapon->range) &&
          read_from(in, &weapon->inverseSpeed) && in.read(&weapon->restockCost))) {
        return false;
    }
    weapon->fireTime = ticks(fire_time);
    return true;
}

fixedPointType required_fixed_point(path_value x) {
    if (x.value().is_map()) {
        Fixed px = required_fixed(x.get("x"));
        Fixed py = required_fixed(x.get("y"));
        return {px, py};
    } else {
        throw std::runtime_error(pn::format("{0}: must be map", x.path()).c_str());
    }
}

sfz::optional<std::vector<fixedPointType>> optional_fixed_point_array(path_value x) {
    if (x.value().is_null()) {
        return sfz::nullopt;
    } else if (x.value().is_array()) {
        pn::array_cref              a = x.value().as_array();
        std::vector<fixedPointType> result;
        for (int i = 0; i < a.size(); ++i) {
            result.emplace_back(required_fixed_point(x.get(i)));
        }
        return sfz::make_optional(std::move(result));
    } else {
        throw std::runtime_error(pn::format("{0}: must be null or array", x.path()).c_str());
    }
}

sfz::optional<BaseObject::Weapon> optional_weapon(path_value x) {
    if (x.value().is_null()) {
        return sfz::nullopt;
    } else if (x.value().is_map()) {
        BaseObject::Weapon w;
        w.base      = required_base(x.get("base"));
        w.positions = optional_fixed_point_array(x.get("positions"))
                              .value_or(std::vector<fixedPointType>{});
        if (w.positions.empty()) {
            w.positions.push_back({Fixed::zero(), Fixed::zero()});
        }
        return sfz::make_optional(std::move(w));
    } else {
        throw std::runtime_error(pn::format("{0}: must be null or map", x.path()).c_str());
    }
}

BaseObject base_object(pn::value_cref x0) {
    if (!x0.is_map()) {
        throw std::runtime_error("must be map");
    }

    path_value x{x0};
    BaseObject o;
    if (!read_from(x.get("bin").value().as_data().open(), &o)) {
        throw std::runtime_error("read failure");
    }

    o.name       = required_string(x.get("long_name")).copy();
    o.short_name = required_string(x.get("short_name")).copy();

    o.race = optional_race(x.get("race")).value_or(Race::none());

    o.destroy = optional_action_array(x.get("on_destroy"))
                        .value_or(std::vector<std::unique_ptr<const Action>>{});
    o.expire = optional_action_array(x.get("on_expire"))
                       .value_or(std::vector<std::unique_ptr<const Action>>{});
    o.create = optional_action_array(x.get("on_create"))
                       .value_or(std::vector<std::unique_ptr<const Action>>{});
    o.collide = optional_action_array(x.get("on_collide"))
                        .value_or(std::vector<std::unique_ptr<const Action>>{});
    o.activate = optional_action_array(x.get("on_activate"))
                         .value_or(std::vector<std::unique_ptr<const Action>>{});
    o.arrive = optional_action_array(x.get("on_arrive"))
                       .value_or(std::vector<std::unique_ptr<const Action>>{});

    auto weapons = x.get("weapons");
    if (weapons.value().is_null()) {
        // no weapons
    } else if (weapons.value().is_map()) {
        o.pulse = optional_weapon(weapons.get("pulse"))
                          .value_or(BaseObject::Weapon{BaseObject::none()});
        o.beam = optional_weapon(weapons.get("beam"))
                         .value_or(BaseObject::Weapon{BaseObject::none()});
        o.special = optional_weapon(weapons.get("special"))
                            .value_or(BaseObject::Weapon{BaseObject::none()});
    } else {
        throw std::runtime_error(pn::format("{0}: must be null or map", weapons.path()).c_str());
    }

    o.destroyDontDie  = optional_bool(x.get("destroy_dont_die")).value_or(false);
    o.expireDontDie   = optional_bool(x.get("expire_dont_die")).value_or(false);
    o.activate_period = optional_ticks_range(x.get("activate_period"))
                                .value_or(Range<ticks>{ticks(0), ticks(0)});

    return o;
}

}  // namespace antares
