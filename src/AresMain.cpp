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

#include <Quickdraw.h>

#include "Admiral.hpp"
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

#include "Debug.hpp"
#include "DirectText.hpp"

#include "EnvironmentCheck.hpp"
#include "Error.hpp"

#include "FakeDrawing.hpp"
#include "Fakes.hpp"


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
#include "Resources.h"
#include "Rotation.hpp"

#include "ScenarioMaker.hpp"
#include "ScreenLabel.hpp"
#include "ScrollStars.hpp"
#include "ShotsBeamsExplosions.hpp"
#include "SoundFX.hpp"
#include "SpaceObjectHandling.hpp"
#include "SpriteCursor.hpp"
#include "SpriteHandling.hpp"
#include "StringHandling.hpp"

#include "TimeLimit.hpp"
#include "TimeUnit.hpp"
#include "TitleScreen.hpp"
#include "Transitions.hpp"

#include "VersionString.hpp"
#include "VideoDriver.hpp"

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

enum GameResult {
    NO_GAME = -1,
    LOSE_GAME = 0,
    WIN_GAME = 1,
    RESTART_GAME = 2,
    QUIT_GAME = 3,
};

//#define   kCanRecordGame

#define kReplayNotAutoPlay  // if this is set, will play back recorded games

#define mAbortNetworkGame       { if ( globals()->gGameOver == 0) globals()->gGameOver = 1; result = kQuitGame; StopNetworking();}

#define kTitleTextScrollWidth   450

#define kFractionalLagCorrectTolerance  8

#define kInterfaceResFileName   "\p:Ares Data Folder:Ares Interfaces"
#define kSpriteResFileName      "\p:Ares Data Folder:Ares Sprites"
#define kSoundResFileName       "\p:Ares Data Folder:Ares Sounds"

//#define   kTestNumber     50//5

//#define   kUseSmallPlayWindow // for debugging

extern TypedHandle<spaceObjectType> gSpaceObjectData;
extern int32_t gRandomSeed;
extern long gNatePortLeft, gNatePortTop, gNetLatency;
extern scenarioType *gThisScenario;
extern short gSpriteFileRefID, gInterfaceFileRefID;
extern PixMap* gActiveWorld;
extern scoped_ptr<Window> fakeWindow;

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
bool         hackWarn = true;

unsigned long kNoKeys = 0;

int GetDemoScenario();
void FakeInit(int argc, char* const* argv);
void Pause( long time);
void MainLoop();
GameResult PlayTheGame(long *seconds);

int main(int argc, char* const* argv) {
    FakeInit(argc, argv);
    return 0;
}

void AresMain() {
    RGBColor                initialFadeColor;
    bool                 skipFading = false;
    scoped_ptr<ColorTable>  theClut;

    init_globals();
    WORLD_WIDTH = fakeWindow->portRect.right;
    WORLD_HEIGHT = fakeWindow->portRect.bottom;
    CLIP_RIGHT = WORLD_WIDTH - kRightPanelWidth;
    CLIP_BOTTOM = WORLD_HEIGHT;
    gPlayScreenWidth = CLIP_RIGHT - CLIP_LEFT;
    gPlayScreenHeight = CLIP_BOTTOM - CLIP_TOP;

    globals()->gPreferencesData.reset(new Preferences);

    // Disable networking.
    globals()->gOptions &= ~kOptionNetworkAvailable;

    GetDateTime( reinterpret_cast<unsigned long *>(&gRandomSeed));

    theClut.reset(new ColorTable(256));
    gTheWindow = fakeWindow.get();
    gActiveWorld = &fakeWindow->portBits;
    ColorTranslatorInit(*theClut);

    InitSpriteCursor();
    InitTransitions();

    initialFadeColor.red = initialFadeColor.green = initialFadeColor.blue = 0;
    skipFading = AutoFadeTo(30, &initialFadeColor, true);

    ClearScreen();
    ResetTransitions();

    CreateOffscreenWorld(gTheWindow->portRect, *theClut);
    MusicInit();

    InitMoviePlayer();
    RotationInit();
    NormalizeColors();
    DrawInRealWorld();

    InterfaceHandlingInit();

    if (globals()->gOptions & kOptionMusicIdle) {
        LoadSong( kTitleSongID);
        SetSongVolume( kMaxMusicVolume);
        PlaySong();
    }
    if (!skipFading) {
        skipFading = CustomPictFade(20, 20, 2000, 2000, gTheWindow);
    }
    if (!skipFading) {
        skipFading = CustomPictFade(20, 20, 2001, 2000, gTheWindow);
    }

    StartCustomPictFade(20, 20, 502, 2001, gTheWindow, skipFading);
    InitDirectText();
    ScreenLabelInit();
    InitMessageScreen();
    InitScrollStars();
    InstrumentInit();
    SpriteHandlingInit();
    AresCheatInit();
    ScenarioMakerInit();
    SpaceObjectHandlingInit();  // MUST be after ScenarioMakerInit()
    InitSoundFX();
    InitMotion();
    AdmiralInit();
    InitBeams();
    TimedWaitForAnyEvent(skipFading ? 1 : 1400);
    EndCustomPictFade(gTheWindow, skipFading);

    gLastTick = TickCount();

    globals()->okToOpenFile = true;
    MainLoop();
}

