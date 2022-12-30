FTL copy protection
--------------------

It looks very much that the weird disk scan is part of a copy protection scheme,
spread throughout the entire game.

Apparently, FTL's paranoid take on copy protection has been a discussion topic in
the more recent past.

[Dan Dippold](http://www.v2.gamasutra.com/view/news/263656/A_look_back_at_the_dawn_of_video_game_DRM__and_those_who_cracked_it.php#comment279465) on Gamasutra writes:

> Sundog: Frozen Legacy on the Apple II was one of the most notoriously hard to
> crack (and have it work properly) games on the system. Also by FTL, and I guess
> you can view the DRM as an early run for Dungeon Master - it had frequent code
> loads, and DRM checks everywhere.
> 
> Another thing that really helped there is that it was in Apple Pascal, which
> used the UCSD P-system virtual machine, so your executable was actually p-code
> for the P-system VM. This sort of thing is everywhere today, but it was way too
> computer sciencey and abstract for your average teen cracker at the time, like
> me. I actually knew Apple Pascal and realized what was going on but decided
> that would be way too much of a pain in the ass to deal with and moved on to
> other games. Copy protection - success!

The Atari ST version isn't very different than the Apple II version with
regard to this.

Jimmy Maher on [The Digital Antiquarian](http://www.filfre.net/2016/01/a-pirates-life-for-me-part-3-case-studies-in-copy-protection/)
has a case study in Dungeon Master's copy protection. There's quite some overlap.
although that one went even more full out nuts:

> Instead of checking the copy protection just once, Dungeon Master does it over
> and over, from half-a-dozen or so different places in its code, turning the
> cracker’s job into a game of whack-a-mole. Every time he thinks he’s got it at
> last, up pops another check. The most devious of all the checks is the one
> that’s hidden inside a file called “graphics.dat,” the game’s graphics store.
> Who would think to look for executable code there?

### Related variables

- GEMBIND\_238 main state word
- MAINLIB\_3b1 checksum computed in XMOVEONS:0x06, compared to 0x13a in MAINLIB:0x34, set to 0x13a
- MAINLIB\_392 set to 0x120 in MAINLIB:0x34, compared to 0x120
- MAINLIB\_3ab checksum wrong counter if 0xcc37
- MAINLIB\_3ac checksum wrong counter if 0x129d
- MAINLIB\_3ad error counter increased in MAINLIB:0x4a WeirdDiskCheck
- MAINLIB\_a73 compared to 0xab7 in MAINLIB:0x34
- SHIPLIB\_6  pointer to sector used in XMOVEONS:0x06
- MAINLIB\_435 packed array of tracks visited

### Related procedures

- MAINLIB:0x34 PossiblySetEvilState: possibly sets MAINLIB\_238 to 0x04d2
  - check depends on whether state is 0x129d, 0xcc37
  - checks MAINLIB\_3b1, MAINLIB\_392, MAINLIB\_a73
  - keeps counters MAINLIB\_3ab, MAINLIB\_3ac
  - if MAINLIB\_3ad + MAINLIB\_3ac + MAINLIB\_3ab >= 3 set state to 0x4d2 (dead), else 0xf71a
- XMOVEONS:0x06 
  - computes checksum of 512-byte sector to MAINLIB\_3b1
  - sets GEMBIND\_238 to 0xfe93
  - sets MAINLIB\_a73 to ?[0x23] * ?[0x66]
  - SHIPLIB\_6 is used as address of the sector
- MAINLIB:0x35
  - sets MAINLIB\_238 to 0x04d2 (evil state) if it was 0x000d
  - sets it to 0x129d if it was 0xfe93
- MAINLIB:0x4a WeirdDiskCheck
  - checks a random track against list of checksums (see `tools/compute_checksums.py`)
  - for track 0 do some extra processing
- XDOINTER:0x1f  Reads a sector, MAINLIB\_3ab
- XDOUSERM:0x0f  GEMBIND\_238, MAINLIB\_3ab
- XSTARTUP:0x06  Initializes MAINLIB\_43a, MAINLIB\_43c, MAINLIB\_3ad, MAINLIB\_3ac, MAINLIB\_3ab
  MAINLIB\_435, MAINLIB\_3b4, MAINLIB\_3b5, MAINLIB\_3b2, GEMBIND\_22e,
  GEMBIND\_230
  - Calls unitstatus(0x80). Looks like the mother function for all integrity-check-related behavior.
  - First dword (overwritable data?) goes to MAINLIB\_3b2
  - Second dword (jump table address) goes to GEMBIND\_22e, where it looks like never used
  - Third word goes to MAINLIB\_3b0 + 1, added 'A' = drive letter
- SUNDOG:0x03 allocates 3\*512 bytes then reads sector 7 then sector 4,5,6 (into
  same buffer, at offset 0) - as the first read is discarded this is clearly a
  way to get a certain instance of a duplicate sector by directing the drive.
  This reads 0x300 words into [SHIPLIB\_6]
- XDOINTER:0x22 Reads a sector then immediately reads sector 4.

### Weird, may be related or not

- SHIPLIB:0x17 Generate 256 random bytes at address SHIPLIB+0x06

States
--------

States of GEMBIND\_238. These are used to communicate the current state of the integrity
check to different parts of the game code:

     0x000d   Set if MAINLIB:0x4a WeirdDiskCheck error, changed to 0x4d2 in MAINLIB:0x35
     0x0012   Do the checksum compute in XMOVEONS:0x06
     0x04d2   Red screen of death (draw red rectangle, hang game)
     0x129d   Tested for in MAINLIB:0x34, set in MAINLIB:0x35
     0x4766   Never tesetd for, set in XSTARTUP:0x03 InitGFX
     0x8ad0   Never tested for, set in MAINLIB:0x4a WeirdDiskCheck when increasing 0x3ad error counter
     0xcc37   Tested for in MAINLIB:0x34, set in XDOINTER:0x1f
     0xf71a   Never tested for, set in MAINLIB:0x34
     0xfe93   Checksum compute in XMOVEONS:0x06 done, MAINLIB_a73 and MAINLIB_3b1 are filled in, changed to 0x129d in MAINLIB:0x35

State changes
-------------------

```
??? state changed to 0x4766
Traceback [tib=006e sp=e976]
  XSTARTUP:0x03:0162 mp=0xe976 base=0x0000 erec=0x1b2a
  XSTARTUP:0x01:2e07 mp=0xe982 base=0x0000 erec=0x1b2a
  DONESOFA:0x02:0069 mp=0xfdb2 base=0x0000 erec=0x184e
  SUNDOG  :0x01:0311 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x008a erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0x000d
Traceback [tib=006e sp=d366]
  MAINLIB :0x4a:122d mp=0xd366 base=0x1f66 erec=0x18d8
  XSTARTUP:0x0f:0847 mp=0xe576 base=0x0000 erec=0x1b2a
  XSTARTUP:0x01:2e11 mp=0xe982 base=0x0000 erec=0x1b2a
  DONESOFA:0x02:0069 mp=0xfdb2 base=0x0000 erec=0x184e
  SUNDOG  :0x01:0311 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x4eab erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0x8ad0
Traceback [tib=006e sp=d366]
  MAINLIB :0x4a:1282 mp=0xd366 base=0x1f66 erec=0x18d8
  XSTARTUP:0x0f:0847 mp=0xe576 base=0x0000 erec=0x1b2a
  XSTARTUP:0x01:2e11 mp=0xe982 base=0x0000 erec=0x1b2a
  DONESOFA:0x02:0069 mp=0xfdb2 base=0x0000 erec=0x184e
  SUNDOG  :0x01:0311 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x4eab erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0xf71a
Traceback [tib=6348 sp=72da]
  MAINLIB :0x34:0b5b mp=0x72da base=0x1f66 erec=0x18d8
  SUNDOG  :0x02:00e2 mp=0x72e4 base=0x4f02 erec=0x17da
  SUNDOG  :0x03:02fb mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x4eab erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0x0012
Traceback [tib=006e sp=fdae]
  SUNDOG  :0x03:018d mp=0xfdae base=0x4f02 erec=0x17da
  SUNDOG  :0x01:0322 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0xfe93
Traceback [tib=006e sp=fd62]
  XMOVEONS:0x06:0234 mp=0xfd62 base=0x1c8e erec=0x19a2
  XMOVEONS:0x09:03fb mp=0xfd78 base=0x1c8e erec=0x19a2
  XMOVEONS:0x01:05d3 mp=0xfd8c base=0x1c8e erec=0x19a2
  SHIPLIB :0x08:0061 mp=0xfda4 base=0x1c8e erec=0x187c
  SUNDOG  :0x03:01ca mp=0xfdae base=0x4f02 erec=0x17da
  SUNDOG  :0x01:0322 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0x129d
Traceback [tib=006e sp=fcce]
  MAINLIB :0x35:0c40 mp=0xfcce base=0x1f66 erec=0x18d8
  XMOVEONG:0x14:0ac6 mp=0xfcfa base=0x0000 erec=0x1c32
  XMOVEONG:0x01:282f mp=0xfd12 base=0x0000 erec=0x1c32
  GRNDLIB :0x03:002f mp=0xfd90 base=0x0000 erec=0x1820
  SUNDOG  :0x04:0267 mp=0xfda6 base=0x4f02 erec=0x17da
  SUNDOG  :0x01:0326 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0xf71a
Traceback [tib=6348 sp=72da]
  MAINLIB :0x34:0b5b mp=0x72da base=0x1f66 erec=0x18d8
  SUNDOG  :0x02:00e2 mp=0x72e4 base=0x4f02 erec=0x17da
  SUNDOG  :0x03:02fb mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
screen v_pline vr=2 col=15 width=1 count=12
```
```
??? state changed to 0x04d2
Traceback [tib=6348 sp=72da]
  MAINLIB :0x34:0b52 mp=0x72da base=0x1f66 erec=0x18d8   PossiblySetEvilState
  SUNDOG  :0x02:00e2 mp=0x72e4 base=0x4f02 erec=0x17da
  SUNDOG  :0x03:02fb mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```

Communication with FTL
----------------------

In a mail, Wayne Holder writes:

> I've done some reverse engineering to figure out a bit more about the copy
> protection mechanism used by Atart ST Sundog.  Part of the trick is that
> track 3 of the floppy disk has 10 sectors rather than the standard 9 sectors.
> The additional sector is a duplicate of sector 5 (that is, there are two
> sectors that both identify as sector 5.)  The contents of these two sectors
> is identical except that the byte at offset 0x113 (275) is set to 0xBC in one
> sector and to 0xA2 in the other.  In addition, the sectors on track 3 are
> written in this order (1, 4, 7, 5, 2, 5, 8, 3, 6, 9).  This means that the
> code can control which version of sector 5 it reads by varying which sector
> it reads before it tries to read sector 5.

So I was right that it is the copy protection. This quite closely describes
what is going on so it should be possible to emulate this and forego the wild
goose chase.
