// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2008-2012 The Antares Authors
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

#include "game/admiral.hpp"

#include "data/space-object.hpp"
#include "data/string-list.hpp"
#include "game/cheat.hpp"
#include "game/globals.hpp"
#include "game/space-object.hpp"
#include "lang/casts.hpp"
#include "math/macros.hpp"
#include "math/random.hpp"
#include "math/units.hpp"
#include "sound/fx.hpp"

using sfz::Bytes;
using sfz::Exception;
using sfz::String;
using sfz::StringSlice;
using std::min;
using std::unique_ptr;

namespace antares {

namespace {

const int32_t kNoFreeAdmiral            = -1;
const int32_t kDestNoObject             = -1;

const int32_t kDestinationNameLen        = 17;
const int32_t kAdmiralNameLen            = 31;

const Fixed kUnimportantTarget          = 0x00000000;
const Fixed kMostImportantTarget        = 0x00000200;
const Fixed kLeastImportantTarget       = 0x00000100;
const Fixed kVeryImportantTarget        = 0x00000160;
const Fixed kImportantTarget            = 0x00000140;
const Fixed kSomewhatImportantTarget    = 0x00000120;
const Fixed kAbsolutelyEssential        = 0x00008000;

unique_ptr<Destination[]> gDestBalanceData;
unique_ptr<Admiral[]> gAdmiralData;

}  // namespace

void Admiral::init() {
    gAdmiralData.reset(new Admiral[kMaxPlayerNum]);
    reset();
    gDestBalanceData.reset(new Destination[kMaxDestObject]);
    ResetAllDestObjectData();
}

void Admiral::reset() {
    for (auto a: Admiral::all()) {
        *a = Admiral();
    }
}

void ResetAllDestObjectData() {
    for (auto d: Destination::all()) {
        d->whichObject = SpaceObject::none();
        d->name.clear();
        d->earn = d->totalBuildTime = d->buildTime = 0;
        d->buildObjectBaseNum = BaseObject::none();
        for (int j = 0; j < kMaxTypeBaseCanBuild; ++j) {
            d->canBuildType[j] = kNoShip;
        }
        for (int j = 0; j < kMaxPlayerNum; ++j) {
            d->occupied[j] = 0;
        }
    }
}

Destination* Destination::get(int i) {
    if ((0 <= i) && (i < kMaxDestObject)) {
        return &gDestBalanceData[i];
    }
    return nullptr;
}

bool Destination::can_build() const {
    for (int i = 0; i < kMaxShipCanBuild; ++i) {
        if (canBuildType[i] != kNoShip) {
            return true;
        }
    }
    return false;
}

Admiral* Admiral::get(int i) {
    if ((0 <= i) && (i < kMaxPlayerNum)) {
        return &gAdmiralData[i];
    }
    return nullptr;
}

Handle<Admiral> Admiral::make(int index, uint32_t attributes, const Scenario::Player& player) {
    Handle<Admiral> a(index);
    if (a->_active) {
        return none();
    }

    a->_active = true;
    a->_attributes = attributes;
    a->_earning_power = player.earningPower;
    a->_race = player.playerRace;
    if ((player.nameResID >= 0)) {
        a->_name.assign(StringList(player.nameResID).at(player.nameStrNum - 1));
        if (a->_name.size() > kAdmiralNameLen) {
            a->_name.resize(kAdmiralNameLen);
        }
    }

    // for now set strategy balance to 0 -- we may want to calc this if player added on the fly?
    return a;
}

static Handle<Destination> next_free_destination() {
    for (auto d: Destination::all()) {
        if (!d->whichObject.get()) {
            return d;
        }
    }
    return Destination::none();
}

Handle<Destination> MakeNewDestination(
        Handle<SpaceObject> object, int32_t* canBuildType, Fixed earn, int16_t nameResID,
        int16_t nameStrNum) {
    auto d = next_free_destination();
    if (!d.get()) {
        return Destination::none();
    }

    d->whichObject = object;
    d->earn = earn;
    d->totalBuildTime = d->buildTime = 0;

    if (canBuildType != NULL) {
        for (int j = 0; j < kMaxTypeBaseCanBuild; j++) {
            d->canBuildType[j] = *canBuildType;
            canBuildType++;
        }
    } else {
        for (int j = 0; j < kMaxTypeBaseCanBuild; j++) {
            d->canBuildType[j] = kNoShip;
        }
    }

    if ((nameResID >= 0)) {
        d->name.assign(StringList(nameResID).at(nameStrNum - 1));
        if (d->name.size() > kDestinationNameLen) {
            d->name.resize(kDestinationNameLen);
        }
    }

    if (object->attributes & kNeutralDeath) {
        for (int j = 0; j < kMaxPlayerNum; j++) {
            d->occupied[j] = 0;
        }

        if (object->owner.get()) {
            d->occupied[object->owner.number()] = object->baseType->initialAgeRange;
        }
    }

    return d;
}

void Admiral::remove_destination(Handle<Destination> d) {
    if (_active) {
        if (_destinationObject == d->whichObject) {
            _destinationObject = SpaceObject::none();
            _destinationObjectID = -1;
            _has_destination = false;
        }
        if (_considerDestination == d.number()) {
            _considerDestination = kNoDestinationObject;
        }

        if (_buildAtObject == d) {
            _buildAtObject = Destination::none();
        }
    }
}

void RemoveDestination(Handle<Destination> d) {
    if (!d.get()) {
        return;
    }
    for (auto a: Admiral::all()) {
        a->remove_destination(d);
    }

    d->whichObject = SpaceObject::none();
    d->name.clear();
    d->earn = d->totalBuildTime = d->buildTime = 0;
    d->buildObjectBaseNum = BaseObject::none();
    for (int i = 0; i < kMaxTypeBaseCanBuild; i++) {
        d->canBuildType[i] = kNoShip;
    }

    for (int i = 0; i < kMaxPlayerNum; i++) {
        d->occupied[i] = 0;
    }
}

void RecalcAllAdmiralBuildData() {
    // first clear all the data
    for (auto a: Admiral::all()) {
        for (int j = 0; j < kMaxNumAdmiralCanBuild; j++) {
            a->canBuildType()[j].baseNum = -1;
            a->canBuildType()[j].base = BaseObject::none();
            a->canBuildType()[j].chanceRange = -1;
        }
        a->totalBuildChance() = 0;
        a->hopeToBuild() = -1;
    }

    for (auto d: Destination::all()) {
        if (d->whichObject.get()) {
            auto anObject = d->whichObject;
            if (anObject->owner.get()) {
                const auto& a = anObject->owner;
                for (int k = 0; k < kMaxTypeBaseCanBuild; k++) {
                    if (d->canBuildType[k] >= 0) {
                        int j = 0;
                        while ((a->canBuildType()[j].baseNum != d->canBuildType[k])
                                && (j < kMaxNumAdmiralCanBuild)) {
                            j++;
                        }
                        if (j == kMaxNumAdmiralCanBuild) {
                            auto baseObject = mGetBaseObjectFromClassRace(d->canBuildType[k], a->race());
                            j = 0;
                            while ((a->canBuildType()[j].baseNum != -1)
                                    && (j < kMaxNumAdmiralCanBuild)) {
                                j++;
                            }
                            if (j == kMaxNumAdmiralCanBuild) {
                                throw Exception("Too Many Types to Build!");
                            }
                            a->canBuildType()[j].baseNum = d->canBuildType[k];
                            a->canBuildType()[j].base = baseObject;
                            a->canBuildType()[j].chanceRange = a->totalBuildChance();
                            if (baseObject.get()) {
                                a->totalBuildChance() += baseObject->buildRatio;
                            }
                        }
                    }
                }
            }
        }
    }
}

uint8_t GetAdmiralColor(Handle<Admiral> a) {
    if (!a.get()) {
        return 0;
    }
    return a->color();
}

int32_t GetAdmiralRace(Handle<Admiral> a) {
    if (!a.get()) {
        return -1;
    }
    return a->race();
}

Handle<SpaceObject> Admiral::flagship() {
    if (_flagship.get()) {
        if (_flagship->id == _flagshipID) {
            return _flagship;
        }
    }
    return SpaceObject::none();
}

void Admiral::set_flagship(Handle<SpaceObject> object) {
    _flagship = object;
    if (object.get()) {
        _flagshipID = object->id;
    } else {
        _flagshipID = -1;
    }
}

void Admiral::set_target(Handle<SpaceObject> obj) {
    _destinationObject = obj;
    if (obj.get()) {
        _destinationObjectID = obj->id;
    } else {
        _destinationObjectID = -1;
    }
    _has_destination = true;
}

Handle<SpaceObject> Admiral::target() const {
    if (_destinationObject.get()
            && (_destinationObject->id == _destinationObjectID)
            && (_destinationObject->active == kObjectInUse)) {
        return _destinationObject;
    }
    return SpaceObject::none();
}

void Admiral::set_control(Handle<SpaceObject> obj) {
    _considerShip = obj;
    if (obj.get()) {
        _considerShipID = obj->id;
        if (obj->attributes & kCanAcceptBuild) {
            auto d = obj->asDestination;
            if (d.get() && d->can_build()) {
                _buildAtObject = d;
            }
        }
    } else {
        _considerShipID = -1;
    }
}

Handle<SpaceObject> Admiral::control() const {
    if (_considerShip.get()
            && (_considerShip->id == _considerShipID)
            && (_considerShip->active == kObjectInUse)
            && (_considerShip->owner.get() == this)) {
        return _considerShip;
    }
    return SpaceObject::none();
}

bool BaseHasSomethingToBuild(Handle<SpaceObject> obj) {
    if (obj->attributes & kCanAcceptBuild) {
        auto d = obj->asDestination;
        return d.get() && d->can_build();
    }
    return false;
}

Handle<Destination> GetAdmiralBuildAtObject(Handle<Admiral> a) {
    auto destBalance = a->buildAtObject();
    if (destBalance.get()) {
        if (destBalance->whichObject.get()) {
            auto anObject = destBalance->whichObject;
            if (anObject->owner != a) {
                a->buildAtObject() = Destination::none();
            }
        } else {
            a->buildAtObject() = Destination::none();
        }
    }
    return a->buildAtObject();
}

void SetAdmiralBuildAtObject(Handle<Admiral> a, Handle<SpaceObject> obj) {
    if (!a.get()) {
        throw Exception("Can't set consider ship for -1 admiral.");
    }
    if (obj.get()) {
        if (obj->attributes & kCanAcceptBuild) {
            auto d = obj->asDestination;
            if (d.get() && d->can_build()) {
                a->buildAtObject() = d;
            }
        }
    }
}

void SetAdmiralBuildAtName(Handle<Admiral> a, StringSlice name) {
    auto destObject = a->buildAtObject();
    destObject->name.assign(name.slice(0, min<size_t>(name.size(), kDestinationNameLen)));
}

StringSlice GetDestBalanceName(Handle<Destination> whichDestObject) {
    return whichDestObject->name;
}

StringSlice GetAdmiralName(Handle<Admiral> a) {
    if (a.get()) {
        return a->name();
    } else {
        return NULL;
    }
}

void SetObjectLocationDestination(Handle<SpaceObject> o, coordPointType *where) {
    // if the object does not have an alliance, then something is wrong here--forget it
    if (o->owner.number() <= kNoOwner) {
        o->destObject = SpaceObject::none();
        o->destObjectDest = SpaceObject::none();
        o->destObjectID = -1;
        o->destinationLocation.h = o->destinationLocation.v = kNoDestinationCoord;
        o->timeFromOrigin = 0;
        o->idealLocationCalc.h = o->idealLocationCalc.v = 0;
        o->originLocation = o->location;
        return;
    }

    // if this object can't accept a destination, then forget it
    if (!(o->attributes & kCanAcceptDestination)) {
        return;
    }

    // if this object has a locked destination, then forget it
    if (o->attributes & kStaticDestination) {
        return;
    }

    // if the owner is not legal, something is very very wrong
    if (!o->owner.get()) {
        return;
    }

    const auto& a = o->owner;

    // if the admiral is not legal, or the admiral has no destination, then forget about it
    if (!a->active()) {
        o->destObject = SpaceObject::none();
        o->destObjectDest = SpaceObject::none();
        o->destinationLocation.h = o->destinationLocation.v = kNoDestinationCoord;
        o->timeFromOrigin = 0;
        o->idealLocationCalc.h = o->idealLocationCalc.v = 0;
        o->originLocation = o->location;
    } else {
        // the object is OK, the admiral is OK, then go about setting its destination
        if (o->attributes & kCanAcceptDestination) {
            o->timeFromOrigin = kTimeToCheckHome;
        } else {
            o->timeFromOrigin = 0;
        }

        // remove this object from its destination
        if (o->destObject.get()) {
            RemoveObjectFromDestination(o);
        }

        o->destinationLocation = o->originLocation = *where;
        o->destObject = SpaceObject::none();
        o->timeFromOrigin = 0;
        o->idealLocationCalc.h = o->idealLocationCalc.v = 0;
    }
}

void SetObjectDestination(Handle<SpaceObject> o, Handle<SpaceObject> overrideObject) {
    auto dObject = overrideObject;

    // if the object does not have an alliance, then something is wrong here--forget it
    if (o->owner.number() <= kNoOwner) {
        o->destObject = SpaceObject::none();
        o->destObjectDest = SpaceObject::none();
        o->destObjectID = -1;
        o->destinationLocation.h = o->destinationLocation.v = kNoDestinationCoord;
        o->timeFromOrigin = 0;
        o->idealLocationCalc.h = o->idealLocationCalc.v = 0;
        o->originLocation = o->location;
        return;
    }

    // if this object can't accept a destination, then forget it
    if (!(o->attributes & kCanAcceptDestination)) {
        return;
    }

    // if this object has a locked destination, then forget it
    if ((o->attributes & kStaticDestination) && !overrideObject.get()) {
        return;
    }

    // if the owner is not legal, something is very very wrong
    if (!o->owner.get()) {
        return;
    }

    // get the admiral
    const auto& a = o->owner;

    // if the admiral is not legal, or the admiral has no destination, then forget about it
    if (!dObject.get() &&
            ((!a->active())
             || !a->has_destination()
             || !a->destinationObject().get()
             || (a->destinationObjectID() == o->id))) {
        o->destObject = SpaceObject::none();
        o->destObjectDest = SpaceObject::none();
        o->destinationLocation.h = o->destinationLocation.v = kNoDestinationCoord;
        o->timeFromOrigin = 0;
        o->idealLocationCalc.h = o->idealLocationCalc.v = 0;
        o->originLocation = o->location;
    } else {
        // the object is OK, the admiral is OK, then go about setting its destination

        // first make sure we're still looking at the same object
        if (!dObject.get()) {
            dObject = a->destinationObject();
        }

        if ((dObject->active == kObjectInUse) &&
                ((dObject->id == a->destinationObjectID())
                 || overrideObject.get())) {
            if (o->attributes & kCanAcceptDestination) {
                o->timeFromOrigin = kTimeToCheckHome;
            } else {
                o->timeFromOrigin = 0;
            }
            // remove this object from its destination
            if (o->destObject.get()) {
                RemoveObjectFromDestination(o);
            }

            // add this object to its destination
            if (o != dObject) {
                o->runTimeFlags &= ~kHasArrived;
                o->destObject = dObject;
                o->destObjectDest = dObject->destObject;
                o->destObjectDestID = dObject->destObjectID;
                o->destObjectID = dObject->id;

                if (dObject->owner == o->owner) {
                    dObject->remoteFriendStrength += o->baseType->offenseValue;
                    dObject->escortStrength += o->baseType->offenseValue;
                    if (dObject->attributes & kIsDestination) {
                        if (dObject->escortStrength < dObject->baseType->friendDefecit) {
                            o->duty = eGuardDuty;
                        } else {
                            o->duty = eNoDuty;
                        }
                    } else {
                        if (dObject->escortStrength < dObject->baseType->friendDefecit) {
                            o->duty = eEscortDuty;
                        } else {
                            o->duty = eNoDuty;
                        }
                    }
                } else {
                    dObject->remoteFoeStrength += o->baseType->offenseValue;
                    if (dObject->attributes & kIsDestination) {
                        o->duty = eAssaultDuty;
                    } else {
                        o->duty = eAssaultDuty;
                    }
                }
            } else {
                o->destObject = SpaceObject::none();
                o->destObjectDest = SpaceObject::none();
                o->destinationLocation.h = o->destinationLocation.v = kNoDestinationCoord;
                o->timeFromOrigin = 0;
                o->idealLocationCalc.h = o->idealLocationCalc.v = 0;
                o->originLocation = o->location;
            }
        } else {
            o->destObject = SpaceObject::none();
            o->destObjectDest = SpaceObject::none();
            o->destinationLocation.h = o->destinationLocation.v = kNoDestinationCoord;
            o->timeFromOrigin = 0;
            o->idealLocationCalc.h = o->idealLocationCalc.v = 0;
            o->originLocation = o->location;
        }
    }
}

void RemoveObjectFromDestination(Handle<SpaceObject> o) {
    if (o->destObject.get()) {
        auto dObject = o->destObject;
        if (dObject->id == o->destObjectID) {
            if (dObject->owner == o->owner) {
                dObject->remoteFriendStrength -= o->baseType->offenseValue;
                dObject->escortStrength -= o->baseType->offenseValue;
            } else {
                dObject->remoteFoeStrength -= o->baseType->offenseValue;
            }
        }
    }

    o->destObject = SpaceObject::none();
    o->destObjectDest = SpaceObject::none();
    o->destObjectID = -1;
}

// assumes you can afford it & base has time
static void AdmiralBuildAtObject(
        Handle<Admiral> admiral, Handle<BaseObject> base, Handle<Destination> buildAtDest) {
    fixedPointType  v = {0, 0};
    if (base.get()) {
        auto coord = buildAtDest->whichObject->location;

        auto newObject = CreateAnySpaceObject(base, &v, &coord, 0, admiral, 0, -1);
        if (newObject.get()) {
            SetObjectDestination(newObject, SpaceObject::none());
            if (admiral == g.admiral) {
                PlayVolumeSound(kComputerBeep2, kMediumVolume, kMediumPersistence,
                        kLowPrioritySound);
            }
        }
    }
}

void AdmiralThink() {
    for (auto destBalance: Destination::all()) {
        destBalance->buildTime -= 10;
        if (destBalance->buildTime <= 0) {
            destBalance->buildTime = 0;
            if (destBalance->buildObjectBaseNum.get()) {
                auto anObject = destBalance->whichObject;
                AdmiralBuildAtObject(anObject->owner, destBalance->buildObjectBaseNum, destBalance);
                destBalance->buildObjectBaseNum = BaseObject::none();
            }
        }

        auto anObject = destBalance->whichObject;
        if (anObject.get() && anObject->owner.get()) {
            anObject->owner->pay(destBalance->earn);
        }
    }

    for (auto a: Admiral::all()) {
        a->think();
    }
}

void Admiral::think() {
    Handle<SpaceObject> anObject;
    Handle<SpaceObject> destObject;
    Handle<SpaceObject> otherDestObject;
    Handle<SpaceObject> stepObject;
    Handle<SpaceObject> origObject;
    int32_t difference;
    Fixed  friendValue, foeValue, thisValue;
    Point gridLoc;

    if (!(_attributes & kAIsComputer) || (_attributes & kAIsRemote)) {
        return;
    }

    if (_blitzkrieg > 0) {
        _blitzkrieg--;
        if (_blitzkrieg <= 0) {
            // Really 48:
            _blitzkrieg = 0 - (gRandomSeed.next(1200) + 1200);
            for (auto anObject: SpaceObject::all()) {
                if (anObject->owner.get() == this) {
                    anObject->currentTargetValue = 0x00000000;
                }
            }
        }
    } else {
        _blitzkrieg++;
        if (_blitzkrieg >= 0) {
            // Really 48:
            _blitzkrieg = gRandomSeed.next(1200) + 1200;
            for (auto anObject: SpaceObject::all()) {
                if (anObject->owner.get() == this) {
                    anObject->currentTargetValue = 0x00000000;
                }
            }
        }
    }

    // get the current object
    if (!_considerShip.get()) {
        _considerShip = anObject = gRootObject;
        _considerShipID = anObject->id;
    } else {
        anObject = _considerShip;
    }

    if (!_destinationObject.get()) {
        _destinationObject = gRootObject;
    }

    if (anObject->active != kObjectInUse) {
        _considerShip = anObject = gRootObject;
        _considerShipID = anObject->id;
    }

    if (_destinationObject.get()) {
        destObject = _destinationObject;
        if (destObject->active != kObjectInUse) {
            destObject = _destinationObject = gRootObject;
        }
        auto origDest = _destinationObject;
        do {
            _destinationObject = destObject->nextObject;

            // if we've gone through all of the objects
            if (!_destinationObject.get()) {
                // ********************************
                // SHIP MUST DECIDE, THEN INCREASE CONSIDER SHIP
                // ********************************
                if ((anObject->duty != eEscortDuty)
                        && (anObject->duty != eHostileBaseDuty)
                        && (anObject->bestConsideredTargetValue >
                            anObject->currentTargetValue)) {
                    _destinationObject = anObject->bestConsideredTargetNumber;
                    _has_destination = true;
                    if (_destinationObject.get()) {
                        destObject = _destinationObject;
                        if (destObject->active == kObjectInUse) {
                            _destinationObjectID = destObject->id;
                            anObject->currentTargetValue
                                = anObject->bestConsideredTargetValue;
                            thisValue = anObject->randomSeed.next(
                                    mFloatToFixed(0.5))
                                - mFloatToFixed(0.25);
                            thisValue = mMultiplyFixed(
                                    thisValue, anObject->currentTargetValue);
                            anObject->currentTargetValue += thisValue;
                            SetObjectDestination(anObject, SpaceObject::none());
                        }
                    }
                    _has_destination = false;
                }

                if ((anObject->duty != eEscortDuty)
                        && (anObject->duty != eHostileBaseDuty)) {
                    _thisFreeEscortStrength += anObject->baseType->offenseValue;
                }

                anObject->bestConsideredTargetValue = 0xffffffff;
                // start back with 1st ship
                _destinationObject = gRootObject;
                destObject = gRootObject;

                // >>> INCREASE CONSIDER SHIP
                origObject = anObject =_considerShip;
                if (anObject->active != kObjectInUse) {
                    anObject = gRootObject;
                    _considerShip = gRootObject;
                    _considerShipID = anObject->id;
                }
                do {
                    _considerShip = anObject->nextObject;
                    if (!_considerShip.get()) {
                        _considerShip = gRootObject;
                        anObject = gRootObject;
                        _considerShipID = anObject->id;
                        _lastFreeEscortStrength = _thisFreeEscortStrength;
                        _thisFreeEscortStrength = 0;
                    } else {
                        anObject = anObject->nextObject;
                        _considerShipID = anObject->id;
                    }
                } while (((anObject->owner.get() != this)
                            || (!(anObject->attributes & kCanAcceptDestination))
                            || (anObject->active != kObjectInUse))
                        && (_considerShip != origObject));
            } else {
                destObject = destObject->nextObject;
            }
            _destinationObjectID = destObject->id;
        } while (((!(destObject->attributes & (kCanBeDestination)))
                    || (_destinationObject == _considerShip)
                    || (destObject->active != kObjectInUse)
                    || (!(destObject->attributes & kCanBeDestination)))
                && (_destinationObject != origDest));

        // if our object is legal and our destination is legal
        if ((anObject->owner.get() == this)
                && (anObject->attributes & kCanAcceptDestination)
                && (anObject->active == kObjectInUse)
                && (destObject->attributes & (kCanBeDestination))
                && (destObject->active == kObjectInUse)
                && ((anObject->owner != destObject->owner)
                    || (anObject->baseType->destinationClass <
                        destObject->baseType->destinationClass))) {
            gridLoc = destObject->distanceGrid;
            stepObject = otherDestObject = destObject;
            while (stepObject->nextFarObject.get()) {
                if ((stepObject->distanceGrid.h == gridLoc.h)
                        && (stepObject->distanceGrid.v == gridLoc.v)) {
                    otherDestObject = stepObject;
                }
                stepObject = stepObject->nextFarObject;
            }
            if (otherDestObject->owner == anObject->owner) {
                friendValue = otherDestObject->localFriendStrength;
                foeValue = otherDestObject->localFoeStrength;
            } else {
                foeValue = otherDestObject->localFriendStrength;
                friendValue = otherDestObject->localFoeStrength;
            }


            thisValue = kUnimportantTarget;
            if (destObject->owner == anObject->owner) {
                if (destObject->attributes & kIsDestination) {
                    if (destObject->escortStrength < destObject->baseType->friendDefecit) {
                        thisValue = kAbsolutelyEssential;
                    } else if (foeValue) {
                        if (foeValue >= friendValue) {
                            thisValue = kMostImportantTarget;
                        } else if (foeValue > (friendValue >> 1)) {
                            thisValue = kVeryImportantTarget;
                        } else {
                            thisValue = kUnimportantTarget;
                        }
                    } else {
                        if ((_blitzkrieg > 0) && (anObject->duty == eGuardDuty)) {
                            thisValue = kUnimportantTarget;
                        } else {
                            if (foeValue > 0) {
                                thisValue = kSomewhatImportantTarget;
                            } else {
                                thisValue = kUnimportantTarget;
                            }
                        }
                    }
                    if (anObject->baseType->orderFlags & kTargetIsBase) {
                        thisValue <<= 3;
                    }
                    if (anObject->baseType->orderFlags & kHardTargetIsNotBase) {
                        thisValue = 0;
                    }
                } else {
                    if (destObject->baseType->destinationClass
                            > anObject->baseType->destinationClass) {
                        if (foeValue > friendValue) {
                            thisValue = kMostImportantTarget;
                        } else {
                            if (destObject->escortStrength
                                    < destObject->baseType->friendDefecit) {
                                thisValue = kMostImportantTarget;
                            } else {
                                thisValue = kUnimportantTarget;
                            }
                        }
                    } else {
                        thisValue = kUnimportantTarget;
                    }
                    if (anObject->baseType->orderFlags & kTargetIsNotBase) {
                        thisValue <<= 3;
                    }
                    if (anObject->baseType->orderFlags & kHardTargetIsBase) {
                        thisValue = 0;
                    }
                }
                if (anObject->baseType->orderFlags & kTargetIsFriend) {
                    thisValue <<= 3;
                }
                if (anObject->baseType->orderFlags & kHardTargetIsFoe) {
                    thisValue = 0;
                }
            } else if (destObject->owner.get()) {
                if ((anObject->duty == eGuardDuty) || (anObject->duty == eNoDuty)) {
                    if (destObject->attributes & kIsDestination) {
                        if (foeValue < friendValue) {
                            thisValue = kMostImportantTarget;
                        } else {
                            thisValue = kSomewhatImportantTarget;
                        }
                        if (_blitzkrieg > 0) {
                            thisValue <<= 2;
                        }
                        if (anObject->baseType->orderFlags & kTargetIsBase) {
                            thisValue <<= 3;
                        }

                        if (anObject->baseType->orderFlags & kHardTargetIsNotBase) {
                            thisValue = 0;
                        }
                    } else {
                        if (friendValue) {
                            if (friendValue < foeValue) {
                                thisValue = kSomewhatImportantTarget;
                            } else {
                                thisValue = kUnimportantTarget;
                            }
                        } else {
                            thisValue = kLeastImportantTarget;
                        }
                        if (anObject->baseType->orderFlags & kTargetIsNotBase) {
                            thisValue <<= 1;
                        }

                        if (anObject->baseType->orderFlags & kHardTargetIsBase) {
                            thisValue = 0;
                        }
                    }
                }
                if (anObject->baseType->orderFlags & kTargetIsFoe) {
                    thisValue <<= 3;
                }
                if (anObject->baseType->orderFlags & kHardTargetIsFriend) {
                    thisValue = 0;
                }
            } else {
                if (destObject->attributes & kIsDestination) {
                    thisValue = kVeryImportantTarget;
                    if (_blitzkrieg > 0) {
                        thisValue <<= 2;
                    }
                    if (anObject->baseType->orderFlags & kTargetIsBase) {
                        thisValue <<= 3;
                    }
                    if (anObject->baseType->orderFlags & kHardTargetIsNotBase) {
                        thisValue = 0;
                    }
                } else {
                    if (anObject->baseType->orderFlags & kTargetIsNotBase) {
                        thisValue <<= 3;
                    }
                    if (anObject->baseType->orderFlags & kHardTargetIsBase) {
                        thisValue = 0;
                    }
                }
                if (anObject->baseType->orderFlags & kTargetIsFoe) {
                    thisValue <<= 3;
                }
                if (anObject->baseType->orderFlags & kHardTargetIsFriend) {
                    thisValue = 0;
                }
            }

            difference = ABS(implicit_cast<int32_t>(destObject->location.h)
                    - implicit_cast<int32_t>(anObject->location.h));
            gridLoc.h = difference;
            difference =  ABS(implicit_cast<int32_t>(destObject->location.v)
                    - implicit_cast<int32_t>(anObject->location.v));
            gridLoc.v = difference;

            if ((gridLoc.h < kMaximumRelevantDistance)
                    && (gridLoc.v < kMaximumRelevantDistance)) {
                if (anObject->baseType->orderFlags & kTargetIsLocal) {
                    thisValue <<= 3;
                }
                if (anObject->baseType->orderFlags & kHardTargetIsRemote) {
                    thisValue = 0;
                }
            } else {
                if (anObject->baseType->orderFlags & kTargetIsRemote) {
                    thisValue <<= 3;
                }
                if (anObject->baseType->orderFlags & kHardTargetIsLocal) {
                    thisValue = 0;
                }
            }


            if (anObject->baseType->orderKeyTag
                    && (anObject->baseType->orderKeyTag == destObject->baseType->levelKeyTag)) {
                thisValue <<= 3;
            } else if (anObject->baseType->orderFlags & kHardMatchingFoe) {
                thisValue = 0;
            }

            if (thisValue > 0) {
                thisValue += anObject->randomSeed.next(thisValue >> 1) - (thisValue >> 2);
            }
            if (thisValue > anObject->bestConsideredTargetValue) {
                anObject->bestConsideredTargetValue = thisValue;
                anObject->bestConsideredTargetNumber = _destinationObject;
            }
        }
    }

    // if we've saved enough for our dreams
    if (_cash > _saveGoal) {
        _saveGoal = 0;

        // consider what ship to build
        if (!_buildAtObject.get()) {
            _buildAtObject = Handle<Destination>(0);
        }

        // try to find the next destination object that we own & that can build
        auto anObject = SpaceObject::none();
        auto begin = _buildAtObject.number() + 1;
        auto end = begin + kMaxDestObject;
        for (int i = begin; i < end; ++i) {
            auto d =_buildAtObject = Handle<Destination>(i % kMaxDestObject);
            if (d->whichObject.get()
                    && (d->whichObject->owner.get() == this)
                    && (d->whichObject->attributes & kCanAcceptBuild)) {
                anObject = d->whichObject;
                break;
            }
        }

        // if we have a legal object
        if (anObject.get()) {
            auto destBalance = anObject->asDestination;
            if (destBalance->buildTime <= 0) {
                if (_hopeToBuild < 0) {
                    int k = 0;
                    while ((_hopeToBuild < 0) && (k < 7)) {
                        k++;
                        // choose something to build
                        thisValue = gRandomSeed.next(_totalBuildChance);
                        friendValue = 0xffffffff; // equals the highest qualifying object
                        for (int j = 0; j < kMaxNumAdmiralCanBuild; ++j) {
                            if ((_canBuildType[j].chanceRange <= thisValue)
                                    && (_canBuildType[j].chanceRange > friendValue)) {
                                friendValue = _canBuildType[j].chanceRange;
                                _hopeToBuild = _canBuildType[j].baseNum;
                            }
                        }
                        if (_hopeToBuild >= 0) {
                            auto baseObject = mGetBaseObjectFromClassRace(_hopeToBuild, _race);
                            if (baseObject->buildFlags & kSufficientEscortsExist) {
                                for (auto anObject: SpaceObject::all()) {
                                    if ((anObject->active)
                                            && (anObject->owner.get() == this)
                                            && (anObject->base == baseObject)
                                            && (anObject->escortStrength <
                                                baseObject->friendDefecit)) {
                                        _hopeToBuild = -1;
                                        break;
                                    }
                                }
                            }

                            if (baseObject->buildFlags & kMatchingFoeExists) {
                                thisValue = 0;
                                for (auto anObject: SpaceObject::all()) {
                                    if ((anObject->active)
                                            && (anObject->owner.get() != this)
                                            && (anObject->baseType->levelKeyTag
                                                == baseObject->orderKeyTag)) {
                                        thisValue = 1;
                                    }
                                }
                                if (!thisValue) {
                                    _hopeToBuild = -1;
                                }
                            }
                        }
                    }
                }
                int j = 0;
                while ((destBalance->canBuildType[j] != _hopeToBuild)
                        && (j < kMaxTypeBaseCanBuild)) {
                    j++;
                }
                if ((j < kMaxTypeBaseCanBuild) && (_hopeToBuild != kNoShip)) {
                    auto baseObject = mGetBaseObjectFromClassRace(_hopeToBuild, _race);
                    if (_cash >= mLongToFixed(baseObject->price)) {
                        Admiral::build(j);
                        _hopeToBuild = -1;
                        _saveGoal = 0;
                    } else {
                        _saveGoal = mLongToFixed(baseObject->price);
                    }
                } // otherwise just wait until we get to it
            }
        }
    }
}

bool Admiral::build(int32_t buildWhichType) {
    auto dest = _buildAtObject;
    Handle<Admiral> self(this - gAdmiralData.get());
    if ((buildWhichType >= 0)
            && (buildWhichType < kMaxTypeBaseCanBuild)
            && (dest.get())
            && (dest->buildTime <= 0)) {
        auto buildBaseObject = mGetBaseObjectFromClassRace(dest->canBuildType[buildWhichType], _race);
        if (buildBaseObject.get() && (buildBaseObject->price <= mFixedToLong(_cash))) {
            _cash -= (mLongToFixed(buildBaseObject->price));
            if (_cheats & kBuildFastBit) {
                dest->buildTime = 9;
                dest->totalBuildTime = 9;
            } else {
                dest->buildTime = buildBaseObject->buildTime;
                dest->totalBuildTime = dest->buildTime;
            }
            dest->buildObjectBaseNum = buildBaseObject;
            return true;
        }
    }
    return false;
}

void StopBuilding(Handle<Destination> destObject) {
    destObject->totalBuildTime = destObject->buildTime = 0;
    destObject->buildObjectBaseNum = BaseObject::none();
}

void Admiral::pay(Fixed howMuch) {
    pay_absolute(mMultiplyFixed(howMuch, _earning_power));
}

void Admiral::pay_absolute(Fixed howMuch) {
    _cash += howMuch;
    if (_cash < 0) {
        _cash = 0;
    }
}

void AlterAdmiralScore(Handle<Admiral> admiral, int32_t whichScore, int32_t amount) {
    if (admiral.get() && (whichScore >= 0) && (whichScore < kAdmiralScoreNum)) {
        admiral->score()[whichScore] += amount;
    }
}

int32_t GetAdmiralScore(Handle<Admiral> admiral, int32_t whichScore) {
    if (admiral.get() && (whichScore >= 0) && (whichScore < kAdmiralScoreNum)) {
        return admiral->score()[whichScore];
    } else {
        return 0;
    }
}

int32_t GetAdmiralShipsLeft(Handle<Admiral> admiral) {
    if (admiral.get()) {
        return admiral->shipsLeft();
    } else {
        return 0;
    }
}

int32_t AlterDestinationObjectOccupation(
        Handle<Destination> d, Handle<Admiral> a, int32_t amount) {
    if (a.get()) {
        d->occupied[a.number()] += amount;
        return d->occupied[a.number()];
    } else {
        return -1;
    }
}

void ClearAllOccupants(Handle<Destination> d, Handle<Admiral> a, int32_t fullAmount) {
    for (int i = 0; i < kMaxPlayerNum; i++) {
        d->occupied[i] = 0;
    }
    if (a.get()) {
        d->occupied[a.number()] = fullAmount;
    }
}

void AddKillToAdmiral(Handle<SpaceObject> anObject) {
    // only for player
    const auto& admiral = g.admiral;

    if (anObject->attributes & kCanAcceptDestination) {
        if (anObject->owner == g.admiral) {
            admiral->losses()++;
        } else {
            admiral->kills()++;
        }
    }
}

int32_t GetAdmiralLoss(Handle<Admiral> admiral) {
    if (admiral.get()) {
        return admiral->losses();
    } else {
        return 0;
    }
}

int32_t GetAdmiralKill(Handle<Admiral> admiral) {
    if (admiral.get()) {
        return admiral->kills();
    } else {
        return 0;
    }
}

}  // namespace antares
