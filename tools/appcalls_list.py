# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

##### SunDog/Atari ST specific procedure numbers
# if tuple: name, ins, outs
APPCALLS = {
    # Game main and initialization
    b'SUNDOG  ': {
        0x01: 'sundog()',
        0x02: 'dispatcher(a)',
        0x03: 'do_ship',
        0x04: 'do_ground', # Dispatch to moveonground/moveinbuilding/etc
    },
    b'XSTARTUP': { # Initialization / start up menu / librarian
        0x01: 'XStartup(flag)',
        0x02: 'set_palette()',
        0x03: 'init_gem(a,b,c)',   # Sets up pointers to graphics
        0x04: 'init_gem:set_tile_rec(ptr,abs_dst,size)',
        0x05: 'init_gem:make_VDI(dst1,dst2,size1,size2)',
        0x06: 'link_SBIOS',
        0x07: 'init_vars',
        0x08: 'request_sundog',
        0x09: 'save_game',
        0x0a: 'load_game(a,b)',  # Loads list of systems, planets and cities
        0x0b: 'load_gcheck',
        0x0c: 'change_deaths',
        0x0d: 'show_score',
        0x0e: 'show_score:vnumln',
        0x0f: 'load_database()',
        #
        0x11: 'ShowSavedGameStatus()',
        0x14: 'StartupMenu()',
        0x15: 'Librarian()',
        0x36: 'AttributeSetting()',
        0x46: 'IntroText()',
        0x49: 'PlayerDied()',
    },
    b'DONESOFA': { # DONESOFAR
        0x02: 'start_up(flag)',
        0x03: 'do_interaction(a,b,c)',
        0x04: 'wipe_title_page',
    },
    # Game actions
    b'SHIPLIB ': {
        0x02: 'do_user_menu',
        0x03: 'pilotage',
        0x04: 'read_map',
        0x05: 'do_repairs(a,b,c)',
        0x06: 'land_fx',
        0x07: 'show_move',
        0x08: 'move_on_ship',
        0x09: 'do_fight',
        0x0a: 'dist',
        0x0b: 'init_plan',
        0x0c: 'polar_to_rect',
        0x0d: 'change_stat',
        0x0e: 'update_system',
        0x0f: 'running_lights',
        0x10: 'whoosh_whoosh_lights', # VDI 129/32
        0x11: 'bay_lights',
        0x12: 'update_ship',
        0x13: 'add_time',
        0x14: 'set_time',
        0x15: 'expired',
        0x16: ('stars()', 0, 0), # This draws the viewscreen background
        0x17: 'set_stars', # Randomize 256 bytes (star positions)
        0x18: ('explosion()', 0, 0), # Handle bogeys (enemy ship movement) in space fight
        0x19: ('laser_fx(x)', 1, 0),
        0x1a: ('wowzo(a,b,c,vdihandle)', 4, 0),
        # Internal functions
        0x1b: 'SetFillColor', # VDI 25
        0x1c: 'FillRectangle', # VDI 114
    },
    b'GRNDLIB ': {
        0x02: 'do_combat(a,b,c)',
        0x03: 'move_on_ground(a,b,c,d,e,f)',
        0x04: 'do_uniteller(a)',
        0x05: 'do_trading(a)',
        0x06: 'move_in_building(a,b,c,d)',
        0x07: 'slots()',
    },
    b'XDOREPAI': { # XDOREPAIR (ship repair)
        0x01: 'XDoRepair',
        0x05: 'Cargo',     # Loads cargo info from disk, shows cargo
        0x07: 'DragItem',
    },
    b'XDOUSERM': { # XDOUSERMENU
        0x01: 'XDoUserMenu',
        0x03: 'ShowStats',     # Draws stat bars for vigor, rest, health, noursh
        0x06: 'WaitEvent',     # Loop until mouse button or MAINLIB+0x03 flag set
        0x07: 'ShowCondition',
        0x0b: 'ShowTime',
        0x0c: 'ShowMoney',
        0x0d: 'ShowAttributes',
        0x0e: 'ShowLocation',  # Current city/planet
    },
    b'XMOVEINB': { # XMOVEINBUILDING
        0x01: 'XMoveInBuilding',
        0x1a: 'SlotMachine()',
    },
    b'XMOVEONS': { # XMOVEONSHIP
        0x01: 'XMoveOnShip',
    },
    b'XDOFIGHT': {
        0x01: 'XDoFight',
        0x05: 'TargetDestroyed(x)',
        0x06: 'FireWeapon()',
        0x0b: 'MouseToShipMovement()',
        0x0c: 'DoShooting()',  # Get mouse button state, if pressed, fire weapon, handle shooting
        0x10: 'TacticalMenu(x)',
        0x13: 'TractorBeam()',
    },
    b'XPILOTAG': { # XPILOTAGE
        0x01: 'XPilotage',
        0x04: 'WaitUserAction',
        0x06: 'NavigationMenuSetup',
        0x07: 'NavigationMenu',
        0x0a: 'MainMenuSetup',  # Only called if another menu appeared in the meantime
    },
    b'XDOINTER': { # XDOINTERACTION
        0x01: 'XDoInteraction(a,b,c)',
        0x0a: 'GetStringOffset(x):integer', # Get offset for string x
        0x0b: 'LookupString(x,y,addr)', # Copy string x (alt y if y>=0) to addr
        0x0c: 'SeekStringAlt(y,base,len)',
        0x0d: 'MapCase()', # Map character 0x5c/0x7c to 'I', and capitals to lower-case letters
        0x0e: 'GetNextByte(addr)',
        0x10: 'OuterIndirectOut(i)', # Process string at offset i*2: substitute other strings
        0x11: 'StringCharOut(x)', # Process string *x*: outer
        0x13: 'HandleChar(x)', # ?
        0x14: 'StringIndirectOut2(addr,?,?)', # ?
        0x16: 'StartInteraction(addr)',
        0x1d: 'ComputeStringOffsets(a,b,c)',
        0x22: 'WeirdLoadSector(addr,block)',
        0x24: 'InitDialogs()',  # Load main dialog words
        0x1f: 'LoadDialogBlocks(x)',  # Load misc dialog words/"code"
    },
    # Utilities
    b'MAINLIB ': {
        0x02: 'button_release',  # Wait until no mouse buttons pressed
        0x03: 'fore_sound',
        0x04: 'back_sound',
        0x05: 'quit',  # Fatal error ("red screen of death")
        0x06: 'color_value(r,g,b):integer',
        0x07: 'iand(a,b):integer', # Logical AND
        0x08: 'sign(x):integer', # Sign function: -1 if x<0, x if x==0, 1 if x>0
        0x09: 'isqrt',
        0x0a: 'condition(x;pval:^integer;y)', # *pval=min(max(*pval,x),y)
        0x0b: 'random():integer',
        0x0c: 'rand(low,high):integer',
        0x0d: 'set_money(addrout,?,value)',
        0x0e: 'more(addra,addrb):integer', # addra[0]>addrb[0] || (addra[0]==addrb[0] && addra[1]>addrb[1])
        0x0f: 'add(addra,addrb)', # Some 32-bit operation on two arguments
        0x10: 'sub(addra,addrb)', # Some 32-bit operation on two arguments (calls 0x0f)
        0x11: 'game_score():integer',
        0x12: 'get_checksum',           # Simple checksum
        0x13: 'decrypt(_,stride,_,_,addr,count,key)', # Sum then deobfuscate data
        0x14: 'encrypt(_,stride,_,_,addr,count,key)',   # Obfuscate data and sum
        0x15: 'str',
        0x16: 'vclear',
        0x17: 'vline',
        0x18: 'vgotoxy',
        0x19: 'moveto',
        0x1a: 'vchar',
        0x1b: 'vstring',
        0x1c: 'vcenter',
        0x1d: 'vnum',
        0x1e: 'wnum', # uses different print function
        0x1f: 'str_money(a,b,c)',
        0x20: 'vmoney(amountptr)',
        0x21: 'vinit(x)',  # Not sure what this actually sets
        0x22: 'draw_vinit',
        0x23: 'verasEOL',
        0x24: 'put_time(a,b,c,d,e)',
        0x25: 'tick_less',
        0x26: 'add_ticks',
        0x27: 'set_timeout',
        0x28: 'timeout',
        0x29: 'set_update',
        0x2a: 'need_update',
        0x2b: 'delay',
        0x2c: 'delay_button',  # Wait for mouse click or other event
        0x2d: 'freeze_time',   # Conditional wait on semaphore MAINLIB+0xa74
        0x2e: 'melt_time', # Conditional signal on semaphore MAINLIB+0xa74
        0x2f: 'backgnd_int',
        0x30: 'start_backgnd',
        0x31: 'stop_backgnd',
        0x32: 'title_color',
        0x33: 'util_color', # Do nothing
        0x34: 'copy_check', # Possibly set state so that RSOD appears
        0x35: 'the_time',
        0x36: 'screen_pos',
        0x37: 'color_win(color,n)', # Fill layout rectangle n with color
        0x38: 'color_rect(color,x0,y0,x1,y1)',
        0x39: 'invert_win',
        0x3a: 'invert_rect',
        0x3b: 'set_window',
        0x3c: 'get_window',
        0x3d: 'center(a,b,c)', # Called from PrintLn
        0x3e: 'draw_outline',
        0x3f: 'in_r_window',
        0x40: 'draw_poly',
        0x41: 'paddress',
        0x42: 'allocate',
        0x43: 'deallocate',
        0x44: 'error',
        0x45: 'disk_error', # Return 1 if error, 0 otherwise
        0x46: 'disk_in', # Read block 0x3a or 0x06 to see if library/sundog disk is in the drive as expected
        0x47: 'get_disk',  # Ask for a either library or sundog disk to be put in drive
        0x48: 'check_SBIOS',
        0x49: 'moveblock(?,lenwordsmin1,?,addr,count,block,?)', # Seems a general function for disk I/O
        0x4a: 'check_track', # Read an entire track, compute checksum?
        0x4b: 'fetch_epic(a,b,c,d,e)', # Load image resource?
        0x4c: 'fetch_at(a,b,c,addr_ptr)', # Load image resource?
        0x4d: 'fetch_dpic(addr_ptr,idx)', # Load image resource? idx 0x00..0x57
        # Internal functions
        0x4e: 'TextPrintHelper',
        0x4f: 'FormatPaddedNumber',
        0x50: 'TimeDigit', # local function of DrawTime
        0x51: 'Sign32(addra)', # (addra[0]<0 || addra[1]<0)?-1:1
        0x52: '?(i,a:bytearray)', # gets passed an index and a 4-byte array
        0x53: '',
        0x54: 'PossiblyRunDiskCheck',
    },
    b'WINDOWLI': {
        0x02: 'add_item',
        0x03: 'put_items',
        0x04: 'swap_items',
        0x05: 'find_item(x,y,a,b)', # Mouse pressed
        0x06: 'put_desc',
        0x07: 'desc_item', # Show item name
        0x08: 'remove_item',
        0x09: 'set_mark',
        0x0a: 'put_pocket',
        0x0b: 'clear_slot',
        0x0c: 'clear_menu',
        0x0d: 'add_char',
        0x0e: 'swap_menu',
        0x0f: 'put_menu',
        0x10: 'find_menu(i,x,y)', # Frequently called while mouse is clicked at position x,y
        0x11: 'confirm_menu',
        0x12: 'read_menu',
        0x13: 'new_menu(x)',
        0x14: 'set_box(i,text:string)',
        0x15: 'make_zoom(a,b,c)',
        0x16: 'pop_zoom',
        # Internal functions
        0x17: 'DrugEffects(item,b,c,d,e)',
        0x19: 'DrawButton(a,b,c)',
    },
    # Bindings
    b'GEMBIND ': {
        0x02: ('call_VDI(handle,opcode,numptsin,numintin)', 4, 0),
        0x03: ('call_AES(a,b,c,d,e,f)', 6, 0),
        0x04: ('moveram(from,to,bytes)', 3, 0), # Some memory copy function
        0x05: ('laddress(ptr,dst)', 2, 0),
        0x06: ('set_screen(x)', 1, 0), # Unused
        0x07: ('decode(addr1,addr2,val1,val2)', 4, 0),  # decode image
        0x08: ('do_sound(x)', 1, 0),
        0x09: ('SBIOS_sum', 3, 1), # Unused
        0x0a: ('format_track(a,b,c,d,e,f,g,h,i)', 9, 0),
        0x0b: ('set_color(color,index)', 2, 0),
        0x0c: 'address', # Unused
        0x0d: ('scroll(amount,direction)', 2, 0),
        0x0e: 'graf_mouse',
        0x0f: 'view_port',
        0x10: 'poly_line(numptsin)',
        0x11: 'line',  # One line
        0x12: 'wstring',  # Draw ASCII text
        0x13: 'wchar(ch,x,y)',  # Wrapper over 0x1d
        0x14: 'fill_rect',
        0x15: 'ell_arc(begang,endang,x,y,xradius,yradius)', # VDI 11, subfunction 6
        0x16: 'draw_mode', # VDI 32
        0x17: 'line_width', # VDI 16
        0x18: 'line_color', # VDI 17
        0x19: 'text_color', # Set a global
        0x1a: 'fill_color', # VDI 25
        0x1b: 'set_coll_color',
        0x1c: ('copy_opaque(psrc,pdst,vrmode,srcx1,srcx2,srcy1,srcy2,dstx1,dstx2,dsty1,dsty2)', 11, 0), # VDI 109
        0x1d: ('copy_trans(psrc,pdst,fgcolor,bgcolor,writemode,srcx1,srcx2,srcy1,srcy2,dstx1,dstx2,dsty1,dsty2)', 13, 0), # Draw black and white raster gfx VDI 121
        0x1e: 'get_mouse(addrx,addry)', # VDI 124
        0x1f: 'button', # VDI 124 (mouse button 0/1 passed in)
        0x20: 'draw_icon(idx,vrmode,x,y)', # calls 0x1c
        0x21: 'draw_char', # set writing mode, draw b/w
        0x22: 'draw_x_char', # calls DrawBW
        0x23: 'no_clip', # similar to 0x0f
        0x24: 'move_screen',
        0x25: 'restore_params',
        0x26: 'curse_on',
        0x27: 'curse_off',
        0x28: ('collide(pattern,x,y): integer', 4, 1),
        0x29: ('do_sprite(flag,back_addr,x,y,pattern,color)', 6, 0),
        0x2a: ('sprite_state(flag)', 1, 0),
        0x2b: ('birth_sprite(x,y,pattern,color,back,idx)', 6, 0), # 
        0x2c: ('attach_sprite(flag)', 1, 0),
        0x2d: 'move_sprite',
        0x2e: 'halt_sprite',
        0x2f: 'kill_sprite',
        0x30: 'draw_sprite',
    },
}
