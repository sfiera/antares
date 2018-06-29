// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2008-2018 The Antares Authors
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

#include "data/condition.hpp"

#include "data/action.hpp"
#include "data/field.hpp"
#include "data/initial.hpp"

namespace antares {

static ObjectRef required_object_ref(path_value x) {
    if (x.value().is_map()) {
        pn::map_cref m = x.value().as_map();
        if (m.size() != 1) {
            throw std::runtime_error(
                    pn::format("{0}must have single element", x.prefix()).c_str());
        }
        pn::key_value_cref kv = *m.begin();
        ObjectRef          o;
        if (kv.key() == "initial") {
            o.type = ObjectRef::Type::INITIAL;
        } else if (kv.key() == "flagship") {
            o.type = ObjectRef::Type::FLAGSHIP;
        } else {
            throw std::runtime_error(
                    pn::format("{0}key must be one of [\"initial\", \"flagship\"]", x.prefix())
                            .c_str());
        }
        switch (o.type) {
            case ObjectRef::Type::INITIAL: o.initial = required_initial(m.get("initial")); break;
            case ObjectRef::Type::FLAGSHIP:
                o.flagship = required_admiral(m.get("flagship"));
                break;
        }
        return o;
    } else {
        throw std::runtime_error(pn::format("{0}must be map", x.prefix()).c_str());
    }
}

static sfz::optional<ObjectRef> optional_object_ref(path_value x) {
    if (x.value().is_null()) {
        return sfz::nullopt;
    } else if (x.value().is_map()) {
        return sfz::make_optional(required_object_ref(x));
    } else {
        throw std::runtime_error(pn::format("{0}must be null or map", x.prefix()).c_str());
    }
}

// clang-format off
#define COMMON_CONDITION_FIELDS                                                                   \
            {"type", {&ConditionBase::type, required_condition_type}},                            \
            {"op", {&ConditionBase::op, required_condition_op}}
// clang-format on

Condition::When::Type Condition::When::type() const { return base.type; }

Condition::When::When() : base{} {}
Condition::When::When(AutopilotCondition a) : autopilot(std::move(a)) {}
Condition::When::When(BuildingCondition a) : building(std::move(a)) {}
Condition::When::When(ComputerCondition a) : computer(std::move(a)) {}
Condition::When::When(CounterCondition a) : counter(std::move(a)) {}
Condition::When::When(DestroyedCondition a) : destroyed(std::move(a)) {}
Condition::When::When(DistanceCondition a) : distance(std::move(a)) {}
Condition::When::When(HealthCondition a) : health(std::move(a)) {}
Condition::When::When(MessageCondition a) : message(std::move(a)) {}
Condition::When::When(OwnerCondition a) : owner(std::move(a)) {}
Condition::When::When(ShipsCondition a) : ships(std::move(a)) {}
Condition::When::When(SpeedCondition a) : speed(std::move(a)) {}
Condition::When::When(SubjectCondition a) : subject(std::move(a)) {}
Condition::When::When(TargetCondition a) : target(std::move(a)) {}
Condition::When::When(TimeCondition a) : time(std::move(a)) {}
Condition::When::When(ZoomCondition a) : zoom(std::move(a)) {}

Condition::When::When(When&& a) {
    switch (a.type()) {
        case Condition::When::Type::AUTOPILOT: new (this) When(std::move(a.autopilot)); break;
        case Condition::When::Type::BUILDING: new (this) When(std::move(a.building)); break;
        case Condition::When::Type::COMPUTER: new (this) When(std::move(a.computer)); break;
        case Condition::When::Type::COUNTER: new (this) When(std::move(a.counter)); break;
        case Condition::When::Type::DESTROYED: new (this) When(std::move(a.destroyed)); break;
        case Condition::When::Type::DISTANCE: new (this) When(std::move(a.distance)); break;
        case Condition::When::Type::HEALTH: new (this) When(std::move(a.health)); break;
        case Condition::When::Type::MESSAGE: new (this) When(std::move(a.message)); break;
        case Condition::When::Type::TARGET: new (this) When(std::move(a.target)); break;
        case Condition::When::Type::OWNER: new (this) When(std::move(a.owner)); break;
        case Condition::When::Type::SHIPS: new (this) When(std::move(a.ships)); break;
        case Condition::When::Type::SPEED: new (this) When(std::move(a.speed)); break;
        case Condition::When::Type::SUBJECT: new (this) When(std::move(a.subject)); break;
        case Condition::When::Type::TIME: new (this) When(std::move(a.time)); break;
        case Condition::When::Type::ZOOM: new (this) When(std::move(a.zoom)); break;
    }
}

Condition::When& Condition::When::operator=(When&& a) {
    this->~When();
    new (this) When(std::move(a));
    return *this;
}

Condition::When::~When() {
    switch (type()) {
        case Condition::When::Type::AUTOPILOT: autopilot.~AutopilotCondition(); break;
        case Condition::When::Type::BUILDING: building.~BuildingCondition(); break;
        case Condition::When::Type::COMPUTER: computer.~ComputerCondition(); break;
        case Condition::When::Type::COUNTER: counter.~CounterCondition(); break;
        case Condition::When::Type::DESTROYED: destroyed.~DestroyedCondition(); break;
        case Condition::When::Type::DISTANCE: distance.~DistanceCondition(); break;
        case Condition::When::Type::HEALTH: health.~HealthCondition(); break;
        case Condition::When::Type::MESSAGE: message.~MessageCondition(); break;
        case Condition::When::Type::OWNER: owner.~OwnerCondition(); break;
        case Condition::When::Type::SHIPS: ships.~ShipsCondition(); break;
        case Condition::When::Type::SPEED: speed.~SpeedCondition(); break;
        case Condition::When::Type::SUBJECT: subject.~SubjectCondition(); break;
        case Condition::When::Type::TARGET: target.~TargetCondition(); break;
        case Condition::When::Type::TIME: time.~TimeCondition(); break;
        case Condition::When::Type::ZOOM: zoom.~ZoomCondition(); break;
    }
}

static Condition::When::Type required_condition_type(path_value x) {
    return required_enum<Condition::When::Type>(
            x, {{"autopilot", Condition::When::Type::AUTOPILOT},
                {"building", Condition::When::Type::BUILDING},
                {"computer", Condition::When::Type::COMPUTER},
                {"counter", Condition::When::Type::COUNTER},
                {"destroyed", Condition::When::Type::DESTROYED},
                {"distance", Condition::When::Type::DISTANCE},
                {"health", Condition::When::Type::HEALTH},
                {"message", Condition::When::Type::MESSAGE},
                {"owner", Condition::When::Type::OWNER},
                {"ships", Condition::When::Type::SHIPS},
                {"speed", Condition::When::Type::SPEED},
                {"subject", Condition::When::Type::SUBJECT},
                {"target", Condition::When::Type::TARGET},
                {"time", Condition::When::Type::TIME},
                {"zoom", Condition::When::Type::ZOOM}});
}

static ConditionOp required_condition_op(path_value x) {
    return required_enum<ConditionOp>(
            x, {{"eq", ConditionOp::EQ},
                {"ne", ConditionOp::NE},
                {"lt", ConditionOp::LT},
                {"gt", ConditionOp::GT},
                {"le", ConditionOp::LE},
                {"ge", ConditionOp::GE}});
}

static Condition::When autopilot_condition(path_value x) {
    return required_struct<AutopilotCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"player", {&AutopilotCondition::player, required_admiral}},
                {"value", {&AutopilotCondition::value, required_bool}}});
}

