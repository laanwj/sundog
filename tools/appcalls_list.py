# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

##### SunDog/Atari ST specific procedure numbers
# if tuple: name, ins, outs
APPCALLS = {
    # Game main and initialization
    b'SUNDOG  ': {
        0x01: 'Main()',
        0x02: 'BackgroundTask(a)',
        0x04: 'MainDispatch()', # Dispatch to moveonground/moveinbuilding/etc
    },
    b'XSTARTUP': { # Initialization / start up menu / librarian
        0x01: 'XStartup(flag)',
        0x02: 'InitPalette()',
        0x03: 'InitGFX(a,b,c)',   # Sets up pointers to graphics
        0x04: 'InitVDIBuffer(ptr,abs_dst,size)',
        0x05: 'AllocVDIBuffer(dst1,dst2,size1,size2)',
        0x0a: 'LoadData(a,b)',  # Loads list of systems, planets and cities
        0x0f: 'LoadResources()',
        0x11: 'ShowSavedGameStatus()',
        0x36: 'AttributeSetting()',
    },
    b'DONESOFA': { # DONESOFAR
        0x02: 'DoStartup(flag)',
        0x03: 'DoInter(a,b,c)',
    },
    # Game actions
    b'SHIPLIB ': {
        0x02: 'DoUserMenu(a,b)',
        0x05: 'DoRepair(a,b,c)',
        0x0f: 'RedFlashToggle',
        0x16: ('DrawStars()', 0, 0), # This draws the viewscreen background
        0x17: 'Randomize', # Randomize 256 bytes
        0x18: ('HandleBogeys()', 0, 0), # Handle bogeys (enemy ship movement) in space fight
        0x19: ('FireWeaponAnimation(x)', 1, 0),
        0x1a: ('WarpAnimation(a,b,c,vdihandle)', 4, 0),
        0x1b: 'SetFillColor', # VDI 25
        0x1c: 'FillRectangle', # VDI 114
        0x10: 'SetClipAndWriteMode', # VDI 129/32
    },
    b'GRNDLIB ': {
        0x02: 'DoCombat(a,b,c)',
        0x03: 'DoMoveOnGround(a,b,c,d,e,f)',
        0x04: 'DoUniteller(a)',
        0x05: 'DoTrading(a)',
        0x06: 'DoMoveInBuilding(a,b,c,d)',
        0x07: 'DoSlots()',
    },
    b'XDOREPAI': { # XDOREPAIR (ship repair)
        0x05: 'Cargo',     # Loads cargo info from disk, shows cargo
    },
    b'XDOUSERM': { # XDOUSERMENU
        0x06: 'WaitEvent',     # Loop until mouse button or MAINLIB+0x03 flag set
        0x07: 'ShowCondition',
        0x0b: 'ShowTime',
        0x0c: 'ShowMoney',
        0x0d: 'ShowAttributes',
        0x0e: 'ShowLocation',  # Current city/planet
    },
    b'XMOVEINB': { # XMOVEINBUILDING
        0x1a: 'SlotMachine()',
    },
    b'XDOFIGHT': {
        0x0b: 'MouseToShipMovement()',
    },
    # Utilities
    b'MAINLIB ': {
        0x02: 'WaitMouseRelease',  # Wait until no mouse buttons pressed
        0x03: 'Sound(x)',
        0x05: 'RedScreenOfDeath',  # Fatal error
        0x06: 'PaletteColor(r,g,b):integer',
        0x07: 'LAnd(a,b):integer', # Logical AND
        0x08: 'Sign(x):integer', # Sign function: -1 if x<0, x if x==0, 1 if x>0
        0x0b: 'PseudoRandom():integer',
        0x0c: 'RandomRange(low,high):integer',
        0x0d: 'Thousands(addrout,?,value)',
        0x11: 'GetScore():integer',
        0x12: 'SumData',           # Simple checksum
        0x13: 'SumAndDeobfuscate(_,stride,_,_,addr,count,key)', # Sum then deobfuscate data
        0x14: 'ObfuscateAndSum(_,stride,_,_,addr,count,key)',   # Obfuscate data and sum
        0x17: 'PrintNewLine',
        0x19: 'SetTextPos',
        0x1b: 'PrintStr',
        0x1c: 'PrintLn',
        0x1d: 'PrintNumber1',
        0x1e: 'PrintNumber2', # uses different print function
        0x1f: 'FormatMoney(a,b,c)',
        0x20: 'PrintMoney(amountptr)',
        0x21: 'SetGlobal7',
        0x24: 'DrawTime(a,b,c,d,e)',
        0x2c: 'WaitEvent',  # Wait for mouse click or other event
        0x2d: 'ConditionalWait',   # Conditional wait on semaphore MAINLIB+0xa74
        0x2e: 'ConditionalSignal', # Conditional signal on semaphore MAINLIB+0xa74
        0x32: 'PaletteChange()',
        0x34: 'PossiblySetEvilState', # Possibly set state so that RSOD appears
        0x44: 'ErrorMessage',
        0x45: 'CheckDiskResult', # Return 1 if error, 0 otherwise
        0x46: 'CheckCorrectDisk', # Read block 0x3a or 0x06 to see if library/sundog disk is in the drive as expected
        0x49: 'DiskIO(?,lenwordsmin1,?,addr,count,block,?)', # Seems a general function for disk I/O
        0x4a: 'WeirdDiskCheck', # Read an entire track, compute checksum?
        0x4b: 'LoadImageBase(a,b,c,d,e)', # Load image resource?
        0x4c: 'LoadImageInner(a,b,c,addr_ptr)', # Load image resource?
        0x4d: 'LoadImageOuter(addr_ptr,idx)', # Load image resource? idx 0x00..0x57
        0x4e: 'TextPrintHelper',
        0x4f: 'FormatPaddedNumber',
        0x50: 'TimeDigit', # local function of DrawTime
        0x54: 'PossiblyRunDiskCheck',
        0x15: 'FormatNumber',
    },
    b'WINDOWLI': {
        0x07: 'ShowItemName', # Show item name
        0x17: 'DrugEffects(item,b,c,d,e)',
    },
    # Bindings
    b'GEMBIND ': {
        0x02: ('VDI(handle,opcode,numptsin,numintin)', 4, 0),
        0x03: ('AES(a,b,c,d,e,f)', 6, 0),
        0x04: ('MemoryCopy(from,to,bytes)', 3, 0), # Some memory copy function
        0x05: ('GetAbsoluteAddress(ptr,dst)', 2, 0),
        0x06: ('SetScreen(x)', 1, 0), # Unused
        0x07: ('DecompressImage(addr1,addr2,val1,val2)', 4, 0),  # some effect (scrolling?)
        0x08: ('DoSound(x)', 1, 0),
        0x09: ('Native09', 3, 1), # Unused
        0x0a: ('FlopFmt(a,b,c,d,e,f,g,h,i)', 9, 0),
        0x0b: ('SetColor(color,index)', 2, 0),
        0x0c: 'Native0C', # Unused
        0x0d: ('ScrollScreen(amount,direction)', 2, 0),
        0x0e: 'SetMouseShape',
        0x0f: 'SetClippingRectangle1',
        0x10: 'PolyLine(numptsin)',
        0x11: 'DrawLine',  # One line
        0x12: 'MultiDrawChar',  # Draw ASCII text
        0x13: 'DrawChar(ch,x,y)',  # Wrapper over 0x1d
        0x14: 'DrawFilledRectangle',
        0x15: 'DrawEllArc(begang,endang,x,y,xradius,yradius)', # VDI 11, subfunction 6
        0x16: 'SetWriteMode', # VDI 32
        0x17: 'SetLineWidth', # VDI 16
        0x18: 'SetLineColor', # VDI 17
        0x19: 'SetTextColor', # Set a global
        0x1a: 'SetFillColor', # VDI 25
        0x1c: ('DrawColor(psrc,pdst,vrmode,srcx1,srcx2,srcy1,srcy2,dstx1,dstx2,dsty1,dsty2)', 11, 0), # VDI 109
        0x1d: ('DrawBW(psrc,pdst,fgcolor,bgcolor,writemode,srcx1,srcx2,srcy1,srcy2,dstx1,dstx2,dsty1,dsty2)', 13, 0), # Draw black and white raster gfx VDI 121
        0x1e: 'GetMousePosition(addrx,addry)', # VDI 124
        0x1f: 'GetMouseButton', # VDI 124 (mouse button 0/1 passed in)
        0x20: 'DrawIcon(idx,vrmode,x,y)', # calls 0x1c
        0x21: 'DrawBWWriteMode', # set writing mode, draw b/w
        0x22: 'DrawBW?', # calls DrawBW
        0x23: 'SetClippingRectangle2', # similar to 0x0f
        0x26: 'ShowMouseCursor',
        0x27: 'HideMouseCursor',
        0x28: ('CollisionDetect(pattern,x,y): integer', 4, 1),
        0x29: ('DrawSprite(flag,back_addr,x,y,pattern,color)', 6, 0),
        0x2a: ('SpriteMovementEnable(flag)', 1, 0),
        0x2b: ('SetSprite(x,y,pattern,color,back,idx)', 6, 0), # 
        0x2c: ('InstallVBlankHandler(flag)', 1, 0),
    },
}