void MainLoop() {
    long                    saveSeed = 0, gameLength;
    bool                    done = false;

    if (!(globals()->gOptions & kOptionHaveSeenIntro)) {
        DoScrollText(5600, 4, kTitleTextScrollWidth, kTitleFontNum, -1);

        globals()->gOptions |= kOptionHaveSeenIntro;
        SaveOptionsPreferences();
    }

    while (!done) {
        switch (DoMainScreenInterface()) {
          case kMainQuit:
            {
                RGBColor fadeColor = { 0, 0, 0 };
                if (globals()->gOptions & kOptionMusicIdle) {
                    AutoMusicFadeTo( 60, &fadeColor, false);
                } else {
                    AutoFadeTo( 60, &fadeColor, false);
                }
                done = true;
            }
            break;

          case kMainTimeoutDemo:
            if (Randomize(4) == 2) {
                DoScrollText(5600, 4, kTitleTextScrollWidth,
                        kTitleFontNum, -1);
            }
            // fall through

          case kMainDemo:
            globals()->gOptions |= kOptionReplay;
            // fall through

          case kMainPlay:
            {
                int whichScenario = 0;

                if (globals()->gOptions & kOptionReplay) {
                    while (globals()->gInputSource.get() == nil) {
                        whichScenario = GetDemoScenario();
                        globals()->gInputSource.reset(
                                new ReplayInputSource(kReplayResID + whichScenario));
                    }

                    saveSeed = gRandomSeed;
                    gRandomSeed = globals()->gInputSource->random_seed();
                } else {
                    whichScenario = DoSelectLevelInterface(GetStartingLevelPreference());
                    if (whichScenario < 0) {
                        break;
                    }
                }

                StopAndUnloadSong();

                GameResult gameResult = NO_GAME;
                do {
                    if ((gameResult == NO_GAME) || (gameResult == WIN_GAME)) {
                        if (GetScenarioPrologueID(whichScenario) > 0) {
                            DoScrollText(
                                    GetScenarioPrologueID( whichScenario),
                                    4, kTitleTextScrollWidth, kTitleFontNum, 4002);
                        }
                    }

                    gameResult = NO_GAME;
                    RGBColor fadeColor = { 0, 0, 0 };
                    if (globals()->gOptions & kOptionMusicIdle) {
                        AutoMusicFadeTo(60, &fadeColor, false);
                        StopAndUnloadSong();
                    } else {
                        AutoFadeTo( 60, &fadeColor, false);
                    }

                    RemoveAllSpaceObjects();
                    globals()->gGameOver = 0;

                    if (globals()->gOptions & kOptionMusicIdle) {
                        LoadSong(3000);
                        SetSongVolume( kMaxMusicVolume);
                        PlaySong();
                    }

                    gameResult = NO_GAME;
                    if (!ConstructScenario(whichScenario)) {
                        break;
                    }

                    if (globals()->gOptions & kOptionMusicIdle) {
                        StopAndUnloadSong();
                    }

                    DrawInstrumentPanel(gTheWindow);

                    if (globals()->gOptions & kOptionMusicPlay) {
                        LoadSong(gThisScenario->songID);
                        SetSongVolume(kMusicVolume);
                        PlaySong();
                    }
                    ResetLastTime((gThisScenario->startTime & kScenario_StartTimeMask) * kScenarioTimeMultiple);

                    VideoDriver::driver()->set_game_state(PLAY_GAME);
                    gameResult = PlayTheGame(&gameLength);
                    VideoDriver::driver()->set_game_state(DONE_GAME);

                    switch (gameResult) {
                      case LOSE_GAME:
                        if (!(globals()->gOptions & kOptionReplay)) {
                            if (globals()->gScenarioWinner.text != -1) {
                                DoMissionDebriefingText(
                                        globals()->gScenarioWinner.text,
                                        -1, -1, -1, -1, -1, -1, -1);
                            }
                            switch (DoPlayAgain(false, false)) {
                              case PLAY_AGAIN_RESTART:
                                gameResult = RESTART_GAME;
                                break;

                              case PLAY_AGAIN_QUIT:
                                gameResult = QUIT_GAME;
                                ClearScreen();
                                AutoFadeFrom(1, false);
                                break;

                              default:
                                fprintf(stderr, "DoPlayAgain(false, false) returned bad value\n");
                                exit(1);
                            }
                        }
                        break;

                      case WIN_GAME:
                        {
                            if (globals()->gScenarioWinner.text != -1) {
                                DoMissionDebriefingText(
                                        globals()->gScenarioWinner.text,
                                        gameLength, gThisScenario->parTime,
                                        GetAdmiralLoss(0), gThisScenario->parLosses,
                                        GetAdmiralKill(0), gThisScenario->parKills,
                                        100);
                            }

                            fadeColor.red = fadeColor.green = fadeColor.blue = 0;
                            if (globals()->gOptions & kOptionMusicPlay) {
                                AutoMusicFadeTo( 60, &fadeColor, false);
                                StopAndUnloadSong();
                            } else {
                                AutoFadeTo( 60, &fadeColor, false);
                            }

                            ClearScreen();
                            AutoFadeFrom( 1, false);

                            // normal scrolltext song
                            int scroll_song = 4002;
                            if (globals()->gScenarioWinner.next == -1) {
                                // we win but no next level? Play triumph song
                                scroll_song = 4003;
                            }

                            if (GetScenarioEpilogueID( whichScenario) > 0) {
                                DoScrollText(
                                        GetScenarioEpilogueID( whichScenario),
                                        4, kTitleTextScrollWidth, kTitleFontNum, scroll_song);
                            }

                            if (globals()->gOptions & kOptionMusicIdle) {
                                ClearScreen();
                                StopAndUnloadSong();
                            }

                            if (globals()->gScenarioWinner.next == -1) {
                                whichScenario = -1;
                            } else {
                                whichScenario = GetScenarioNumberFromChapterNumber(
                                        globals()->gScenarioWinner.next);
                            }

                            if ((!(globals()->gOptions & kOptionReplay))
                                    && (whichScenario <= GetScenarioNumber())
                                    && (whichScenario >= 0)) {
                                int chapter = GetChapterNumberFromScenarioNumber(whichScenario);
                                if ((chapter >= 0) && (chapter <= kHackLevelMax)
                                        && (chapter <= GetScenarioNumber())) {
                                    SaveStartingLevelPreferences(chapter);
                                } else {
                                    whichScenario = -1;
                                }
                            }
                        }
                        break;

                      default:
                        break;
                    }

                    if (globals()->gOptions & kOptionReplay) {
                        gRandomSeed = saveSeed;
                    }
                    globals()->gInputSource.reset();

                    if (globals()->gOptions & kOptionMusicPlay) {
                        StopAndUnloadSong();
                    }
                } while ((gameResult != QUIT_GAME)
                        && (GetChapterNumberFromScenarioNumber(whichScenario) <= kHackLevelMax)
                        && (GetChapterNumberFromScenarioNumber(whichScenario) <= GetScenarioNumber())
                        && (whichScenario >= 0)
                        && (!(globals()->gOptions & kOptionReplay)));

                if ( globals()->gOptions & kOptionMusicIdle) {
                    LoadSong(kTitleSongID);
                    SetSongVolume(kMaxMusicVolume);
                    PlaySong();
                }
                globals()->gOptions &= ~kOptionReplay;
            }
            break;

          case kMainNetwork:
            {
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
                                AutoMusicFadeTo( 60, &fadeColor, false);
                                StopAndUnloadSong();
                            } else
                            {
                                AutoFadeTo( 60, &fadeColor, false);
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
                                    if (globals()->gScenarioWinner.player
                                            == globals()->gPlayerAdmiralNumber) {
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
                                if (globals()->gScenarioWinner.text != -1) {
                                    DoMissionDebriefingText(  gTheWindow,
                                            globals()->gScenarioWinner.text,
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
            }
            break;

          case kMainTrain:
            DoScrollText(5600, 4, kTitleTextScrollWidth, kTitleFontNum, -1);
            break;

          case kMainAbout:
            DoScrollText(6500, 2, 540, kTitleFontNum, -1);
            break;

          case kMainOptions:
            DoOptionsInterface();
            break;
        }
    }
}

GameResult PlayTheGame(long *seconds) {
    unsigned long       decideCycle = 0;
    Str255              string;
    uint64_t            lastTime, thisTime, scrapTime = 0;
    Rect                clipRect;
    long                    unitsToDo = 0, unitsPassed = 0, unitsDone = 0,
                            l1, l2, newGameTime = 0, lastclicktime = 0,
                            additionalSeconds = 0;
    KeyMap              keyMap = { }, lastKeyMap;
    bool             playerPaused = false, mouseDown = false,
                            enteringMessage = false,
                            afEntering = false, demoKey = false, newKeyMap = false, commandAndQ = false;
    unsigned long       scenarioCheckTime = 0;
    Rect                    playAreaRect;
    GameResult          result = NO_GAME;
    EventRecord         theEvent;

    commandAndQ = BothCommandAndQ();

    SetSpriteCursorTable(500);
    ShowSpriteCursor(true);
    ResetHintLine();
    playAreaRect = Rect(CLIP_LEFT, CLIP_TOP, CLIP_RIGHT, CLIP_BOTTOM);

    globals()->gLastKeys = globals()->gTheseKeys = 0;

    HideCursor();
    Microseconds(&lastTime);

    CheckScenarioConditions(0);

    const int64_t scenario_start_time = (gThisScenario->startTime & kScenario_StartTimeMask)
        * kScenarioTimeMultiple;

    while ((globals()->gGameOver <= 0 ) && (!globals()->returnToMain)) {
        globals()->gFrameCount = 0;
        gLastTick = TickCount();

        SetLongRect(&clipRect, CLIP_LEFT, CLIP_TOP, CLIP_RIGHT, CLIP_BOTTOM);
        while (globals()->gGameOver <= 0) {
            EraseSpriteCursorSprite();
            EraseSpriteTable();
            EraseAllLabels();
            EraseSectorLines();
            PrepareToMoveScrollStars();
            EraseSite();

            while (unitsPassed == 0) {
                Microseconds(&thisTime);
                scrapTime = thisTime;
                thisTime -= globals()->gLastTime;
                newGameTime = (thisTime / kTimeUnit) + scenario_start_time;

                if (((globals()->gOptions & kOptionSubstituteFKeys)
                            ? mNOFFastMotionKey(keyMap)
                            : mFastMotionKey(keyMap)) &&
                        !enteringMessage) {
                    demoKey = true;
                    newGameTime = globals()->gGameTime + 12;
                    l1 = newGameTime - scenario_start_time;
                    l2 = kTimeUnit;
                    thisTime = (newGameTime - scenario_start_time) * kTimeUnit;
                    globals()->gLastTime = scrapTime - thisTime;
                }

                unitsDone = unitsPassed = newGameTime - globals()->gGameTime;
            }

            if (playerPaused) {
                playerPaused = false;
                unitsDone = unitsPassed = 0;
                newGameTime = globals()->gGameTime;
                thisTime = (newGameTime - scenario_start_time) * kTimeUnit;
                globals()->gLastTime = scrapTime - thisTime;
            }

            if (globals()->gGameOver < 0) {
                globals()->gGameOver += unitsPassed;
                if ( globals()->gGameOver == 0)
                    globals()->gGameOver = 1;
            }

            while (unitsPassed > 0) {
                unitsToDo = unitsPassed;
                if (unitsToDo > kMaxTimePerCycle) {
                    unitsToDo = kMaxTimePerCycle;
                }
                if ((decideCycle + unitsToDo) > kDecideEveryCycles) {
                    unitsToDo = kDecideEveryCycles - decideCycle;
                }
                decideCycle += unitsToDo;

                if (unitsToDo > 0) {
                    // executed arbitrarily, but at least once every kDecideEveryCycles
                    MoveScrollStars(unitsToDo);
                    MoveSpaceObjects(*gSpaceObjectData, kMaxSpaceObject, unitsToDo);
                }

                globals()->gGameTime += unitsToDo;

                if ( decideCycle == kDecideEveryCycles) {
                    // everything in here gets executed once every kDecideEveryCycles
                    playerPaused = false;

                    NonplayerShipThink( kDecideEveryCycles);
                    AdmiralThink();
                    ExecuteActionQueue( kDecideEveryCycles);

                    if (globals()->gOptions & kOptionReplay) {
                        uint32_t keys;
                        if (!globals()->gInputSource->next(&keys)) {
                            globals()->gGameOver = 1;
                        }

                        if (!playerPaused) {
                            playerPaused = PlayerShipGetKeys(
                                    kDecideEveryCycles, keys, &enteringMessage);
                        } else {
                            PlayerShipGetKeys( kDecideEveryCycles, keys, &enteringMessage);
                        }
                    } else {
                        if (!playerPaused) {
                            playerPaused = PlayerShipGetKeys(
                                    kDecideEveryCycles, 0xffffffff, &enteringMessage);
                        } else {
                            PlayerShipGetKeys(kDecideEveryCycles, 0xffffffff, &enteringMessage);
                        }
                    }

                    if (Button()) {
                        if (globals()->gOptions & kOptionReplay) {
                            result = QUIT_GAME;
                            globals()->gGameOver = 1;
                        } else {
                            if (!mouseDown) {
                                if (!(globals()->gOptions & ( kOptionAutoPlay | kOptionReplay))) {
                                    if (((globals()->gGameTime - lastclicktime)) <= GetDblTime()) {
                                        InstrumentsHandleDoubleClick();
                                        lastclicktime -= GetDblTime();
                                    } else {
                                        InstrumentsHandleClick();
                                        lastclicktime = globals()->gGameTime;
                                    }
                                }
                                mouseDown = true;
                            } else {
                                InstrumentsHandleMouseStillDown();
                            }
                        }
                    } else if (mouseDown) {
                        mouseDown = false;
                        InstrumentsHandleMouseUp();
                    }

                    CollideSpaceObjects(*gSpaceObjectData, kMaxSpaceObject);
                    decideCycle = 0;
                    scenarioCheckTime++;
                    if (scenarioCheckTime == 30) {
                        scenarioCheckTime = 0;
                        CheckScenarioConditions( 0);
                    }
                }
                unitsPassed -= unitsToDo;
            }

            newKeyMap = false;
            for ( l1 = 0; l1 < 4; l1++) {
                lastKeyMap[l1] = keyMap[l1];
            }
            GetKeys(keyMap);
            for (l1 = 0; l1 < 4; l1++) {
                if (lastKeyMap[l1] != keyMap[l1]) {
                    newKeyMap = true;
                }
            }

            if (mPauseKey(keyMap)) {
                RestoreOriginalColors();
                GetIndString( string, 3100, 11);

                PlayVolumeSound(kComputerBeep4, kMaxSoundVolume, kShortPersistence, kMustPlaySound);
                while ( (mPauseKey( keyMap)) && (!(mReturnKey(keyMap)))) {
                    l1 = TickCount();
                    StartPauseIndicator(string, Randomize(16));
                    playerPaused = false;
                    while ((mPauseKey(keyMap))
                            && (!(mReturnKey(keyMap)))
                            && ((TickCount() - l1) < 20)) {
                        GetKeys(keyMap);
                    }

                    l1 = TickCount();
                    StopPauseIndicator( string);
                    playerPaused = true;
                    while ((mPauseKey(keyMap))
                            && (!(mReturnKey(keyMap)))
                            && ((TickCount() - l1) < 20)) {
                        GetKeys( keyMap);
                        if (CommandKey()) {
                            WaitNextEvent (everyEvent, &theEvent, 3, nil);
                        }
                    }

                }
            }

            if ((!(globals()->gOptions & kOptionNetworkOn)) &&
                    (!(globals()->gOptions & kOptionReplay)) &&
                    ((mRestartResumeKey(keyMap))
                     || ((!commandAndQ) && (mQuitKeys(keyMap))))) {

                RestoreOriginalColors();
                MacShowCursor();
                bool is_training = gThisScenario->startTime & kScenario_IsTraining_Bit;
                switch (DoPlayAgain(true, is_training)) {
                case PLAY_AGAIN_QUIT:
                    result = QUIT_GAME;
                    globals()->gGameOver = 1;
                    if ( CommandKey())
                        globals()->gScenarioWinner.player = globals()->gPlayerAdmiralNumber;
                    globals()->gScenarioWinner.next = -1;
                    globals()->gScenarioWinner.text = -1;
                    break;

                case PLAY_AGAIN_RESTART:
                    result = RESTART_GAME;
                    globals()->gGameOver = 1;
                    if ( CommandKey())
                        globals()->gScenarioWinner.player = globals()->gPlayerAdmiralNumber;
                    globals()->gScenarioWinner.next = -1;
                    globals()->gScenarioWinner.text = -1;
                    break;

                case PLAY_AGAIN_RESUME:
                    break;

                case PLAY_AGAIN_SKIP:
                    result = WIN_GAME;
                    globals()->gGameOver = 1;
                    globals()->gScenarioWinner.player = globals()->gPlayerAdmiralNumber;
                    globals()->gScenarioWinner.next =
                        GetChapterNumberFromScenarioNumber(globals()->gThisScenarioNumber) + 1;
                    globals()->gScenarioWinner.text = -1;
                    break;
                }
                CopyOffWorldToRealWorld(&playAreaRect);
                HideCursor();
                playerPaused = true;
            }

            if (!(globals()->gOptions & kOptionReplay)
                    && !afEntering
                    && mHelpKey(keyMap)) {
                RestoreOriginalColors();
                MacShowCursor();
                DoHelpScreen();
                HideCursor();
                CopyOffWorldToRealWorld(&playAreaRect);
                playerPaused = true;
            }

            if (!(globals()->gOptions & kOptionReplay)
                    && !afEntering
                    && mVolumeDownKey(keyMap)
                    && !mVolumeDownKey(lastKeyMap)) {
                if ( globals()->gSoundVolume > 0) {
                    globals()->gSoundVolume--;
                }
                if ( globals()->gOptions & kOptionMusicPlay) {
                    SetSongVolume(kMusicVolume);
                }
            }

            if (!(globals()->gOptions & kOptionReplay)
                    && !afEntering
                    && mVolumeUpKey( keyMap)
                    && !mVolumeUpKey(lastKeyMap)) {
                if (globals()->gSoundVolume < kMaxVolumePreference) {
                    globals()->gSoundVolume++;
                }
                if (globals()->gOptions & kOptionMusicPlay) {
                    SetSongVolume( kMusicVolume);
                }
            }

            if (!(globals()->gOptions & kOptionReplay)
                    && !afEntering
                    && mActionMusicKey(keyMap)
                    && !mActionMusicKey(lastKeyMap)) {
                if (globals()->gOptions & kOptionMusicPlay) {
                    ToggleSong();
                }
            }

            keyMap[3] &= ~0x80; // mask out power key
            keyMap[1] &= ~0x02; // mask out caps lock key
            if ((globals()->gOptions & kOptionReplay)
                    && !demoKey
                    && !newKeyMap
                    && ((keyMap[0] != 0)
                        || (keyMap[1] != 0)
                        || (keyMap[2] != 0)
                        || (keyMap[3] != 0))) {
                result = QUIT_GAME;
                globals()->gGameOver = 1;
            }
            demoKey = false;

            MiniComputerHandleNull(unitsDone);

            ClipToCurrentLongMessage();
            SetLongRect( &clipRect, CLIP_LEFT, CLIP_TOP, CLIP_RIGHT, CLIP_BOTTOM);
            DrawScrollStars(true);
            DrawCurrentLongMessage( unitsDone);

            DrawSectorLines();
            DrawAllBeams();
            DrawSpriteTableInOffWorld(&clipRect);
            UpdateAllLabelPositions(unitsDone);
            DrawAllLabels();
            DrawSite();
            SetLongRect(&clipRect, 0, 0, WORLD_WIDTH, WORLD_HEIGHT);
            DrawSpriteCursorSprite(&clipRect);

            ShowSpriteCursorSprite();
            ShowSpriteTable();
            ShowAllLabels();
            ShowAllBeams();
            ShowScrollStars( true);
            ShowSectorLines();
            ShowSite();
            CopyOffWorldToRealWorld(&playAreaRect);

            DrawMessageScreen(unitsDone);
            UpdateRadar(unitsDone);
            UpdateboolColorAnimation(unitsDone);

            ++globals()->gFrameCount;
            VideoDriver::driver()->main_loop_iteration_complete(globals()->gGameTime);
        }
    }

    Microseconds(&thisTime);
    thisTime -= globals()->gLastTime;
    newGameTime = thisTime / 1000000; // divide by a million to get seconds
    *seconds = newGameTime + additionalSeconds;
    RestoreOriginalColors();

    if (result == NO_GAME) {
        if (globals()->gScenarioWinner.player == globals()->gPlayerAdmiralNumber) {
            return WIN_GAME;
        } else {
            return LOSE_GAME;
        }
    } else {
        return result;
    }
}

void Pause( long time)

{
    long    starttime = TickCount();

    while (( TickCount() - starttime) < time) {
        // DO NOTHING
    }
}