static Condition::When building_condition(path_value x) {
    return required_struct<BuildingCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"player", {&BuildingCondition::player, required_admiral}},
                {"value", {&BuildingCondition::value, required_bool}}});
}

static Condition::When computer_condition(path_value x) {
    return required_struct<ComputerCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"player", nullptr},
                {"screen", {&ComputerCondition::screen, required_screen}},
                {"line", {&ComputerCondition::line, optional_int}}});
}

static Condition::When counter_condition(path_value x) {
    return required_struct<CounterCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"player", {&CounterCondition::player, required_admiral}},
                {"counter", {&CounterCondition::counter, required_int}},
                {"value", {&CounterCondition::value, required_int}}});
}

static Condition::When destroyed_condition(path_value x) {
    return required_struct<DestroyedCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"initial", {&DestroyedCondition::initial, required_object_ref}},
                {"value", {&DestroyedCondition::value, required_bool}}});
}

static Condition::When distance_condition(path_value x) {
    return required_struct<DistanceCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"value", {&DistanceCondition::value, required_int}},
                {"from", {&DistanceCondition::from, required_object_ref}},
                {"to", {&DistanceCondition::to, required_object_ref}}});
}

static Condition::When health_condition(path_value x) {
    return required_struct<HealthCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"value", {&HealthCondition::value, required_double}},
                {"initial", {&HealthCondition::initial, required_object_ref}}});
}

