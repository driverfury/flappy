# flappy
## Description

The goal of this project is to make a flappy bird clone in a single, small, self-contained executable file.

This is the final result:

![flappy](docs/img/gameplay.png)

The minimum size of the executable that I got is 32KB. The executable only links to KERNEL32.dll.

One future goal is to port it on WinXP.

## How I made it

First, I made the game without grafics:

![flappy without graphics](docs/img/gameplay_nogfx.png)

Btw you can compile the game without graphics with the preprocessor flag `GFX=0`.

I used my small [ezplat](https://github.com/driverfury/ezplat) library, so I made sure the executable was "self-contained" (no external libraries used, only stuff from default windows system).

The game was too big in size so I tried to remove the C runtime library by following [this guide](https://hero.handmade.network/forums/code-discussion/t/94-guide_-_how_to_avoid_c_c++_runtime_on_windows).

At the end, the game without graphics was 27KB in size.

### Graphics

Then graphics came into play.

I got two problems:

- a huge executable file
- the game was not a single exe file, it needed images (PNG files)

So, I "reduced" the images making them smaller.

As an example: the ground is only 24x22 resulting in 219 bytes (also considering the PNG headers).

This is the ground image:

![ground.png](docs/img/ground.png)

In the game I just draw this image many times.

Same applies for the skyscrapers, it's just a 140x50 image.

![skyscraper.png](docs/img/skyscraper.png)

I spared a ton of KBs.

Then I created a tool to "build the assets": `tools/build_addets.py`.

You provide it a json file which specifies the assets you wanna build, and it "packs" them together into a binary file with the following format:

```
+------------------------+
|        entry #0        |
+------------------------+
|        entry #1        |
+------------------------+
|        entry #2        |
+------------------------+
|        entry #3        |
+------------------------+
           ...
+------------------------+
|        entry #n        |
+------------------------+
```

Each entry is composed of:

```
+------------------------+
|   asset name: char[]   | <-- null-terminating and padded to 4 bytes
+------------------------+
|  image width: uint32   |
+------------------------+
|  image height: uint32  |
+------------------------+
|    # entries in the    |
|  palette table: uint32 |
+------------------------+
|     color #0 : uint32  |
+------------------------+
|     color #2 : uint32  |
+------------------------+
          ...
+------------------------+
|     color #m : uint32  |
+------------------------+
|   pixel (0,0) : uint8  | <-- here starts the pixels array
+------------------------+
|   pixel (1,0) : uint8  |
+------------------------+
            ...
+------------------------+
| pixel (w-1,h-1) : uint8|
+------------------------+
```

So I basically parsed every image, put every color used into a palette table and transformed the pixels into bytes (which are just pointers to palette entries).

The PNG compression algorithm does something similar, but doing it my way I don't have the PNG header, so I saved some space.

So I put everything into a binary files and compressed it with gzip (`tools/gzip`).

The idea is to decompress the file at run-time, so I created a small library which does that: [gzdec](https://github.com/driverfury/gzdec).

But I still have the problem of not having a single exe file, but I also have a `assets.bin.gz`. So I made `tools/bin2c.py` which converts a binary file into a C array.

For example, if you have a txt file with this content:

```
hello
```

it will be converted into this text file:

```
/* This is a generated file */
u8 CompAssets[] = {
    104,101,108,108,111
};
```

so you just need to include it in your C source code.

At the end I got a SINGLE, SELF-CONTAINED executable file of 32KB.
