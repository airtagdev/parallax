# Rewrite validation

The default C17 executable was built with MSVC 19.51 on Windows x64. It contains no JVM and does not run the original game as its backend. Original game resources remain local under ignored `release/resources/`.

## Resource data

The independent C JSON reader successfully parsed all 2,042 prepared recipe files and 1,866 advancement files. It found 126 displayed advancement definitions and 4,045 criteria, with no invalid/missing trigger fields. These are catalog integrity results, not implemented gameplay or working advancements. The prepared 26.3 metadata pins protocol 777 and data version 5023.

## Random stream

The original `tools/RandomOracle.java` harness calls the installed JDK's public `java.util.Random` API for 4,096 seeds, including zero, negative values and both long endpoints. The independent C implementation matches each scenario's nextInt, rejection-sensitive nextInt(bound), nextLong, float bits and double bits. The C arithmetic avoids signed overflow. This establishes those stream operations for these sequences; Gaussian draws and other Minecraft RNGs are not covered.

## Native screenshot comparison

A native 26.3 process was launched from the owned installation with an isolated game directory and dummy offline reference identity. Its own F2 screenshot supplied the reference frame, avoiding occluded-desktop captures. Both images were 1440x900, English, default resources and GUI scale 3.

`ParallaxImageCompare` is a C/WIC test executable. It compares every RGB pixel in seven complete title-widget rectangles:

| Region | Compared pixels | Different pixels |
| --- | ---: | ---: |
| Singleplayer | 36,000 | 0 |
| Multiplayer | 36,000 | 0 |
| Minecraft Realms | 36,000 | 0 |
| Options | 17,640 | 0 |
| Quit Game | 17,640 | 0 |
| Language | 3,600 | 0 |
| Accessibility | 3,600 | 0 |

The comparison caught and corrected 26.3's extra appearance-button row, narrow-widget tiling, and icon sizing/placement. No background pixels were masked out inside those rectangles. The native screenshot is kept privately in `.local/`, not committed.

## Explicit limits

This does not verify the entire title screen, animation, hover/focus/click behavior, sounds, other scales/languages, any other menu, native gameplay, world formats, world generation or advancement execution. The panorama's exact filtering/orientation/timing, splash, player appearance and intro fade still differ or are missing. There is no completed or playable Minecraft rewrite yet. See PARITY.md.