static Condition::When message_condition(path_value x) {
    return required_struct<MessageCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"player", nullptr},
                {"id", {&MessageCondition::id, required_int}},
                {"page", {&MessageCondition::page, required_int}}});
}

static Condition::When owner_condition(path_value x) {
    return required_struct<OwnerCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"player", {&OwnerCondition::player, required_admiral}},
                {"initial", {&OwnerCondition::initial, required_object_ref}}});
}

static Condition::When ships_condition(path_value x) {
    return required_struct<ShipsCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"player", {&ShipsCondition::player, required_admiral}},
                {"value", {&ShipsCondition::value, required_int}}});
}

static Condition::When speed_condition(path_value x) {
    return required_struct<SpeedCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"value", {&SpeedCondition::value, required_fixed}},
                {"initial", {&SpeedCondition::initial, required_object_ref}}});
}

static SubjectCondition::Value required_subject_value(path_value x) {
    return required_enum<SubjectCondition::Value>(
            x, {{"control", SubjectCondition::Value::CONTROL},
                {"target", SubjectCondition::Value::TARGET},
                {"flagship", SubjectCondition::Value::FLAGSHIP}});
}

static Condition::When subject_condition(path_value x) {
    return required_struct<SubjectCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"initial", {&SubjectCondition::initial, required_object_ref}},
                {"player", {&SubjectCondition::player, required_admiral}},
                {"value", {&SubjectCondition::value, required_subject_value}}});
}

static Condition::When target_condition(path_value x) {
    return required_struct<TargetCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"initial", {&TargetCondition::initial, required_object_ref}},
                {"target", {&TargetCondition::target, required_object_ref}}});
}

static Condition::When time_condition(path_value x) {
    return required_struct<TimeCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"duration", {&TimeCondition::duration, required_ticks}},
                {"legacy_start_time", {&TimeCondition::legacy_start_time, optional_bool, false}}});
}

static Condition::When zoom_condition(path_value x) {
    return required_struct<ZoomCondition>(
            x, {COMMON_CONDITION_FIELDS,
                {"player", nullptr},
                {"value", {&ZoomCondition::value, required_zoom}}});
}

static Condition::When when(path_value x) {
    if (!x.value().is_map()) {
        throw std::runtime_error(pn::format("{0}: must be map", x.path()).c_str());
    }

    switch (required_condition_type(x.get("type"))) {
        case Condition::When::Type::AUTOPILOT: return autopilot_condition(x);
        case Condition::When::Type::BUILDING: return building_condition(x);
        case Condition::When::Type::COMPUTER: return computer_condition(x);
        case Condition::When::Type::COUNTER: return counter_condition(x);
        case Condition::When::Type::DESTROYED: return destroyed_condition(x);
        case Condition::When::Type::DISTANCE: return distance_condition(x);
        case Condition::When::Type::HEALTH: return health_condition(x);
        case Condition::When::Type::MESSAGE: return message_condition(x);
        case Condition::When::Type::TARGET: return target_condition(x);
        case Condition::When::Type::OWNER: return owner_condition(x);
        case Condition::When::Type::SHIPS: return ships_condition(x);
        case Condition::When::Type::SPEED: return speed_condition(x);
        case Condition::When::Type::SUBJECT: return subject_condition(x);
        case Condition::When::Type::TIME: return time_condition(x);
        case Condition::When::Type::ZOOM: return zoom_condition(x);
    }
}

Condition condition(path_value x) {
    return required_struct<Condition>(
            x, {{"persistent", {&Condition::persistent, optional_bool, false}},
                {"disabled", {&Condition::disabled, optional_bool, false}},
                {"when", {&Condition::when, when}},
                {"subject", {&Condition::subject, optional_object_ref}},
                {"object", {&Condition::object, optional_object_ref}},
                {"action", {&Condition::action, required_array<Action, action>}}});
}

}  // namespace antares
