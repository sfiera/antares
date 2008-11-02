/*
Ares, a tactical space combat game.
Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// Handle Handling.h

#pragma options align=mac68k

#define	mHandleLockAndRegister( mhandle, munlockProc, mlockPrc, mresolveProc, mhandlename)\
MoveHHi( (mhandle));\
HLock( (mhandle));\
HHClearHandle( mhandle);\
HHRegisterHandle( &(mhandle), munlockProc, mlockPrc, mresolveProc, false, mhandlename);

#define	mDataHandleLockAndRegister( mhandle, munlockProc, mlockPrc, mresolveProc, mhandlename)\
MoveHHi( (mhandle));\
HLock( (mhandle));\
HHRegisterHandle( &(mhandle), munlockProc, mlockPrc, mresolveProc, false, mhandlename);

#define	mCheckDataHandleLockAndRegister( mhandle, munlockProc, mlockPrc, mresolveProc, mhandlename)\
MoveHHi( (mhandle));\
HLock( (mhandle));\
HHRegisterHandle( &(mhandle), munlockProc, mlockPrc, mresolveProc, true, mhandlename);

#define	mHandleDisposeAndDeregister( mhandle)\
HHDeregisterHandle( &(mhandle));\
DisposeHandle( (mhandle));

short HandleHandlerInit( void);
void HandleHandlerCleanup( void);
void ResetAllHandleData( void);
short HHRegisterHandle( Handle *newHandle,
			void			(*unlockData)( Handle),
			void			(*lockData)( Handle),
			void			(*resolveData)( Handle),
			Boolean, StringPtr);
void HHDeregisterHandle( Handle *);
void HHMaxMem( void);
Handle HHNewHandle( long);
Handle HHGetResource( ResType, short);
void HHConcatenateHandle( Handle, Handle);
void HHClearHandle( Handle);
void HHCheckHandle( Handle, Handle);
void HHCheckAllHandles( void);

#pragma options align=reset
