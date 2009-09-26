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

#include "AresMain.hpp"

#include <Timer.h>
#include <Quickdraw.h>
//#include <stdio.h>        // for _DATE_ & _TIME_ macros

#define kProfiling_On   0
#if kProfiling_On
#include <profiler.h>
#endif

#include "Admiral.hpp"
#include "AmbrosiaSerial.h"
#include "AresAppleEvent.hpp"
#include "AresCheat.hpp"
#include "AresDemoScanner.hpp"
#include "AresExternalFile.hpp"
#include "AresGameRanger.hpp"
#include "AresGlobalType.hpp"
#include "AresGuideMaker.hpp"
#include "AresMoviePlayer.hpp"
#include "AresNetworkSprocket.hpp"
#include "AresPreferences.hpp"
#include "AresResFile.hpp"

#include "Beam.hpp"

#include "ColorTable.hpp"
#include "ColorTranslation.hpp"
#include "ConditionalMacros.h"
//#include "CopyProtection.h"  // is included in prefs

#include "Debug.hpp"
#include "DirectText.hpp"

#include "EnvironmentCheck.hpp"
#include "Error.hpp"

#include "Fakes.hpp"
#include "Fonts.h"

#include "GDeviceHandling.hpp"
#include "GXMath.h"

#include "HideMenubar.hpp"

#include "InputSource.hpp"
#include "Instruments.hpp"
#include "InterfaceHandling.hpp"

#include "KeyCodes.hpp"
#include "KeyMapTranslation.hpp"

#include "MathSpecial.hpp"
#include "MessageScreen.hpp"
#include "Minicomputer.hpp"
#include "Motion.hpp"
#include "Music.hpp"

#include "NateDraw.hpp"
#include "NetSetupScreen.hpp"
#include "NonPlayerShip.hpp"

#include "OffscreenGWorld.hpp"
#include "Options.hpp"

#include "PlayerInterface.hpp"
#include "PlayerShip.hpp"

#include "Randomize.hpp"
#include "RegistrationTool.h"
#include "Resources.h"
#include "Rotation.hpp"

#include "ScenarioMaker.hpp"
#include "ScreenLabel.hpp"
#include "ScrollStars.hpp"
#include "SetFontByString.h"
#include "ShotsBeamsExplosions.hpp"
#include "SoundFX.hpp"
#include "SpaceObjectHandling.hpp"
#include "SpriteCursor.hpp"
#include "SpriteHandling.hpp"
#include "StringHandling.hpp"

#include "TimeLimit.hpp"
#include "TimeUnit.hpp"
#include "TitleScreen.hpp"
#include "ToolUtils.h"
#include "Transitions.hpp"
#include "Traps.h"

#include "VersionString.hpp"

#include "WinAresGlue.hpp"
#include "WrapGameRanger.hpp"

//#define   kTempNet

#define MENU_BAR_ID     500
#define APPLE_MENU_ID   500
#define FILE_MENU_ID    501
#define EDIT_MENU_ID    502

#define ABOUT_ALERT     500

#define ABOUT_ITEM      1

#define QUIT_ITEM       1

#define kSmallScreenMem     2580
#define kMediumScreenMem    3092
#define kLargeScreenMem     3855
#define kBaseMemorySize     2000000//1652000

#define kReplayDataSize     6000L
#define kReplayBufferSize   5000L

#define kMainError      "\pmain"
#define kReplayResType  'NLRP'
#define kReplayResID    600

#define k68KMaxFrameSkip    9

#define kResendRequestFirstTime     600
#define kResendRequestSecondTime    1200

// result 0 = lose, 1 = win, 2 = restart, 3 = quit

#define kLoseGame       0
#define kWinGame            1
#define kRestartGame    2
#define kQuitGame       3
#define kNoGame         -1

//#define   kCanRecordGame

#define kReplayNotAutoPlay  // if this is set, will play back recorded games

#define mAbortNetworkGame       { if ( globals()->gGameOver == 0) globals()->gGameOver = 1; result = kQuitGame; StopNetworking();}

#define kTitleTextScrollWidth   450

#define kFractionalLagCorrectTolerance  8

#define kInterfaceResFileName   "\p:Ares Data Folder:Ares Interfaces"
#define kSpriteResFileName      "\p:Ares Data Folder:Ares Sprites"
#define kSoundResFileName       "\p:Ares Data Folder:Ares Sounds"
//#define   kConstantRate

//#define   kTestNumber     50//5

//#define   kUseSmallPlayWindow // for debugging

extern TypedHandle<spaceObjectType> gSpaceObjectData;
extern long gRandomSeed;
extern long gNatePortLeft, gNatePortTop, gNetLatency;
extern scenarioType *gThisScenario;
extern short gSpriteFileRefID, gInterfaceFileRefID;
extern GDHandle theDevice;

CWindowPtr      gTheWindow = nil;//, globals()->gBackWindow = nil;
MenuHandle      gAppleMenu;
long            gLastTick, // globals()->gGameOver = 0,
                WORLD_WIDTH = 640,
                WORLD_HEIGHT = 480,
                CLIP_LEFT = 128,
                CLIP_TOP = 0,
                CLIP_RIGHT = 608,
                CLIP_BOTTOM = 480,
                gPlayScreenWidth = 480,
                gPlayScreenHeight = 480;
Boolean         hackWarn = true;

unsigned long kNoKeys = 0;

int GetDemoScenario();
void FakeInit(int argc, const char** argv);
int main(int argc, const char** argv);
void SetWindowColorTable( WindowPtr);
void Pause( long time);
void DrawOutlinedString( const unsigned char* string, RGBColor *color);

int main(int argc, const char** argv) {
    Rect                    windowRect, tRect;
    OSErr                   error;
    RGBColor                initialFadeColor;
    Boolean                 skipFading = false;
    Point                   tpoint;
    EventRecord             theEvent;
    scoped_ptr<ColorTable>  theClut;
    Size                    freeMemory = 0;
    Str255                  tempString, userName;
    short                   ts1;

    FakeInit(argc, argv);

    init_globals();

    error = InitPreferences();
    if (error == kNoError) {

#if NETSPROCKET_AVAILABLE
        if ( InitNetworking() == kNoError)
        {
            globals()->gOptions |= kOptionNetworkAvailable;
        } else
#endif NETSPROCKET_AVAILABLE
        {
            globals()->gOptions &= ~kOptionNetworkAvailable;
        }

        GetDateTime( reinterpret_cast<unsigned long *>(&qd.randSeed));
        GetDateTime( reinterpret_cast<unsigned long *>(&gRandomSeed));

        error  = RT_Open( true, VERSION_2_CODES);
        if ( error != noErr)
        {
            ShowErrorOfTypeOccurred( eQuitErr, kErrorStrID, 77, error, __FILE__, 0);
        }

        if ( OptionKey())
        {
            globals()->gOptions &= ~(kOptionScreenMedium | kOptionScreenLarge | kOptionScreenSmall);
            globals()->gOptions |= kOptionScreenSmall;//kOptionScreenLarge;
        }

        if ( globals()->gOptions & kOptionScreenMedium)
        {
            WORLD_WIDTH = kMediumScreenWidth;
            WORLD_HEIGHT = kMediumScreenHeight;
            CLIP_LEFT = kLeftPanelWidth;
            CLIP_TOP = 0;
            CLIP_RIGHT = kMediumScreenWidth - kRightPanelWidth;
            CLIP_BOTTOM = kMediumScreenHeight;
            gPlayScreenWidth = kMediumScreenWidth - (kLeftPanelWidth + kRightPanelWidth);
            gPlayScreenHeight = kMediumScreenHeight;
        } else if ( globals()->gOptions & kOptionScreenLarge)
        {
            WORLD_WIDTH = kLargeScreenWidth;
            WORLD_HEIGHT = kLargeScreenHeight;
            CLIP_LEFT = kLeftPanelWidth;
            CLIP_TOP = 0;
            CLIP_RIGHT = kLargeScreenWidth - kRightPanelWidth;
            CLIP_BOTTOM = kLargeScreenHeight;
            gPlayScreenWidth = kLargeScreenWidth - (kLeftPanelWidth + kRightPanelWidth);
            gPlayScreenHeight = kLargeScreenHeight;
        } else
        {
            WORLD_WIDTH = kSmallScreenWidth;
            WORLD_HEIGHT = kSmallScreenHeight;
            CLIP_LEFT = kLeftPanelWidth;
            CLIP_TOP = 0;
            CLIP_RIGHT = kSmallScreenWidth - kRightPanelWidth;
            CLIP_BOTTOM = kSmallScreenHeight;
            gPlayScreenWidth = kSmallScreenWidth - (kLeftPanelWidth + kRightPanelWidth);
            gPlayScreenHeight = kSmallScreenHeight;
        }

        // returns true if device of desired size available
//      Debugger();
//      error = ChooseTheDevice( 8, TRUE);

        MacSetRect( &tRect, 0, 0, 640, 480);
        error = UserChooseTheDevice( 8, TRUE, &tRect);

        #ifndef kUseSmallPlayWindow
            WORLD_WIDTH = tRect.right - tRect.left;
            WORLD_HEIGHT = tRect.bottom - tRect.top;
        #else
            WORLD_WIDTH = 640;
            WORLD_HEIGHT = 480;
        #endif

        if (( WORLD_WIDTH > kLargeScreenWidth) || ( WORLD_HEIGHT > kLargeScreenHeight))
        {
            MacSetRect( &tRect, 0, 0, kLargeScreenWidth, kLargeScreenHeight);
            CenterRectInDevice( theDevice, &tRect);
            WORLD_WIDTH = tRect.right - tRect.left;
            WORLD_HEIGHT = tRect.bottom - tRect.top;
        }


        freeMemory = CompactMem( maxSize) - kBaseMemorySize;
        freeMemory /= 3;
        if ( freeMemory < (WORLD_WIDTH * WORLD_HEIGHT))
        {
            WORLD_HEIGHT = 3L * freeMemory;
            WORLD_HEIGHT /= 4;
            WORLD_HEIGHT = lsqrt( WORLD_HEIGHT);
            WORLD_WIDTH = WORLD_HEIGHT * 4;
            WORLD_WIDTH /= 3;
            WORLD_WIDTH -= WORLD_WIDTH % 8;
            if (( WORLD_WIDTH > kLargeScreenWidth) || ( WORLD_HEIGHT > kLargeScreenHeight))
            {
                WORLD_WIDTH = kLargeScreenWidth;
                WORLD_HEIGHT = kLargeScreenHeight;
            }
            if (( WORLD_WIDTH < kSmallScreenWidth) || ( WORLD_HEIGHT > kSmallScreenHeight))
            {
                // ERROR put in error handing & exit here -- perhaps you can calc it?
            }
            MacSetRect( &tRect, 0, 0, WORLD_WIDTH, WORLD_HEIGHT);
            CenterRectInDevice( theDevice, &tRect);
            WORLD_WIDTH = tRect.right - tRect.left;
            WORLD_HEIGHT = tRect.bottom - tRect.top;
//          NumToString( WORLD_WIDTH, tempString);
//          ParamText( tempString, nil, nil, nil);
//          Alert( 802, nil);
        } else // we had enough memory to fill the screen (up to large screen size)
        {
//          NumToString( freeMemory, tempString);
//          ParamText( tempString, nil, nil, nil);
//          Alert( 802, nil);
        }

            CLIP_LEFT = kLeftPanelWidth;
            CLIP_TOP = 0;
            CLIP_RIGHT = WORLD_WIDTH - kRightPanelWidth;
            CLIP_BOTTOM = WORLD_HEIGHT;
            gPlayScreenWidth = WORLD_WIDTH - (kLeftPanelWidth + kRightPanelWidth);
            gPlayScreenHeight = WORLD_HEIGHT;

        {
            short   oldResFile = CurResFile();

            UseResFile( globals()->gMainResRefNum);

            theClut.reset(new ColorTable(256));

            UseResFile( oldResFile);
        }

        if (theClut.get() == nil) {
            ShowErrorAny( eQuitErr, kErrorStrID, nil, nil, nil, nil, RESOURCE_ERROR, -1, -1, -1, __FILE__, 500);
        }

//      theDevice = GetMainDevice();
//      error = true;
//      SetColorDepth( theDevice, 8);
        if ( !error) // really if device was not available
        {
            WORLD_WIDTH = kSmallScreenWidth;
            WORLD_HEIGHT = kSmallScreenHeight;
            CLIP_LEFT = kLeftPanelWidth;
            CLIP_TOP = 0;
            CLIP_RIGHT = kSmallScreenWidth - kRightPanelWidth;
            CLIP_BOTTOM = kSmallScreenHeight;
            gPlayScreenWidth = kSmallScreenWidth - (kLeftPanelWidth + kRightPanelWidth);
            gPlayScreenHeight = kSmallScreenHeight;
            globals()->gOptions &= ~( kOptionScreenSmall | kOptionScreenMedium |
                            kOptionScreenLarge);
            globals()->gOptions |= kOptionScreenSmall;
            error = ChooseTheDevice( 8, TRUE);
        }
        if ( error) // really if device was available
        {
            MacSetRect( &windowRect, 0, 0, WORLD_WIDTH, WORLD_HEIGHT);
            GetDeviceRect( theDevice, &tRect);
            tpoint.h = tpoint.v = 0;
            ShieldCursor( &tRect, tpoint);

            if ( theDevice == GetMainDevice())
            {

                // FROM ADG:TOOL CHEST 2/95: HIDEMENUBAR CODE SNIPPET
                //
                // Set the global MBarHeight to 0 to prevent any
                //  other apps from writing to the menu bar.
                //

/*              MacSetRect( &mBarRect, tRect.left, tRect.top, tRect.right, tRect.top + oldMBarHeight);

                #ifdef kHideMenuBar
                grayRgn = LMGetGrayRgn();
                LMSetMBarHeight( 0);

                // from TotMGPG p 127-129
//              oldGrayRgn = NewRgn();
//              CopyRgn( grayRgn, oldGrayRgn);

                mBarRgn = NewRgn();
                if ( mBarRgn == nil)
                        ShowErrorRecover( OFFSCREEN_GRAPHICS_ERROR, kMainError, 2);
                RectRgn( mBarRgn, &mBarRect);
                UnionRgn( grayRgn, mBarRgn, grayRgn);
                #endif
*/
//              InitHideMenubar();
//              SetMBarState( false);
            }

            InitHideMenubar();
            SetMBarState( false, theDevice);

            InitSpriteCursor();
            CenterRectInDevice( theDevice, &windowRect);

            globals()->gBackWindow = nil;
            globals()->gBackWindow = NewCWindow (nil, &tRect, "\p", false, plainDBox,
                        reinterpret_cast<WindowPtr>(-1), false, 701);
            SetWindowColorTable( globals()->gBackWindow);
            InitTransitions();

            initialFadeColor.red = initialFadeColor.green = initialFadeColor.blue = 0;
            MacSetPort( globals()->gBackWindow);
            RGBBackColor( &initialFadeColor);
            BackPat( &qd.black);

            initialFadeColor.red = initialFadeColor.green = initialFadeColor.blue = 0;
//          initialFadeColor.green = 65000;
            RGBForeColor( &initialFadeColor);
            skipFading = AutoFadeTo( 30, &initialFadeColor, true);

            MacShowCursor();

            do
            {
                Ares_WaitNextEvent (everyEvent, &theEvent, 3, nil);
            } while ( theEvent.what != nullEvent);

//          WaitForAnyEvent();

            HideCursor();

            MacShowWindow( globals()->gBackWindow);

            RGBBackColor( &initialFadeColor);
//          BackPat( &qd.black);

//          WaitForAnyEvent();

            MacSetPort( globals()->gBackWindow);

            PaintRect( &(globals()->gBackWindow->portRect));

//          WaitForAnyEvent();

            RestoreDeviceClut( theDevice);

//          WaitForAnyEvent();

            ResetTransitions();

//          WaitForAnyEvent();

            skipFading = AutoFadeFrom( 1, true);


            skipFading = AutoFadeTo( 1, &initialFadeColor, true);

            gTheWindow = NewCWindow (nil, &windowRect, "\p", TRUE, plainDBox, //documentProc,//
                        reinterpret_cast<WindowPtr>(-1), true, 700);

            SetWindowColorTable( gTheWindow);
            initialFadeColor.red = initialFadeColor.green = initialFadeColor.blue = 0;
            MacSetPort( gTheWindow);
            RGBBackColor( &initialFadeColor);
//          BackPat( &qd.black);

            MacShowWindow ( gTheWindow);
            RGBBackColor( &initialFadeColor);
//          BackPat( &qd.black);

//          WriteDebugLine("\p>Debug Win");

            MacSetPort ( gTheWindow);
            MacSetRect( &windowRect, 0, 0, WORLD_WIDTH, WORLD_HEIGHT);
            MacFillRect( &tRect, &qd.black);

            BringDebugToFront();
            skipFading = AutoFadeFrom(1, true);

            ShieldCursorInDevice();

            do
            {
                Ares_WaitNextEvent (everyEvent, &theEvent, 3, nil);
            } while ( theEvent.what != nullEvent);
            MacSetPort ( gTheWindow);
            MacShowCursor();

            error = CreateOffscreenWorld(gTheWindow->portRect, *theClut);
            if ( error == kNoError)
            {
                WriteDebugLine("\p>Offworld");
                WriteDebugLine("\pGDPMapBounds");
                WriteDebugLong( (*(*theDevice)->gdPMap)->bounds.left);
                WriteDebugLine("\pGDRect");
                WriteDebugLong( (*theDevice)->gdRect.left);
                WriteDebugLine("\pgNatePortLeft");
                WriteDebugLong( gNatePortLeft);
                WriteDebugLine("\pPortRect");
                WriteDebugLong( gTheWindow->portRect.left);

                error = MusicInit();
                    WriteDebugLine("\p>Music");
                if ( OpenSoundFile() == kNoError)
                {
                    mWriteDebugString("\p>Sound File");
                    InitMoviePlayer();

                    WriteDebugLine("\p>Movie");
                    error = RotationInit();
                    if ( error == kNoError)
                    {
                        WriteDebugLine("\p>Rot");

                        NormalizeColors();
                        DrawInRealWorld();
//                      AutoFadeFrom( 30);
                        ColorTranslatorInit(*theClut);
//                      InitTransitions();
//                      ResetTransitions();
                        error = InterfaceHandlingInit();
                        if ( error == kNoError)
                        {
                            WriteDebugLine("\p>Interface");

                            if ( globals()->originalExternalFileSpec.name[0] > 0)
                            {
                                globals()->externalFileSpec =
                                    globals()->originalExternalFileSpec;

                                EF_OpenExternalFile();
                            }

                            if ( globals()->gOptions & kOptionMusicIdle)
                            {
                                LoadSong( kTitleSongID);
                                SetSongVolume( kMaxMusicVolume);
                                PlaySong();
                            }
                            MacSetPort( gTheWindow);
                            if ( !skipFading)
                            {
                                skipFading = CustomPictFade( 20, 20, 2000, 2000, gTheWindow);
                                if ( !skipFading)
                                {
                                    skipFading = CustomPictFade( 20, 20, 2001, 2000, gTheWindow);
                                }
                            }

                            BlackTitleScreen();

                            if ( !skipFading) PlayMovieByName("\p:Ares Data Folder:Title", gTheWindow,
                                false, theDevice);

//                          AutoFadeTo( 1, &initialFadeColor, FALSE);
//                          DrawTitleScreen();
//                          AutoFadeFrom( 90, FALSE);
                            MacSetPort( gTheWindow);

                            skipFading = StartCustomPictFade( 20, 20, 502, 2001,
                                gTheWindow, skipFading);


//                          MacShowCursor();

/*                          ColorTest();
                            MacSetRect( &tRect, 0, 0, WORLD_WIDTH, WORLD_HEIGHT);
                            CopyOffWorldToRealWorld( (WindowPtr)gTheWindow, &tRect);

                            WaitForAnyEvent();
*/

            do
            {
                Ares_WaitNextEvent (everyEvent, &theEvent, 3, nil);
            } while ( theEvent.what != nullEvent);

                            MacSetPort( gTheWindow);
                            GetVersionString( tempString, globals()->gMainResRefNum);
                            SetFontByString( "\pgeneva");
                            TextSize( 9);
                            TextFace( 0);
                            initialFadeColor.red = initialFadeColor.blue = initialFadeColor.green = 30000;

                            RGBForeColor( &initialFadeColor);

/*                          MoveTo( ((WORLD_WIDTH - kSmallScreenWidth) / 2) +
                                ( kSmallScreenWidth / 2) - (StringWidth( tempString) / 2),
                                465 + ( WORLD_HEIGHT - kSmallScreenHeight) / 2);
*/
                            MoveTo( ( WORLD_WIDTH - StringWidth( tempString) - 4),
                                478 + ( WORLD_HEIGHT - kSmallScreenHeight) / 2);

/*
                            MoveTo( 370 + ( WORLD_WIDTH - kSmallScreenWidth) / 2,
                                    450 + ( WORLD_HEIGHT - kSmallScreenHeight) / 2);
*/
                            DrawOutlinedString( tempString, &initialFadeColor); //DrawString( tempString);

                            initialFadeColor.red = 65535;
                            initialFadeColor.blue = initialFadeColor.green = 0;
                            RGBForeColor( &initialFadeColor);
                            MoveTo( 4, 12 + ( WORLD_HEIGHT - kSmallScreenHeight) / 2);
                            #ifdef kUseAlphaCopyProtection
                                DrawOutlinedString("\pALPHA COPY PROTECTION IS ON. ",
                                    &initialFadeColor);
                            #endif
                            #ifndef kUsePublicCopyProtection
                                DrawOutlinedString("\pPUBLIC COPY PROTECTION IS OFF. ",
                                    &initialFadeColor);
                            #endif

//                          ColorTest();
                            MoveTo( 35 + ( WORLD_WIDTH - kSmallScreenWidth) / 2,
                                    50 + ( WORLD_HEIGHT - kSmallScreenHeight) / 2);
                            initialFadeColor.red = initialFadeColor.green = initialFadeColor.blue = 65535;

                            RGBForeColor( &initialFadeColor);
                            TextSize( 24);
                            TextFont( 0);
                            TextFace( 0);
//                          DrawEncodedString( "\p\x67\x36\x68\x2D\x0C\x0B\x13\x29\x48\x51\x64\x11\x0B\x27\x29\x0E\x4E\x4D\x46\x12\x57\x3D\x12\x3B\x75\x24\x14\x65\x31\x10\x5B\x30\x4D\x5F\x44\x16\x66\x4C\x4E\x6E"); // Beta Demo -- please do NOT redistribute!
//                          DrawEncodedString( "\p\x75\x43\x59\x42\x55\x2C\x25\x5B\x1D\x17\x25\x53\x0B\x63\x5C\x78\x59\x3F\x42\x65\x58\x6D\x36\x5C\x46\x1E\x63\x67\x6B\x7D\x57\x21\x42\x60\x4F\x26\x5A\x3A\x5E\x42\x35"); // Preview Demo - Please Do Not Redistribute
                            MoveTo( 35 + ( WORLD_WIDTH - kSmallScreenWidth) / 2,
                                        385 + ( WORLD_HEIGHT - kSmallScreenHeight) / 2);
                            TextSize( 12);
                            TextFont( 0);
                            TextFace( 0);
//                          DrawEncodedString( "\p\x7B\x36\x66\x3F\x55\x36\x1C\x5B\x0A\x5F\x67\x12\x1B\x77\x6E"); // version 1.0.0A2
//                          DrawEncodedString( "\p\x66\x46\x68\x34\x5B\x39\x17\x36\x3E\x16\x57\x59\x5E\x1C\x2F\x62\x0D"); // authorized user:
//                          prefsData = (preferencesDataType *)*globals()->gPreferencesData;
//                          DrawYeOldeEncodedString( (StringPtr)prefsData->serialNumber.name);

                            #ifdef kUsePublicCopyProtection
                                GetIndString( tempString, 700, 3);  // "REGISTERED TO:"
                                RT_GetLicenseeName( userName);
                                ConcatenatePString( tempString, userName);

                                ts1 = StringWidth( tempString);
//                              ts1 += StringWidth( userName);
                                MoveTo( (( WORLD_WIDTH) / 2) - (ts1 / 2),
                                            456 + ( WORLD_HEIGHT - kSmallScreenHeight) / 2);
                                DrawOutlinedString( tempString, &initialFadeColor);
//                              DrawString( globals()->gUserName);
                            #endif

                            if ( globals()->externalFileRefNum >= 0)
                            {
                                ts1 = StringWidth(
                                    globals()->scenarioFileInfo.titleString);

                                MoveTo( (( WORLD_WIDTH) / 2) - (ts1 / 2),
                                            356 + ( WORLD_HEIGHT - kSmallScreenHeight)
                                                / 2);
                                DrawOutlinedString(
                                        globals()->scenarioFileInfo.titleString,
                                        &initialFadeColor);
                            }

                            SetFontByString( "\pgeneva");
                            TextSize( 10);
                            TextFace( bold);
                            MoveTo( 245 + ( WORLD_WIDTH - kSmallScreenWidth) / 2,
                                    18 + ( WORLD_HEIGHT - kSmallScreenHeight) / 2);
//                          DrawCString( __DATE__);
//                          DrawCString(" ");
//                          DrawCString( __TIME__);
                            error = InitDirectText();
                            if ( error == kNoError)
                            {
                                WriteDebugLine("\p>DText");

                                error = ScreenLabelInit();
                                if ( error == kNoError)
                                {
                                    WriteDebugLine("\p>Label");

                                    error = InitMessageScreen();
                                    if ( error == kNoError)
                                    {
                                        WriteDebugLine("\p>Message");

                                        error = InitScrollStars();
                                        if ( error == kNoError)
                                        {
                                            WriteDebugLine("\p>ScrollStar");

                                            error = InstrumentInit();
                                            if ( error == kNoError)
                                            {
                                                WriteDebugLine("\p>Instrument");


                                                SpriteHandlingInit();
                                                AresCheatInit();
                                                error = ScenarioMakerInit();
                                                {
                                                    error = SpaceObjectHandlingInit();  // MUST be after ScenarioMakerInit()
                                                    if ( error == kNoError)
                                                    {
                                                        WriteDebugLine("\p>SpaceObj");
                                                        error = InitSoundFX();
//                                                      if ( error == kNoError)
                                                        {
                                                            WriteDebugLine("\p>SoundFX");
                                                            error =InitMotion();
                                                            if ( error == kNoError)
                                                            {
                                                                WriteDebugLine("\p>Motion");

                                                                error = AdmiralInit();
                                                                if ( error == kNoError)
                                                                {
                                                                    error = InitBeams();
                                                                    if ( error == kNoError)
                                                                    {
//          InitNetworking();
                                                                        TimedWaitForAnyEvent(
                                                                            skipFading?1:1400);
                                                                        EndCustomPictFade(
                                                                            gTheWindow,
                                                                            skipFading);
                                                                        MacShowCursor();    // one for the titlescreen
                                                                        MacShowCursor();    // one for the whole deal
//                                                                      ColorTranslatorInit( theClut);

                                                                        gLastTick = TickCount();

//          RandomInit();
                                                                        globals()->okToOpenFile = true;
                                                                        MainLoop();
//          RandomCleanup();
            CleanupMoviePlayer();
//          DisposeNetworking();
                                                                        CleanupBeams();
                                                                        WriteDebugLine("\p<Beams");
                                                                    }
                                                                    AdmiralCleanup();
                                                                    WriteDebugLine("\p<Admiral");
                                                                }

                                                                MotionCleanup();
                                                                WriteDebugLine("\p<Motion");
                                                            }

                                                            SoundFXCleanup();
                                                            WriteDebugLine("\p<Sound");
                                                        }
                                                        CleanupSpaceObjectHandling();
                                                        WriteDebugLine("\p<Obj Handle");
                                                        CleanupSpriteHandling();
                                                        CleanupAresCheat();
                                                        WriteDebugLine("\p<Sprite");
                                                    }
                                                    ScenarioMakerCleanup();
                                                }
                                                InstrumentCleanup();
                                                WriteDebugLine("\p<Instrument");
                                            }
                                            CleanupScrollStars();
                                            WriteDebugLine("\p<Stars");
                                        }
                                        MessageScreenCleanup();
                                        WriteDebugLine("\p<Message");
                                    }
                                    ScreenLabelCleanup();
                                    WriteDebugLine("\p<Label");
                                }
                                DirectTextCleanup();
                                WriteDebugLine("\p<DText");
                            }
                            InterfaceHandlingCleanup();
                            WriteDebugLine("\p<Interface");
                        }
                        ColorTranslatorCleanup();
                        WriteDebugLine("\p<Color");
                        RotationCleanup();
                        WriteDebugLine("\p<Rotation");
                        CleanupTransitions();
                        CleanupSpriteCursor();
                        WriteDebugLine("\p<Transition");
                        CleanupMoviePlayer();
                    }
                    MusicCleanup();
                    WriteDebugLine("\p<Music");
                }
                CleanUpOffscreenWorld();
                WriteDebugLine("\p<GWorld");
            }
            WriteDebugLine("\p<Network");
/*          WriteDebugLine("\p<WAITING>");
            WaitForAnyEvent();
*/
            CleanUpTheDevice( TRUE);

            theClut.reset();

            if ( theDevice == GetMainDevice())
            {
/*              LMSetMBarHeight(oldMBarHeight);
                DiffRgn( grayRgn, mBarRgn, grayRgn);
                DisposeRgn( mBarRgn);
*/
                SetMBarState( true, theDevice);
            }

            DebugWindowCleanup();
            DisposeWindow ( gTheWindow);
        } else
        {
            ShowErrorAny( eQuitErr, kErrorStrID, nil, nil, nil, nil, PIX_DEPTH_ERROR, -1, -1, -1, __FILE__, 1);
        }

#if NETSPROCKET_AVAILABLE
        DisposeNetworking();
#endif NETSPROCKET_AVAILABLE
        RT_Close();
        PreferencesCleanup();
    }

    if ( globals()->internetConfigPresent)
    {
//      ICStop( globals()->internetConfig);
    }

    FlushEvents(everyEvent, 0);
    delete globals();
    InitCursor();

}
//#pragma code68020 reset

//#pragma code68020 off
void ToolBoxInit( void)

{
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs( nil);
    InitCursor();
    SetEventMask( everyEvent);
    MoreMasters();
    MoreMasters();
    MoreMasters();
    MoreMasters();
    MoreMasters();
    MoreMasters();
    MaxApplZone();
    FlushEvents(everyEvent, 0);     // Removes all events
    /*SetGrowZone( &OutOfMemoryError);*/
}
//#pragma code68020 reset


void MenuBarInit( void)
{
    Handle      myMenuBar;

    myMenuBar = GetNewMBar( MENU_BAR_ID);
    SetMenuBar ( myMenuBar);
    gAppleMenu = GetMenuHandle (APPLE_MENU_ID);

    AppendResMenu( gAppleMenu, 'DRVR');
    MacDrawMenuBar();
}

void MainLoop (void)

{
    long                    whichScenario = 0, saveSeed = 0, gameLength, whichDemoLevel, t;
    Boolean                 done = FALSE, jumpLevel = false;
    mainScreenResultType    mainResult;
    RGBColor                fadeColor;
    short                   gameResult;
    Str255                  movieName;     // for GetResInfo, when we're jumping to a demo
    short                   resID = 0;          // '' ''

    if (!(globals()->gOptions & kOptionHaveSeenIntro))
    {
        DoScrollText( gTheWindow, 5600, 4, kTitleTextScrollWidth,
            kTitleFontNum, -1);

        globals()->gOptions |= kOptionHaveSeenIntro;
        SaveOptionsPreferences();
    }

    while ( !done)
    {
        WriteDebugLine("\p>MainScreen");
        MacSetPort( gTheWindow);
        mainResult = DoMainScreenInterface( &whichDemoLevel);

        switch( mainResult)
        {
            case kMainQuit:
#ifdef kCreateAresDemoData
                MakeDemoDataHack();
#endif
//              DoScrollText( (WindowPtr)gTheWindow, 6501, 4, 540,
//                  kTitleFontNum/*kComputerFontNum*/, -1);
//              MakeAresGuide();

                fadeColor.red = fadeColor.green = fadeColor.blue = 0;
                if ( globals()->gOptions & kOptionMusicIdle)
                {
                    AutoMusicFadeTo( 60, &fadeColor, FALSE);
                } else
                {
                    AutoFadeTo( 60, &fadeColor, FALSE);
                }
                done = TRUE;
                break;

            case kMainTimeoutDemo:

                if ( Randomize( 4) == 2)
                {
                    DoScrollText( gTheWindow, 5600, 4, kTitleTextScrollWidth,
                        kTitleFontNum, -1);
                }

                // NO BREAK! FALL THROUGH TO case kMainDemo:

            case kMainDemo: // identical to main play, except we turn on autoplay bit
//              AutoFadeTo( 60, &fadeColor, FALSE);
//              ShowSuccessAnimation( (WindowPtr)gTheWindow);

                globals()->gOptions |= kOptionReplay;

                // NO BREAK! FALL THROUGH TO case kMainPlay:

//What you are about to do is totally unsupported. Nothing is final. This is a pre-release development build!
                                  //This is a pre-release development build! This is not the final demo. Elements are subject to change.
            case kMainPlay:
                whichScenario = 0;
                jumpLevel = FALSE;

                if ( (!Ambrosia_Is_Registered()) &&
                    (globals()->externalFileRefNum > 0))
                {
                    ShowErrorAny( eContinueErr, kErrorStrID,  nil, nil, nil, nil,
                        79, -1, -1, -1, __FILE__, 2);

                    globals()->externalFileSpec.name[0] = 0;
                    EF_OpenExternalFile();
                }

                if ( !( globals()->gOptions & kOptionReplay))
                {
                    whichScenario = DoSelectLevelInterface( GetStartingLevelPreference());
                    jumpLevel = TRUE;
                }
//              globals()->gOptions |= kOptionAutoPlay;

                if ( whichScenario >= 0)
                {
                    if ( globals()->gOptions & kOptionReplay)
                    {
                        #ifdef kReplayNotAutoPlay

                        short   refNum = CurResFile();

                        UseResFile( globals()->gMainResRefNum);

                        if (!(jumpLevel))
                        {
                            if ( whichDemoLevel >= 0)// get indexed demo level
                            {
                                globals()->gInputSource.reset(
                                        new ReplayInputSource(whichDemoLevel + 1));
                                if (globals()->gInputSource.get() != nil) {
                                    //GetResInfo(reinterpret_cast<Handle>(globals()->gReplayData),
                                    //        &resID, &resType, resName);
                                    whichScenario = resID - kReplayResID;
                                }
                            }

                            if (globals()->gInputSource.get() == nil) {
                                do
                                {
                                    whichScenario = GetDemoScenario();
                                    // whichScenario = Randomize( globals()->levelNum);//Randomize( 30 + 1);
                                    // whichScenario = GetScenarioNumberFromChapterNumber( whichScenario);
                                    globals()->gInputSource.reset(
                                            new ReplayInputSource(kReplayResID + whichScenario));
                                } while (globals()->gInputSource.get() == nil);
                            }
                        } else {
                                globals()->gInputSource.reset(
                                        new ReplayInputSource(kReplayResID + whichScenario));
                        }

                        if (globals()->gInputSource.get() != nil) {
                            saveSeed = gRandomSeed;
                            gRandomSeed = globals()->gInputSource->random_seed();
                        }
                        UseResFile( refNum);

                        #else

//                      whichScenario = Randomize( kHackLevelMax);
                        whichScenario = 5;

                        #endif
                    }

                    StopAndUnloadSong();


                    gameResult = kNoGame;

                    do
                    {
                        if (( gameResult == kNoGame) || ( gameResult == kWinGame))
                        {
                            GetScenarioMovieName( whichScenario, movieName);
                            if ( movieName[0] != 0)
                            {
                                PlayMovieByName( movieName, gTheWindow,
                                    true, theDevice);
                            }

                            if ( GetScenarioPrologueID( whichScenario) > 0)
                            {
                                DoScrollText( gTheWindow,
                                    GetScenarioPrologueID( whichScenario),
                                    4, kTitleTextScrollWidth, kTitleFontNum, 4002);
                            }
                        }

                    gameResult = kNoGame;
                    fadeColor.red = fadeColor.green = fadeColor.blue = 0;
                    if ( globals()->gOptions & kOptionMusicIdle)
                    {
                        AutoMusicFadeTo( 60, &fadeColor, FALSE);
                        StopAndUnloadSong();
                    } else
                    {
                        AutoFadeTo( 60, &fadeColor, FALSE);
                    }

                        RemoveAllSpaceObjects();
                        globals()->gGameOver = 0;

                        // mission briefing unfades screen

    //                  if ( globals()->gOptions & kOptionAutoPlay) whichScenario = 2;

                        if ( globals()->gOptions & kOptionMusicIdle)
                        {
                            LoadSong( 3000);
                            SetSongVolume( kMaxMusicVolume);
                            PlaySong();
                        }

    //gRandomSeed = 910697788;
                        gameResult = kNoGame;
                        if ( !ConstructScenario( whichScenario))
                            gameResult = kQuitGame;

                        if ( gameResult != kQuitGame)
                        {
                            SetMBarState( false, theDevice);
                            if ( globals()->gOptions & kOptionMusicIdle)
                            {
                                StopAndUnloadSong();
                            }

                            HideCursor();
                            DrawInstrumentPanel(  gTheWindow);
                            MacShowCursor();

                            if ( globals()->gOptions & kOptionMusicPlay)
                            {
                                LoadSong( gThisScenario->songID);
                                SetSongVolume( kMusicVolume);
                                PlaySong();
                            }
                            ResetLastTime( (gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);

// *** PLAY THE GAME

                            gameResult = PlayTheGame( &gameLength);

//              DebugFileSave( "\p_Poopy");
//              DebugFileCleanup();


                            if (( !( globals()->gOptions & (kOptionAutoPlay | kOptionReplay))) && ( gameResult == kLoseGame))
                            {
                                if ( (globals()->gScenarioWinner & kScenarioWinnerTextMask) != kScenarioWinnerNoText)
                                {
                                    DoMissionDebriefingText(  gTheWindow,
                                        (globals()->gScenarioWinner & kScenarioWinnerTextMask) >> kScenarioWinnerTextShift,
                                            -1, -1, -1, -1, -1, -1, -1);
                                }
                                if ( DoPlayAgain( false, false))
                                {
                                    gameResult = kRestartGame;
                                    WriteDebugLine("\pAGAIN!");
                                }  else
                                {
                                    gameResult = kQuitGame;
                                    WriteDebugLine("\pNOT AGAIN!");
                                    BlackTitleScreen();
                                    AutoFadeFrom( 1, false);
                                }
                            } else if ( gameResult == kWinGame)
                            {
                                if ( (globals()->gScenarioWinner & kScenarioWinnerTextMask) != kScenarioWinnerNoText)
                                {
                                    DoMissionDebriefingText(  gTheWindow,
                                        (globals()->gScenarioWinner & kScenarioWinnerTextMask) >> kScenarioWinnerTextShift,
                                        gameLength, gThisScenario->parTime,
                                        GetAdmiralLoss( 0), gThisScenario->parLosses, GetAdmiralKill( 0), gThisScenario->parKills, 100);
                                }

                                fadeColor.red = fadeColor.green = fadeColor.blue = 0;
                                if ( globals()->gOptions & kOptionMusicPlay)
                                {
                                    AutoMusicFadeTo( 60, &fadeColor, FALSE);
                                    StopAndUnloadSong();
                                } else
                                {
                                    AutoFadeTo( 60, &fadeColor, FALSE);
                                }

                                if ( gameResult == kWinGame)
                                {

                                    BlackTitleScreen();
                                    AutoFadeFrom( 1, FALSE);

                                    t = 4002; // normal scrolltext song
                                    if (( globals()->gScenarioWinner & kScenarioWinnerNextMask) ==
                                        kScenarioWinnerNoNext)
                                        t = 4003; // we win but no next level? Play triumph song

                                    if ( (!Ambrosia_Is_Registered()) &&
                                        ( GetChapterNumberFromScenarioNumber( whichScenario) >= 9))
                                    {
                                        globals()->gScenarioWinner = (globals()->gScenarioWinner
                                            & ~kScenarioWinnerNextMask) | kScenarioWinnerNoNext;

                                        SaveStartingLevelPreferences( 10);
                                        DoScrollText( gTheWindow,
                                            6501,
                                            4, kTitleTextScrollWidth, kTitleFontNum, t);
                                    } else
                                    {

                                        if ( GetScenarioEpilogueID( whichScenario) > 0)
                                            DoScrollText( gTheWindow,
                                                GetScenarioEpilogueID( whichScenario),
                                                4, kTitleTextScrollWidth, kTitleFontNum, t);
                                    }
        /*                              DoMissionDebriefing( (WindowPtr) gTheWindow, gameLength, gThisScenario->parTime,
                                        GetAdmiralLoss( 0), gThisScenario->parLosses, GetAdmiralKill( 0), gThisScenario->parKills, 100);
    */

// *********

                                    PlayMovieByName("\p:Ares Data Folder:Next Level", gTheWindow,
                                        true, theDevice);
                                    if ( globals()->gOptions & kOptionMusicIdle)
                                    {
                                        BlackTitleScreen();
                                        StopAndUnloadSong();
                                    }

                                    if ( globals()->gOptions & kOptionReplay)
                                    {
                                        gRandomSeed = saveSeed;
                                    }

                                    globals()->gInputSource.reset();

    //                              whichScenario++;
                                    // whichScenario = GetNextScenarioChapter( whichScenario);
                                    if (( globals()->gScenarioWinner & kScenarioWinnerNextMask) ==
                                        kScenarioWinnerNoNext)
                                        whichScenario = -1;
                                    else
                                    {
                                        whichScenario = GetScenarioNumberFromChapterNumber(
                                            (globals()->gScenarioWinner &
                                            kScenarioWinnerNextMask) >>
                                            kScenarioWinnerNextShift);
                                    }
                                    if (( !( globals()->gOptions & (kOptionAutoPlay | kOptionReplay))) && ( whichScenario <= GetScenarioNumber()) &&
                                        ( whichScenario >= 0))
                                    {
                                        if (( GetChapterNumberFromScenarioNumber( whichScenario) >= 0) &&
                                            ( GetChapterNumberFromScenarioNumber( whichScenario) <= kHackLevelMax) &&
                                            ( GetChapterNumberFromScenarioNumber( whichScenario) <= GetScenarioNumber()))
                                        {
                                            SaveStartingLevelPreferences( GetChapterNumberFromScenarioNumber( whichScenario));
                                        } else whichScenario = -1;
                                    }
                                } else
                                {
                                    BlackTitleScreen();
                                    AutoFadeFrom( 1, false);
                                }
                            }
                        }
                        if ( globals()->gOptions & kOptionMusicPlay) StopAndUnloadSong();
                    } while ( (gameResult != kQuitGame) &&
                        ( GetChapterNumberFromScenarioNumber( whichScenario) <= kHackLevelMax)
                        && ( GetChapterNumberFromScenarioNumber( whichScenario) <= GetScenarioNumber())
                        && ( whichScenario >= 0) &&
                        ( !( globals()->gOptions & (kOptionAutoPlay | kOptionReplay))));
                    if ( globals()->gOptions & kOptionMusicIdle)
                    {
                        LoadSong( kTitleSongID);
                        SetSongVolume( kMaxMusicVolume);
                        PlaySong();
                    }
                }
                globals()->gOptions &= ~(kOptionAutoPlay | kOptionReplay);
                if ( OptionKey()) DebugFileSave( "\p_Poopy");
                DebugFileCleanup();
                break;

            case kMainNetwork:
#if NETSPROCKET_AVAILABLE
                if ( globals()->gOptions & kOptionNetworkOn)
                    StopNetworking();
                globals()->gOptions &= ~kOptionNetworkOn;
                netResult = StartNetworkGameSetup();
                if ( globals()->gOptions & kOptionMusicIdle)
                {
                    StopAndUnloadSong();
                }
                if ( netResult != kCancel)
                {
                    globals()->gOptions |= kOptionNetworkOn;
                    if ( netResult == kHost)
                    {
                        netResult = HostAcceptClientInterface();
                    } else
                    {
                        netResult = ClientWaitInterface();
                    }
                }

                if ( netResult != kCancel)
                {
                    do
                    {
                        ResetNetData();

                        //whichScenario = DoNetLevelInterface();
                        whichScenario = DoTabbedNetLevelInterface();
                        mWriteDebugString("\pLEVEL:");
                        WriteDebugLong( whichScenario);
                        if ( whichScenario >= 0)
                        {
                            whichScenario = GetScenarioNumberFromChapterNumber( whichScenario);
                            mWriteDebugString("\pSCEN:");
                            WriteDebugLong( whichScenario);
                            StopAndUnloadSong();

    /*                      if ( GetScenarioPrologueID( whichScenario) > 0)
                                DoScrollText( (WindowPtr)gTheWindow, GetScenarioPrologueID( whichScenario), \
                                    4, kTitleFontNum);
    */
                            gameResult = kNoGame;
                            fadeColor.red = fadeColor.green = fadeColor.blue = 0;
                            if ( globals()->gOptions & kOptionMusicIdle)
                            {
                                AutoMusicFadeTo( 60, &fadeColor, FALSE);
                                StopAndUnloadSong();
                            } else
                            {
                                AutoFadeTo( 60, &fadeColor, FALSE);
                            }

                            RemoveAllSpaceObjects();
                            globals()->gGameOver = 0;

                            // mission briefing unfades screen

                            if ( globals()->gOptions & kOptionMusicIdle)
                            {
                                LoadSong( 3000);
                                SetSongVolume( kMaxMusicVolume);
                                PlaySong();
                            }

                            WriteDebugDivider();
                            mWriteDebugString("\pConstructing:");
                            WriteDebugLong( whichScenario);
                            if (ConstructScenario( whichScenario))
                            {
    //                          EMERGENCYHACKTEST = true;

                                SetMBarState( false, theDevice);
                                if ( globals()->gOptions & kOptionMusicIdle)
                                {
                                    StopAndUnloadSong();
                                }

                                HideCursor();
                                DrawInstrumentPanel(  gTheWindow);
                                MacShowCursor();
                                if ( globals()->gOptions & kOptionMusicPlay)
                                {
                                    LoadSong( gThisScenario->songID);
                                    SetSongVolume( kMusicVolume);
                                    PlaySong();
                                }
                                ResetLastTime( (gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);

                                if ( globals()->gameRangerInProgress)
                                {
                                    Wrap_GRGameBegin();
                                }

                                gameResult = PlayTheGame( &gameLength);

                                if ( globals()->gameRangerInProgress)
                                {
                                    if ( (globals()->gScenarioWinner & kScenarioWinnerPlayerMask) == globals()->gPlayerAdmiralNumber)
                                    {
                                        Wrap_GRStatScore( 1);
                                        Wrap_GRStatOtherScore( 0);
                                    } else
                                    {
                                        Wrap_GRStatScore( 0);
                                        Wrap_GRStatOtherScore( 1);
                                    }

                                    Wrap_GRGameEnd();
                                }

                                SetNetMinutesPlayed( GetNetMinutesPlayed() + (( gameLength + 30) / 60));
                                SetNetKills( GetNetKills() + GetAdmiralKill( globals()->gPlayerAdmiralNumber));
                                SetNetLosses( GetNetLosses() + GetAdmiralLoss( globals()->gPlayerAdmiralNumber));
                                if ( (globals()->gScenarioWinner &
                                    kScenarioWinnerTextMask) != kScenarioWinnerNoText)
                                {
//                                  DoMissionDebriefingText( (WindowPtr) gTheWindow,
//                                      (globals()->gScenarioWinner & kScenarioWinnerTextMask) >> kScenarioWinnerTextShift,
//                                      -1, -1, -1, -1, -1, -1, -1);
                                    DoMissionDebriefingText(  gTheWindow,
                                        (globals()->gScenarioWinner & kScenarioWinnerTextMask)
                                        >> kScenarioWinnerTextShift,
                                        gameLength, gThisScenario->parTime,
                                        GetAdmiralLoss( globals()->gPlayerAdmiralNumber),
                                        gThisScenario->parLosses,
                                        GetAdmiralKill( globals()->gPlayerAdmiralNumber),
                                        gThisScenario->parKills, 100);

                                }
                                if ( globals()->gOptions & kOptionMusicPlay)
                                {
                                    StopAndUnloadSong();
                                }

                            }
                        }
                    } while (( whichScenario >= 0) && ( NetGameIsOn()));
                }
                if (( globals()->gOptions & kOptionMusicIdle) && ( !SongIsPlaying()))
                {
                    LoadSong( kTitleSongID);
                    SetSongVolume( kMaxMusicVolume);
                    PlaySong();
                }

                globals()->gOptions &= ~kOptionNetworkOn;
                if ( OptionKey()) DebugFileSave( "\p_Poopy");
                DebugFileCleanup();
#endif NETSPROCKET_AVAILABLE
                break;
            case kMainTrain:    // now replay intro
                    DoScrollText( gTheWindow, 5600, 4, kTitleTextScrollWidth,
                        kTitleFontNum, -1);
            /*
                whichScenario = GetScenarioNumberFromChapterNumber( 1);
                StopAndUnloadSong();
                if ( GetScenarioPrologueID( whichScenario) > 0)
                    DoScrollText( (WindowPtr)gTheWindow,
                        GetScenarioPrologueID( whichScenario),
                        4, kTitleTextScrollWidth, kTitleFontNum, 4002);

                gameResult = kNoGame;
                fadeColor.red = fadeColor.green = fadeColor.blue = 0;
                if ( globals()->gOptions & kOptionMusicIdle)
                {
                    AutoMusicFadeTo( 60, &fadeColor, FALSE);
                    StopAndUnloadSong();
                } else
                {
                    AutoFadeTo( 60, &fadeColor, FALSE);
                }

                do
                {
                    RemoveAllSpaceObjects();
                    globals()->gGameOver = 0;

                    // mission briefing unfades screen

                    if ( globals()->gOptions & kOptionMusicIdle)
                    {
                        LoadSong( 3000);
                        SetSongVolume( kMaxMusicVolume);
                        PlaySong();
                    }

                    ConstructScenario( whichScenario);

                    SetMBarState( false, theDevice);
                    if ( globals()->gOptions & kOptionMusicIdle)
                    {
                        StopAndUnloadSong();
                    }

                    HideCursor();
                    DrawInstrumentPanel( (WindowPtr) gTheWindow);
                    MacShowCursor();

                    if ( globals()->gOptions & kOptionMusicPlay)
                    {
                        LoadSong( gThisScenario->songID);
                        SetSongVolume( kMusicVolume);
                        PlaySong();
                    }
                    ResetLastTime( (gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);
                    gameResult = PlayTheGame( &gameLength);

                    if (( !( globals()->gOptions & (kOptionAutoPlay | kOptionRecord | kOptionReplay))) && ( gameResult == kLoseGame))
                    {
                        if ( (globals()->gScenarioWinner & kScenarioWinnerTextMask) != kScenarioWinnerNoText)
                        {
                            DoMissionDebriefingText( (WindowPtr) gTheWindow,
                                (globals()->gScenarioWinner & kScenarioWinnerTextMask) >> kScenarioWinnerTextShift,
                                    -1, -1, -1, -1, -1, -1, -1);
                        }
                        if ( DoPlayAgain( false, false))
                        {
                            gameResult = kRestartGame;
                            WriteDebugLine("\pAGAIN!");
                        }  else
                        {
                            gameResult = kQuitGame;
                            WriteDebugLine("\pNOT AGAIN!");
                            BlackTitleScreen();
                            AutoFadeFrom( 1, false);
                        }
                    } else if ( gameResult == kWinGame)
                    {
                        fadeColor.red = fadeColor.green = fadeColor.blue = 0;
                        if ( globals()->gOptions & kOptionMusicPlay)
                        {
                            AutoMusicFadeTo( 60, &fadeColor, FALSE);
                            StopAndUnloadSong();
                        } else
                        {
                            AutoFadeTo( 60, &fadeColor, FALSE);
                        }

                        if ( gameResult == kWinGame)
                        {

                            BlackTitleScreen();
                            AutoFadeFrom( 1, FALSE);

                            if ( GetScenarioEpilogueID( whichScenario) > 0)
                                DoScrollText( (WindowPtr)gTheWindow,
                                    GetScenarioEpilogueID( whichScenario),
                                    4, kTitleTextScrollWidth, kTitleFontNum, 4002);

                        if ( globals()->gOptions & kOptionMusicIdle)
                            {
                                BlackTitleScreen();
                                StopAndUnloadSong();
                            }

                            if ( globals()->gOptions & kOptionReplay)
                            {
                                gRandomSeed = saveSeed;
                            }
                        }
                    }
                    if ( globals()->gOptions & kOptionMusicPlay) StopAndUnloadSong();
                } while ( (gameResult != kQuitGame) && ( gameResult != kWinGame) &&
                    ( GetChapterNumberFromScenarioNumber( whichScenario) <= kHackLevelMax)
                    && ( GetChapterNumberFromScenarioNumber( whichScenario) <= GetScenarioNumber())
                    && ( whichScenario >= 0) &&
                    ( !( globals()->gOptions & (kOptionAutoPlay | kOptionRecord | kOptionReplay))));
                if ( globals()->gOptions & kOptionMusicIdle)
                {
                    LoadSong( kTitleSongID);
                    SetSongVolume( kMaxMusicVolume);
                    PlaySong();
                }
*/              break;

            case kMainAbout:
//              if ( ShiftKey())
//                  DoMissionDebriefing( (WindowPtr) gTheWindow, 371, 123, 456, 789, 101, 112, 131);
                break;

            case kMainOptions:
            case kNullResult:
                break;
        }
    }


/*  for ( hackCount = 0; hackCount < kTestNumber; hackCount++)
    {
        globals()->gGameTime = 0;
        RemoveAllSpaceObjects();
        DrawInstrumentPanel();
        MacSetRect( &bounds, 0, 0, WORLD_WIDTH, WORLD_HEIGHT);
        CopyOffWorldToRealWorld( (WindowPtr) gTheWindow, &bounds);
        globals()->gGameOver = -1;

        MakeOneScenario();

        ResetLastTime();
        HideCursor();
        PlayTheGame();
        MacShowCursor();
    }

    done = TRUE;
*/
}

short PlayTheGame( long *seconds)   // result 0 = lose, 1 = win, 2 = restart, 3 = quit

{
    unsigned long       decideCycle = 0;
    Str255              string;
    uint64_t            lastTime, thisTime, scrapTime = { 0 }, netTime;
    Rect                clipRect;
    long                    unitsToDo = 0, unitsPassed = 0, unitsDone = 0,
                            l1, l2, newGameTime = 0, lastclicktime = 0,
                            additionalSeconds = 0;
    KeyMap              keyMap = { }, lastKeyMap;
    Boolean             playerPaused = FALSE, mouseDown = FALSE,
                            enteringMessage = false,
                            afEntering = false, demoKey = false, newKeyMap = false, commandAndQ = false;
    unsigned long       keyDataSize = 0, scenarioCheckTime = 0;
    Rect                    playAreaRect;
    short                   result = -1;
    EventRecord         theEvent;
//  long                hacktc = TickCount(), hacktcsamplesize = 4, hacktcsamplecount = 0;

    DebugFileCleanup();
    DebugFileInit();
    DebugFileAppendString("\p<START DEBUG FILE>\r\r");

    commandAndQ = BothCommandAndQ();

    if ( globals()->gOptions & kOptionNetworkOn)
    {
#if NETSPROCKET_AVAILABLE
        ResetGotMessages( 0x7fffffff);
        if ( !JumpstartLatencyQueue(globals()->gGameTime, kDecideEveryCycles))
        {
//          ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because ", "\pJumpstart failed.", nil, nil, -1, -1, -1, -1, __FILE__, 1);
            mAbortNetworkGame
            DeclareWinner( -1, -1, 300);
        }
#endif NETSPROCKET_AVAILABLE
    }
    SetSpriteCursorTable( 500);
    ShowSpriteCursor( true);
    ResetHintLine();
    MacSetRect( &playAreaRect, CLIP_LEFT, CLIP_TOP, CLIP_RIGHT, CLIP_BOTTOM);

    globals()->gLastKeys = globals()->gTheseKeys = 0;

    HideCursor();
    Microseconds( &lastTime);

    unitsPassed = 0;

    WriteDebugLine("\pEntr Game");
    WriteDebugLong( gRandomSeed);
    DebugFileAppendString("\p---NEW GAME---");

    DebugFileAppendString("\pTime\tFile\tLine\t#\r");

    if ((globals()->gOptions & kOptionReplay)
            && (globals()->gInputSource.get() != nil)) {
        // pass
    } else
    {
        if ( globals()->user_is_scum)
        {
            ShowErrorAny( eQuitErr, -1, "\pCan't continue because an error of type "
                "-2021 occurred; contact Ambrosia Software for a valid "
                "registration code.", "\p", nil, nil, -1, -1, -1, -1, __FILE__, 1);
            ExitToShell();
        }
    }
    netTime = 0;
//  EMERGENCYHACKTEST = false;

    CheckScenarioConditions( 0);

/*  ExecuteCheat( 1, 0);
    ExecuteCheat( 1, 1);

    ExecuteCheat( 3, 0);
    ExecuteCheat( 3, 1);

    ExecuteCheat( 3, 0);
    ExecuteCheat( 3, 1);

    ExecuteCheat( 6, 0);
    ExecuteCheat( 6, 1);

    ExecuteCheat( 2, 0);
    ExecuteCheat( 2, 1);
*/
#if kProfiling_On
    if (ProfilerInit( collectSummary, bestTimeBase, 256, 8) != noErr)
        DebugStr("\pprofiler init error");
    ProfilerSetStatus( true);
#endif

    while (( globals()->gGameOver <= 0 ) && ( !globals()->returnToMain))
    {

        globals()->gFrameCount = 0;
        gLastTick = TickCount();

        SetLongRect( &clipRect, CLIP_LEFT, CLIP_TOP, CLIP_RIGHT, CLIP_BOTTOM);
        while (globals()->gGameOver <= 0)
        {
            EraseSpriteCursorSprite();
            EraseSpriteTable();
            EraseAllLabels();
            EraseSectorLines();
            PrepareToMoveScrollStars();
            EraseSite();

            while ( unitsPassed == 0)
            {
//              MyWideAdd( (wide *)&globals()->gLastTime, (wide *)&netTime);
                netTime = 0;
                Microseconds( &thisTime);
                scrapTime = thisTime;
                thisTime -= globals()->gLastTime;
                newGameTime = (thisTime / kTimeUnit) + ((gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);
//              newGameTime = globals()->gGameTime + Randomize( 7) + 1;//Randomize( kDecideEveryCycles);
#ifdef kConstantRate
                newGameTime = globals()->gGameTime + 1;
#endif
//#ifndef powerc
#ifdef powercxx
                if ( globals()->gOptions & kOptionNetworkOn)
                {
#endif
                    if ( (newGameTime - globals()->gGameTime) > k68KMaxFrameSkip)
                    {
                        newGameTime = globals()->gGameTime + k68KMaxFrameSkip;
                        l1 = newGameTime - ((gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);
                        l2 = kTimeUnit;
                        MyWideMul(l1, l2, reinterpret_cast<int64_t*>(&thisTime));
                        globals()->gLastTime = scrapTime;
                        globals()->gLastTime -= thisTime;
                    }
#ifdef powercxx
                }
#endif

//#endif
//              if ( (!(globals()->gOptions & kOptionReplay)) &&
//                  (mSlowMotionKey( keyMap)))
/*              if ( (mSlowMotionKey( keyMap)))
                {
                    demoKey = true;
                    if ( unitsPassed < 3) Pause( 3);
                    newGameTime = globals()->gGameTime + 1;
                    l1 = newGameTime - ((gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);
                    l2 = kTimeUnit;
                    MyWideMul( l1, l2, (wide *)&thisTime);
                    globals()->gLastTime = scrapTime;
                    WideSubtract( (wide *)&globals()->gLastTime, (wide *)&thisTime);
                } else */if (((( globals()->gOptions & kOptionSubstituteFKeys) ?
                    ( mNOFFastMotionKey( keyMap)):( mFastMotionKey( keyMap)))) &&
                    (!enteringMessage))
                {
                    demoKey = true;
                    newGameTime = globals()->gGameTime + 12;
                    l1 = newGameTime - ((gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);
                    l2 = kTimeUnit;
                    MyWideMul(l1, l2, reinterpret_cast<int64_t*>(&thisTime));
                    globals()->gLastTime = scrapTime;
                    globals()->gLastTime -= thisTime;
                }/* else
                {
                    newGameTime = globals()->gGameTime + Randomize( 9) + 1;
                    l1 = newGameTime - ((gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);
                    l2 = kTimeUnit;
                    MyWideMul( l1, l2, (wide *)&thisTime);
                    globals()->gLastTime = scrapTime;
                    WideSubtract( (wide *)&globals()->gLastTime, (wide *)&thisTime);
                }*/
//              if (( newGameTime - globals()->gGameTime) > 6) newGameTime = globals()->gGameTime + 6;

                if ( newGameTime >= kMaxGameTime)
                {
                    l1 = kTimeUnit;
                    l2 = newGameTime - kMaxGameTime;
                    MyWideMul(l1, l2, reinterpret_cast<int64_t*>(&thisTime));
                    globals()->gLastTime = scrapTime;
                    globals()->gLastTime -= thisTime;
                    additionalSeconds += ( newGameTime / 60);
                    newGameTime -= kMaxGameTime;
//                  globals()->gGameTime -= kMaxGameTime;
                }
                if ( globals()->gGameTime <= newGameTime)
                    unitsDone = unitsPassed = newGameTime - globals()->gGameTime;
                else
                    unitsDone = unitsPassed = newGameTime - ( globals()->gGameTime - kMaxGameTime);
/*              if ( globals()->gOptions & ( kOptionReplay | kOptionRecord))
                {
                    if ( unitsDone > kDecideEveryCycles)
                    {
                        globals()->gGameTime = newGameTime - kDecideEveryCycles;
                        unitsDone = unitsPassed = kDecideEveryCycles;
                    }
                }
*/
//              WideSubtract( (wide *)&thisTime, (wide *)&lastTime);
//              unitsDone = unitsPassed = thisTime.lo / kTimeUnit;
//              thisTick = TickCount();
//              unitsDone = unitsPassed = thisTick - lastTick;
//              newGameTime = TickCount() - globals()->gGameStartTime;
//              unitsDone = unitsPassed = newGameTime - globals()->gGameTime;
            }
//          Microseconds( &lastTime);   // don't activate
//          lastTick = thisTick;

            if ( playerPaused)
            {
                playerPaused = false;
                unitsDone = unitsPassed = 0;
                newGameTime = globals()->gGameTime;
                l1 = newGameTime - ((gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);
                l2 = kTimeUnit;
                MyWideMul(l1, l2, reinterpret_cast<int64_t*>(&thisTime));
                globals()->gLastTime = scrapTime;
                globals()->gLastTime -= thisTime;
            }

            if ( globals()->gGameOver < 0)
            {
                globals()->gGameOver += unitsPassed;
                if ( globals()->gGameOver == 0)
                    globals()->gGameOver = 1;
            }

            while ( unitsPassed > 0)
            {
                unitsToDo = unitsPassed;
                if ( unitsToDo > kMaxTimePerCycle) unitsToDo = kMaxTimePerCycle;
                if ( (decideCycle + unitsToDo) > kDecideEveryCycles)
                    unitsToDo = kDecideEveryCycles - decideCycle;
                decideCycle += unitsToDo;

                if ( unitsToDo > 0) // executed arbitrarily, but at least once every kDecideEveryCycles
                {

                    MoveScrollStars( unitsToDo);
                    MoveSpaceObjects(*gSpaceObjectData, kMaxSpaceObject, unitsToDo);

//                  WriteDebugLine("\pMove");
//                  WriteDebugLong( decideCycle);
//                  WriteDebugLong( unitsToDo);
                }

                globals()->gGameTime += unitsToDo;
                if ( globals()->gGameTime >= kMaxGameTime) globals()->gGameTime -= kMaxGameTime;
                if ( decideCycle == kDecideEveryCycles) // everything in here gets executed once every kDecideEveryCycles
                {
//                  MoveScrollStars( kDecideEveryCycles);
//                  MoveSpaceObjects( (spaceObjectType *)*gSpaceObjectData, kMaxSpaceObject,
//                                  kDecideEveryCycles);

                    playerPaused = FALSE;


                    NonplayerShipThink( kDecideEveryCycles);
                    AdmiralThink();
                    ExecuteActionQueue( kDecideEveryCycles);

                    if ( globals()->gOptions & kOptionReplay)
                    {
                        uint32_t keys;
                        if (!globals()->gInputSource->next(&keys)) {
                            globals()->gGameOver = 1;
                        }

                        if ( !playerPaused) {
                            playerPaused = PlayerShipGetKeys(
                                    kDecideEveryCycles, keys, &enteringMessage);
                        } else {
                            PlayerShipGetKeys( kDecideEveryCycles, keys, &enteringMessage);
                        }
                    } else
                    {
                        if ( !playerPaused) playerPaused =
                            PlayerShipGetKeys( kDecideEveryCycles,
                                0xffffffff, &enteringMessage);
                        else PlayerShipGetKeys( kDecideEveryCycles, 0xffffffff,
                            &enteringMessage);
                    }

                    if ( Button())
                    {
                        if ( globals()->gOptions & kOptionReplay)
                        {
                            result = kQuitGame;
                            globals()->gGameOver = 1;
                        } else
                        {
                            if ( !mouseDown)
                            {
                                if ( !(globals()->gOptions & ( kOptionAutoPlay | kOptionReplay)))
                                {
                                    if ((( globals()->gGameTime - lastclicktime)) <= GetDblTime())
                                    {
                                        InstrumentsHandleDoubleClick();
                                        lastclicktime -= GetDblTime();
                                    } else
                                    {
                                        InstrumentsHandleClick();
                                        lastclicktime = globals()->gGameTime;
                                    }
                                }
                                mouseDown = TRUE;
                            } else
                            {
                                InstrumentsHandleMouseStillDown();
                            }
                        }
                    } else if ( mouseDown)
                    {
                        mouseDown = FALSE;
                        InstrumentsHandleMouseUp();
                    }

                    if ( globals()->gOptions & kOptionNetworkOn)
                    {
//#ifdef kDemoTimeLimit
if ( (!Ambrosia_Is_Registered()) || ( GetOpponentIsUnregistered()))
{
                        if ( globals()->gGameTime > kTimeLimitWarning)
                        {
                            if ( globals()->gGameTime > kTimeLimit)
                            {
                                result = kQuitGame;
                                globals()->gGameOver = 1;
                                DeclareWinner( -1, -1, 302);
                            } else if ( !GetHaveSeenUnregisteredTimeLimitWarning())
                            {
                                SetHaveSeenUnregisteredTimeLimitWarning( true);
                                StartLongMessage( 6123, 6123);

                            }
                        }
}
//#endif
#if NETSPROCKET_AVAILABLE
                        l2 = -1;
                        waitingForMessage = false;
                        resendTime = kResendRequestFirstTime;
                        l1 = l3 = TickCount();
                        afEntering = enteringMessage;
                        Microseconds( &thisTime);   // don't activate
//                      TickleOutgoingMessage( false);
                        if ( SendInGameMessage( globals()->gGameTime + gNetLatency))
                        {
                            do
                            {
                                GetKeys( keyMap);
                                HandleTextMessageKeys( keyMap, lastMessageKeyMap, &afEntering);
                                for ( scratch = 0; scratch < 4; scratch++)
                                    lastMessageKeyMap[scratch] = keyMap[scratch];
                                if ( !ProcessInGameMessages( globals()->gGameTime, &pauseLevel))
                                {
//                                  ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because ", "\pProcessInGamesMessages failed.", nil, nil, -1, -1, -1, -1, __FILE__, 2);
//                                  mAbortNetworkGame
                                    if ( globals()->gGameOver == 0)
                                    {
                                        globals()->gGameOver = 1;
                                        result = kRestartGame;
                                    }

                                    mWriteDebugString("\pPROC ERR");
                                    if ( (globals()->gScenarioWinner &
                                        kScenarioWinnerTextMask) == kScenarioWinnerNoText)
                                        DeclareWinner( -1, -1,  6004);
                                }

                                // *** START HANDLING NETWORK PAUSE ***
                                if ((mRestartResumeKey( keyMap)) || ((!commandAndQ) &&
                                    ( mQuitKeys( keyMap))))
                                {
                                    if ( blinkOn)
                                    {
                                        StopPauseIndicator( string);
                                        blinkOn = false;
                                    }
                                    if ( !( globals()->gOptions & (kOptionAutoPlay | kOptionReplay)))
                                    {
                                        if ( globals()->gOptions & kOptionNetworkOn)
                                        {
                                            if ( !SendInGameBasicMessage( 0, eStartPauseMessage,
                                                true, false))
                                            {
//                                              ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because ", "\pStartPause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 3);
                                                mAbortNetworkGame
                                                mWriteDebugString("\pPSE SEND ERR");
                                                DeclareWinner( -1, -1, 300);
                                            }
                                        }

                                        MacShowCursor();
                                        RestoreOriginalColors();
                                        switch ( DoPlayAgain( true,
                                            (gThisScenario->startTime & kScenario_IsTraining_Bit) ? (true):(false)))
                                        {
                                            case 0: // quit
//                                              ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p the user chose to quit.", nil, nil, -1, -1, -1, -1, __FILE__, 33);
                                                result = kQuitGame;
                                                globals()->gGameOver = 1;
                                                if ( CommandKey())
                                                    globals()->gScenarioWinner = globals()->gPlayerAdmiralNumber;
                                                globals()->gScenarioWinner |= kScenarioWinnerNoNext | kScenarioWinnerNoText;
                                                break;

                                            case 1: // restart
//                                              ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p the user chose to restart.", nil, nil, -1, -1, -1, -1, __FILE__, 34);
                                                result = kRestartGame;
                                                globals()->gGameOver = 1;
                                                if ( CommandKey())
                                                    globals()->gScenarioWinner = globals()->gPlayerAdmiralNumber;
                                                globals()->gScenarioWinner |= kScenarioWinnerNoNext | kScenarioWinnerNoText;
                                                break;

                                            case 2: // resume
                                                break;

                                            case 3: // skip
                                                result = kWinGame;
                                                globals()->gGameOver = 1;
                                                globals()->gScenarioWinner =  globals()->gPlayerAdmiralNumber |
                                                    (( GetChapterNumberFromScenarioNumber(
                                                        globals()->gThisScenarioNumber)
                                                            +1) << kScenarioWinnerNextShift) |
                                                    kScenarioWinnerNoText;
                                                break;
                                        }
                                        CopyOffWorldToRealWorld( gTheWindow, &playAreaRect);
                                        HideCursor();
                                        playerPaused = true;
                                        if ( globals()->gOptions & kOptionNetworkOn)
                                        {
                                            if ( !SendInGameBasicMessage( 0, eEndPauseMessage,
                                                true, false))
                                            {
//                                              ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because ", "\pEndPause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 4);
                                                mAbortNetworkGame
                                                mWriteDebugString("\pPSE END ERR");
                                                DeclareWinner( -1, -1, 300);
                                            }
                                            mWriteDebugString("\pSent End Pause");
                                        }
                                    } else
                                    {
//                                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p this isn't a real game and the user hit ESC or COMMAND-Q.", nil, nil, -1, -1, -1, -1, __FILE__, 35);
                                        result = kQuitGame;
                                        globals()->gGameOver = 1;
                                        if ( CommandKey())
                                            globals()->gScenarioWinner = globals()->gPlayerAdmiralNumber;
                                        globals()->gScenarioWinner |= kScenarioWinnerNoNext | kScenarioWinnerNoText;
                                    }
                                    l1 = TickCount();
                                }
                                // *** END HANDLING NETWORK PAUSE ***

                                if (( pauseLevel > 0) && ( l2 < 0))
                                {
                                    if ( blinkOn)
                                    {
                                        StopPauseIndicator( string);
                                    }
                                    blinkOn = false;
                                    playerPaused = true;
                                    l2 = TickCount();
                                }// else if  ( pauseLevel <= 0) l2 = -1;

                                /*
                                if (( pauseLevel > 0) && (!wasPaused))
                                {
                                    playerPaused = true;
                                    wasPaused = true;
                                    blinkOn = false;
                                    l2 = TickCount();
                                }
                                if ( (wasPaused) && (pauseLevel <= 0))
                                {
                                    wasPaused = false;
                                }
                                */

                                if ( pauseLevel > 0)
                                {
                                    l1 = l3 = TickCount();
                                    if ( (TickCount() - l2) > 20)
                                    {
                                        if ( blinkOn)
                                        {
//                                          GetIndString( string, 3100, 12);
                                            StopPauseIndicator( string);
                                            blinkOn = false;
                                        } else
                                        {
                                            GetIndString( string, 3100, 12);
                                            RestoreOriginalColors();
                                            StartPauseIndicator( string, Randomize(16));
                                            blinkOn = true;
                                        }
                                        l2 = TickCount();
                                    }
                                }

                                if ( (IAmHosting()) && ( GetNumberOfPlayers() < 2))
                                {
                                    if ( globals()->gGameOver == 0)
                                    {
//                                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p I am the host and there are fewer than 2 players.", nil, nil, -1, -1, -1, -1, __FILE__, 22);
                                        DeclareWinner( globals()->gPlayerAdmiralNumber, -1, 6004);
                                        globals()->gGameOver = 1;
                                    }
                                    result = kQuitGame;
                                }

                                if ( !NetGameIsOn())
                                {
                                    if ( globals()->gGameOver == 0)
                                    {
//                                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p there is no network game.", nil, nil, -1, -1, -1, -1, __FILE__, 23);
                                        DeclareWinner( globals()->gPlayerAdmiralNumber, -1, 6004);
                                        globals()->gGameOver = 1;
                                    }
                                    result = kQuitGame;
                                } else
                                {
                                    if ((( TickCount() - l3) > 60) && ( pauseLevel <= 0))
                                    {
                                        if ( l2 < 0)
                                        {
                                            l2 = TickCount();
                                            if ( blinkOn)
                                            {
                                                StopPauseIndicator( string);
                                                blinkOn = false;
                                            } else
                                            {
                                                GetIndString( string, 3100, 13);
                                                RestoreOriginalColors();
                                                StartPauseIndicator( string, Randomize(16));
                                                blinkOn = true;
                                            }

                                        }
                                        if (( TickCount() - l2) > 20)
                                        {
                                            if (blinkOn)
                                            {
//                                              GetIndString( string, 3100, 13);
                                                StopPauseIndicator( string);
                                                blinkOn = false;
                                            } else
                                            {
                                                GetIndString( string, 3100, 13);
                                                RestoreOriginalColors();
                                                StartPauseIndicator( string, Randomize(16));
                                                blinkOn = true;
                                            }
                                            l2 = TickCount();
                                        }
                                    }
                                    scratch = TickCount() - l1;
                                    if ( scratch > GetResendDelay())
                                    {
                                        SendResendMessage( globals()->gGameTime);
                                        l1 = TickCount();

                                        if ( GetRegisteredSetting() > 0)
                                            resendTime = GetResendDelay() * 20;
                                        else
                                            resendTime = GetResendDelay() * 2;
                                    }
                                }
                            } while (((pauseLevel > 0) || ( !GotAllMessages())) &&
                                ( result == -1));
                        } else
                        {
//                          ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p SendInGameMessage failed.", nil, nil, -1, -1, -1, -1, __FILE__, 5);
                            mAbortNetworkGame
                            mWriteDebugString("\pSEND MSG ERR");
                            DeclareWinner( -1, -1, 300);
                        }

                        Microseconds( &netTime);    // don't activate
                        netTime -= thisTime;
                        if ( netTime > kTimeUnit)
                        {
                            netCount++;
                            if ( netCount > kFractionalLagCorrectTolerance)
                            {
                                playerPaused = true;
                                netCount = 0;
                            }
                        } else netCount = 0;

                        if ( blinkOn)
                        {
                            StopPauseIndicator( string);
                            blinkOn = false;
                        }
                        ExecuteInGameData();
                        ResetGotMessages( globals()->gGameTime);
//                      UseNextLatency();
#endif NETSPROCKET_AVAILABLE
                    }

                    CollideSpaceObjects(*gSpaceObjectData, kMaxSpaceObject);
                    decideCycle  = 0;
                    scenarioCheckTime++;
                    if ( scenarioCheckTime == 30)
                    {
                        scenarioCheckTime = 0;
                        CheckScenarioConditions( 0);
                    }
//                  WriteDebugLine("\pDecide");
                }
                unitsPassed -= unitsToDo;
            }

//          CollideSpaceObjects( (spaceObjectType *)*gSpaceObjectData, kMaxSpaceObject);
            newKeyMap = false;
            for ( l1 = 0; l1 < 4; l1++)
            {
                lastKeyMap[l1] = keyMap[l1];
            }
            GetKeys( keyMap);
            for ( l1 = 0; l1 < 4; l1++)
            {
#if TARGET_OS_MAC
                if ( lastKeyMap[l1] != keyMap[l1]) newKeyMap = true;
#else
                if ( lastKeyMap[l1].bigEndianValue != keyMap[l1].bigEndianValue) newKeyMap = true;
#endif TARGET_OS_MAC
            }
            if ( !(globals()->gOptions & kOptionNetworkOn))
            {
#if NETSPROCKET_AVAILABLE
                afEntering = enteringMessage;
                HandleTextMessageKeys( keyMap, lastMessageKeyMap, &afEntering);
                for ( scratch = 0; scratch < 4; scratch++)
                    lastMessageKeyMap[scratch] = keyMap[scratch];
#endif
            }
            if (
                //(!(globals()->gOptions & kOptionReplay)) &&
                (mPauseKey( keyMap)))
            {
                mWriteDebugString("\pThis Time:");
                WriteDebugLong( newGameTime);
                WriteDebugLong( globals()->gGameTime);
#if NETSPROCKET_AVAILABLE
                DebugMessageQueue();
#endif
                RestoreOriginalColors();
                GetIndString( string, 3100, 11);
                if ( globals()->gOptions & kOptionNetworkOn)
                {
#if NETSPROCKET_AVAILABLE
                    if ( !SendInGameBasicMessage( 0, eStartPauseMessage, true,
                        false))
                    {
//                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p StartPause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 6);
                        mAbortNetworkGame
                        DeclareWinner( -1, -1, 300);
                    }
#endif NETSPROCKET_AVAILABLE
                }

                PlayVolumeSound( kComputerBeep4, kMaxSoundVolume, kShortPersistence, kMustPlaySound);
                while ( (mPauseKey( keyMap)) && (!(mReturnKey(keyMap))))
                {

                    l1 = TickCount();
                    StartPauseIndicator( string, Randomize( 16));
                    playerPaused = false;
                    while ( (mPauseKey( keyMap)) && (!(mReturnKey(keyMap)))
                        && ( (TickCount() - l1) < 20))
                    {
                        GetKeys( keyMap);
                    }


                    l1 = TickCount();
                    StopPauseIndicator( string);
                    playerPaused = true;
                    while ( (mPauseKey( keyMap)) && (!(mReturnKey(keyMap)))
                        && ( (TickCount() - l1) < 20))
                    {
                        GetKeys( keyMap);
                        if ( CommandKey())
                        {
                            Ares_WaitNextEvent (everyEvent, &theEvent, 3, nil);
                        }
                    }

                }
                if ( globals()->gOptions & kOptionNetworkOn)
                {
#if NETSPROCKET_AVAILABLE
                    if ( !SendInGameBasicMessage( 0, eEndPauseMessage, true,
                        false))
                    {
//                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p Endpause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 7);
                        mAbortNetworkGame
                        DeclareWinner( -1, -1, 300);
                    }
                    mWriteDebugString("\pSent End Pause");
#endif
                }
            }

            if ((!( globals()->gOptions & kOptionNetworkOn)) &&
                ( !(globals()->gOptions & kOptionReplay)) &&
                ((mRestartResumeKey( keyMap)) || ((!commandAndQ) && ( mQuitKeys( keyMap)))))
            {
                if ( !( globals()->gOptions & (kOptionAutoPlay | kOptionReplay)))
                {
                    if ( globals()->gOptions & kOptionNetworkOn)
                    {
#if NETSPROCKET_AVAILABLE
                        if ( !SendInGameBasicMessage( 0, eStartPauseMessage, true,
                            false))
                        {
//                          ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p Startpause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 8);
                            mAbortNetworkGame
                            DeclareWinner( -1, -1, 300);
                        }
#endif NETSPROCKET_AVAILABLE
                    }

                    RestoreOriginalColors();
                    MacShowCursor();
                    switch ( DoPlayAgain( true,
                                            (gThisScenario->startTime & kScenario_IsTraining_Bit) ? (true):(false)))
                    {
                        case 0: // quit
//                          ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p the user chose to quit.", nil, nil, -1, -1, -1, -1, __FILE__, 35);
                            result = kQuitGame;
                            globals()->gGameOver = 1;
                            if ( CommandKey())
                                globals()->gScenarioWinner = globals()->gPlayerAdmiralNumber;
                            globals()->gScenarioWinner |= kScenarioWinnerNoNext | kScenarioWinnerNoText;
                            break;

                        case 1: // restart
//                          ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p the user chose to restart.", nil, nil, -1, -1, -1, -1, __FILE__, 36);
                            result = kRestartGame;
                            globals()->gGameOver = 1;
                            if ( CommandKey())
                                globals()->gScenarioWinner = globals()->gPlayerAdmiralNumber;
                            globals()->gScenarioWinner |= kScenarioWinnerNoNext | kScenarioWinnerNoText;
                            break;

                        case 2: // resume
                            break;

                        case 3: // skip
                            result = kWinGame;
                            globals()->gGameOver = 1;
                            globals()->gScenarioWinner =  globals()->gPlayerAdmiralNumber |
                                (( GetChapterNumberFromScenarioNumber(globals()->gThisScenarioNumber)+1)
                                    << kScenarioWinnerNextShift) |
                                kScenarioWinnerNoText;
                            break;
                    }
                    CopyOffWorldToRealWorld( gTheWindow, &playAreaRect);
                    HideCursor();
                    playerPaused = true;
                    if ( globals()->gOptions & kOptionNetworkOn)
                    {
#if NETSPROCKET_AVAILABLE
                        if ( !SendInGameBasicMessage( 0, eEndPauseMessage, true,
                            false))
                        {
//                          ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p EndPause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 8);
                            mAbortNetworkGame
                            DeclareWinner( -1, -1, 300);
                        }
                        mWriteDebugString("\pSent End Pause");
#endif NETSPROCKET_AVAILABLE
                    }
                } else
                {
//                  ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p this isn't a real game and the user hit ESC or COMMAND-Q.", nil, nil, -1, -1, -1, -1, __FILE__, 37);
                    result = kQuitGame;
                    globals()->gGameOver = 1;
                    if ( CommandKey())
                        globals()->gScenarioWinner = globals()->gPlayerAdmiralNumber;
                    globals()->gScenarioWinner |= kScenarioWinnerNoNext | kScenarioWinnerNoText;
                }
            }

            if ((!(globals()->gOptions & kOptionReplay)) &&
                (((!afEntering)&&( mHelpKey( keyMap)))))

            {
                if ( globals()->gOptions & kOptionNetworkOn)
                {
#if NETSPROCKET_AVAILABLE
                    if ( !SendInGameBasicMessage( 0, eStartPauseMessage, true,
                        false))
                    {
//                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p StartPause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 9);
                        mAbortNetworkGame
                        DeclareWinner( -1, -1, 300);
                    }
#endif NETSPROCKET_AVAILABLE
                }
                RestoreOriginalColors();
                MacShowCursor();
                DoHelpScreen();
                HideCursor();
                CopyOffWorldToRealWorld( gTheWindow, &playAreaRect);
                playerPaused = true;
                if ( globals()->gOptions & kOptionNetworkOn)
                {
#if NETSPROCKET_AVAILABLE
                    if ( !SendInGameBasicMessage( 0, eEndPauseMessage, true,
                        false))
                    {
//                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p Endpause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 10);
                        mAbortNetworkGame
                        DeclareWinner( -1, -1, 300);
                    }
                    mWriteDebugString("\pSent End Pause");
#endif NETSPROCKET_AVAILABLE
                }
            }
            if ((globals()->gOptions & kOptionNetworkOn) &&
#if NETSPROCKET_AVAILABLE
            ( IAmHosting()) &&
#endif NETSPROCKET_AVAILABLE
                    (((!afEntering)&&( mNetSettingsKey( keyMap)))))
            {
                if ( globals()->gOptions & kOptionNetworkOn)
                {
#if NETSPROCKET_AVAILABLE
                    if ( !SendInGameBasicMessage( 0, eStartPauseMessage, true,
                        false))
                    {
//                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p Startpause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 11);
                        mAbortNetworkGame
                        DeclareWinner( -1, -1, 300);
                    }
#endif NETSPROCKET_AVAILABLE
                }
                RestoreOriginalColors();
                MacShowCursor();
#if NETSPROCKET_AVAILABLE
                DoNetSettings();
#endif NETSPROCKET_AVAILABLE
                HideCursor();
                CopyOffWorldToRealWorld( gTheWindow, &playAreaRect);
                playerPaused = true;
                if ( globals()->gOptions & kOptionNetworkOn)
                {
#if NETSPROCKET_AVAILABLE
                    if ( !SendInGameBasicMessage( 0, eEndPauseMessage, true,
                        false))
                    {
//                      ShowErrorAny( eContinueOnlyErr, -1, "\pEnding the game because", "\p EndPause failed.", nil, nil, -1, -1, -1, -1, __FILE__, 12);
                        mAbortNetworkGame
                        DeclareWinner( -1, -1, 300);
                    }
                    mWriteDebugString("\pSent End Pause");
#endif NETSPROCKET_AVAILABLE
                }
            }

            if (((!(globals()->gOptions & kOptionReplay)) &&
            ((!afEntering) && ( ( mVolumeDownKey( keyMap) && !mVolumeDownKey( lastKeyMap))))))
            {
                if ( globals()->gSoundVolume > 0) globals()->gSoundVolume--;
                if ( globals()->gOptions & kOptionMusicPlay)
                {
                    SetSongVolume( kMusicVolume);
                }
            }

            if (((!(globals()->gOptions & kOptionReplay)) &&
                ((!afEntering) && ( mVolumeUpKey( keyMap) && !mVolumeUpKey( lastKeyMap)))))
            {
                if ( globals()->gSoundVolume < kMaxVolumePreference) globals()->gSoundVolume++;
                if ( globals()->gOptions & kOptionMusicPlay)
                {
                    SetSongVolume( kMusicVolume);
                }
            }

            if ((!(globals()->gOptions & kOptionReplay)) && (!afEntering) &&
                ( mActionMusicKey( keyMap)) && ( !mActionMusicKey( lastKeyMap)))
            {
                if ( globals()->gOptions & kOptionMusicPlay)
                {
                    ToggleSong();
                }
            }

#if TARGET_OS_MAC
            keyMap[3] &= ~0x80; // mask out power key
            keyMap[1] &= ~0x02; // mask out caps lock key
            if ( (globals()->gOptions & kOptionReplay) && (!demoKey) &&
                (!newKeyMap) &&
                ((keyMap[0] != 0) || ( keyMap[1] != 0) || ( keyMap[2] != 0) ||
                (keyMap[3] != 0)))
#else
            keyMap[3].bigEndianValue &= EndianU32_NtoB(~0x80);  // mask out power key
            keyMap[1].bigEndianValue &= EndianU32_NtoB(~0x02);  // mask out caps lock key
            if ( (globals()->gOptions & kOptionReplay) && (!demoKey) &&
                (!newKeyMap) &&
                ((keyMap[0].bigEndianValue != 0) || ( keyMap[1].bigEndianValue != 0) ||
                ( keyMap[2].bigEndianValue != 0) ||
                (keyMap[3].bigEndianValue != 0)))
#endif TARGET_OS_MAC
            {
                result = kQuitGame;
                globals()->gGameOver = 1;
            }
            demoKey = false;

//          if ( globals()->gGameTime > newGameTime) { WriteDebugLong( newGameTime); globals()->gGameTime = newGameTime;}

            MiniComputerHandleNull( unitsDone);

            ClipToCurrentLongMessage();
            SetLongRect( &clipRect, CLIP_LEFT, CLIP_TOP, CLIP_RIGHT, CLIP_BOTTOM);
//          hacktc = TickCount();
//          hacktcsamplecount++;
            DrawScrollStars( TRUE);
            DrawCurrentLongMessage( unitsDone);

            DrawSectorLines();
            DrawAllBeams();
            DrawSpriteTableInOffWorld( &clipRect);
            UpdateAllLabelPositions( unitsDone);
            DrawAllLabels();
            DrawSite();
            SetLongRect( &clipRect, 0, 0, WORLD_WIDTH, WORLD_HEIGHT);
            DrawSpriteCursorSprite( &clipRect);
//          DrawCurrentLongMessage();

            if (( globals()->gOptions & kOptionQDOnly))
            {
                RGBColor    hackcolor = { 32765, 32765, 32765};

                DrawOutlinedString( "\pkOptionQDOnly", &hackcolor); //DrawString( tempString);
                ShowSpriteCursorSprite();
                DrawAllBeams();
                DontShowScrollStars();
                CopyOffWorldToRealWorld( gTheWindow, &playAreaRect);
            } else
            {
                ShowSpriteCursorSprite();
                ShowSpriteTable();
                ShowAllLabels();
//              DrawAllBeams();
                ShowAllBeams();
                ShowScrollStars( TRUE);
                ShowSectorLines();
                ShowSite();
                CopyOffWorldToRealWorld( gTheWindow, &playAreaRect);

            }
//          if ( hacktcsamplecount > hacktcsamplesize)
//          {
//              hacktcsamplecount = 0;
//              WriteDebugLong( TickCount() - hacktc);
//          }
            //PixMapTest();

            DrawMessageScreen( unitsDone);
            UpdateRadar( unitsDone);
            UpdateBooleanColorAnimation( unitsDone);
//          CheckScenarioConditions( unitsDone);
            globals()->gFrameCount++;
            MainLoopIterationComplete(globals()->gGameTime);
        }
    }
    exit(0);  // Temporary: exit after finishing demo.

#if kProfiling_On
    ProfilerSetStatus( false);
    if (ProfilerDump("\pares_profile") != noErr)
        DebugStr("\pprofiler dump error");
    ProfilerTerm();
#endif

    if ( globals()->gOptions & kOptionNetworkOn)
    {
#if NETSPROCKET_AVAILABLE
        if ( IAmHosting())
        {
//          StopNetworking();
            if ( !SendInGameMiscLongMessage( 0, eStopPlayingMessage,
                globals()->gScenarioWinner, true, false))
            {
                mAbortNetworkGame
            }
        } else
        {
//          StopNetworking();
            if ( !SendInGameMiscLongMessage( 0, eStopPlayingMessage,
                globals()->gScenarioWinner, true, false))
            {
                mAbortNetworkGame
            }
        }
#endif NETSPROCKET_AVAILABLE
    }

    WriteDebugLine("\p<GameOver");
    WriteDebugLong( keyDataSize);
    MacShowCursor();

    Microseconds( &thisTime);
    thisTime -= globals()->gLastTime;
    newGameTime = thisTime / 1000000; // divide by a million to get seconds
//  *seconds = newGameTime + additionalSeconds;
    *seconds = newGameTime + additionalSeconds;
//  HHCheckAllHandles();
    RestoreOriginalColors();
    if ( result < 0)
    {
        if ( (globals()->gScenarioWinner & kScenarioWinnerPlayerMask) == globals()->gPlayerAdmiralNumber) return ( 1);
        else return ( 0);
    } else return ( result);
}

Boolean HandleMouseDown( EventRecord *theEvent)

{
    long        menuChoice;
    short       whichPart;
    WindowPtr   whichWindow;
    Point       where;
    Boolean     done = FALSE;

    whichPart = MacFindWindow (theEvent->where,  &whichWindow);
    switch (whichPart)
    {
        case inMenuBar:
            menuChoice = MenuSelect( theEvent->where);
            done = HandleMenuChoice( menuChoice);
            break;
        case inSysWindow:
            SystemClick( theEvent, whichWindow);
            break;
        case inContent:
            where = theEvent->where;
            GlobalToLocal( &where);
            break;
        case inDrag:
            DragWindow (whichWindow, theEvent->where, &qd.screenBits.bounds);
            break;
        case inGoAway:
            if ( TrackGoAway (whichWindow, theEvent->where))
            {
                ShowHide ( whichWindow, FALSE);
                if ( whichWindow == gTheWindow)
                    done = TRUE;
            }
            break;
    }
    return ( done);
}

Boolean HandleMenuChoice( long menuChoice)

{
    int     theMenu;
    int     theItem;
    Boolean done = FALSE;

    if (menuChoice != 0)
    {
        theMenu = HiWord( menuChoice);
        theItem = LoWord( menuChoice);
        switch (theMenu)
        {
            case APPLE_MENU_ID:
                HandleAppleChoice( theItem);
                break;
            case FILE_MENU_ID:
                done = HandleFileChoice( theItem);
                break;
            case EDIT_MENU_ID:
                HandleEditChoice( theItem);
                break;
        }
        HiliteMenu( 0 );
    }
    return ( done);
}

void HandleAppleChoice ( int theItem)

{
    Str255      accName;
    int         accNumber;

    switch ( theItem)
    {
        case ABOUT_ITEM:
            NoteAlert( ABOUT_ALERT, nil);
            break;
        default:
            GetMenuItemText ( gAppleMenu, theItem, accName);
#if TARGET_OS_MAC
            accNumber = OpenDeskAcc( accName);
#endif TARGET_OS_MAC
            break;
    }
}

Boolean HandleFileChoice( int theItem)

{
    switch( theItem)
    {
        case QUIT_ITEM:
            return ( TRUE);
            break;
    }
    return ( FALSE);
}

void HandleEditChoice( int theItem)

{
#pragma unused( theItem)
}

Boolean HandleKeyDown( EventRecord *theEvent)

{
    char    whichChar;

    whichChar = theEvent->message & charCodeMask;
    if (( theEvent->modifiers & cmdKey ) != 0)
        return( HandleMenuChoice( MenuKey( whichChar )));
    else return( false);
}

void Pause( long time)

{
    long    starttime = TickCount();

    while (( TickCount() - starttime) < time) {
        // DO NOTHING
    }
}

void SetWindowColorTable( WindowPtr)
{
    /*

    WCTabHandle     winCTabHandle;                                                  // 2
    AuxWinHandle    auxWinHandle;                                                   // 2

    GetAuxWin (window,&auxWinHandle);                                               // 2
    winCTabHandle = (WCTabHandle) ((**auxWinHandle).awCTable);                      // 2

    HandToHand ((Handle *) &winCTabHandle);                                         // 3
    if (!MemError ( ))                                                              // 3
    {
        RGBColor blackness = { 0, 0, 0 };                                           // 4

        if (SetColorTableEntry ((CTabHandle) winCTabHandle, 0, &blackness))         // 4
        {
            SetWinColor (window,winCTabHandle);                                     // 5
        }
    }
    */
}

/*
static pascal Boolean SetColorTableEntry (CTabHandle cth, short value, const RGBColor *rgbP)
{
    ColorSpecPtr    ctTable     = (**cth).ctTable;
    short           ctSize      = (**cth).ctSize;

    while (ctSize > -1)
    {
        if (ctTable->value == value)
        {
            ctTable->rgb = *rgbP;
            CTabChanged (cth);
            return true;
        }

        ++ctTable;
        --ctSize;
    }

    return false;
}
*/

void DrawOutlinedString( const unsigned char* string, RGBColor *color)
{
    RGBColor    backColor = {0, 0, 0};
    Point       pen;

    GetPen( &pen);

    RGBForeColor( &backColor);
    MoveTo( pen.h - 1, pen.v - 1);
    DrawString( string);
    MoveTo( pen.h, pen.v - 1);
    DrawString( string);
    MoveTo( pen.h + 1, pen.v - 1);
    DrawString( string);
    MoveTo( pen.h - 1, pen.v);
    DrawString( string);
    MoveTo( pen.h + 1, pen.v);
    DrawString( string);
    MoveTo( pen.h - 1, pen.v + 1);
    DrawString( string);
    MoveTo( pen.h, pen.v + 1);
    DrawString( string);
    MoveTo( pen.h + 1, pen.v + 1);
    DrawString( string);

    MoveTo( pen.h, pen.v);

    RGBForeColor( color);
    DrawString( string);
}
