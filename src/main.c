#define EZPLAT_IMPLEMENTATION
#include "ezplat.h"

#ifndef GFX
#define GFX 1
#endif

// To make MSVC not complain
int _fltused;

#include <windows.h>

#include <inttypes.h>
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int16_t  i16;

#define MIN(X,Y) (((X)<(Y))?(X):(Y))
#define MAX(X,Y) (((X)>(Y))?(X):(Y))

#define PAD(X,N) (((X)%(N)!=0)?((((X)/(N))+1)*(N)):(X))
#define PADDING(X,N) (PAD((X),(N))-(X))

#define ARRCOUNT(A) (sizeof(A)/sizeof((A)[0]))

int
StrLen(char *s)
{
    int Len = 0;
    while(*s++) ++Len;
    return(Len);
}

int
StrEq(char *s1, char *s2)
{
    int Len1 = StrLen(s1);
    int Len2 = StrLen(s2);

    if(Len1 != Len2)
    {
        return(0);
    }

    while(*s1)
    {
        if(*s1++ != *s2++)
        {
            return(0);
        }
    }

    return(1);
}

void *
MemAlloc(unsigned int Size)
{
    return(VirtualAlloc(
        0, (SIZE_T)Size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE));
}

void
MemFree(void *Ptr)
{
    VirtualFree(Ptr, 0, MEM_RELEASE | MEM_DECOMMIT);
}

/**
 * GZIP decompressor
 */

#define GZDEC_IMPLEMENTATION
#include "gzdec.h"

/**
 * Random number generator
 * Linear congruential generator
 *
 * Xn+1 = (a*Xn + c) mod m
 *
 * We use a, c, m as used by GCC
 * a = 1103515245
 * c = 12345
 * m = 2147483648
 */

#define RNDA 1103515245
#define RNDC 12345
#define RNDM 2147483648

int PrevRandomInt = 887;

void
RandomSeed(int Seed)
{
    PrevRandomInt = Seed;
}

int
GetRandomInt(void)
{
    int Res = (RNDA*PrevRandomInt + RNDC) % RNDM;
    PrevRandomInt = Res;
    return(Res);
}

int
GetRandomIntBetween(int MinVal, int MaxVal)
{
    int Num = GetRandomInt();
    Num = (Num % (MaxVal - MinVal + 1)) + MinVal;
    return(Num);
}

#if GFX
#include "resources.h"
#endif

u8 *Assets;
unsigned int AssetsSize;

void
GetImageWH(void *Asset, int *W, int *H)
{
    u8 *Ptr = Asset;
    *W = (int)*((u32 *)Ptr);
    Ptr += sizeof(u32);
    *H = (int)*((u32 *)Ptr);
    Ptr += sizeof(u32);
}

u32 *
GetImagePalette(void *Asset)
{
    u8 *Ptr = Asset;
    Ptr += 2*sizeof(u32);
    return((u32 *)Ptr);
}

u8 *
GetImagePixels(void *Asset)
{
    u8 *Ptr = Asset;
    Ptr += 2*sizeof(u32);

    u32 PaletteSize = *((u32 *)Ptr);
    Ptr += sizeof(u32);
    Ptr += PaletteSize*sizeof(u32);

    return((u8 *)Ptr);
}

void *
SearchAsset(char *Name)
{
    void *Result = 0;
    u8 *Ptr = (u8 *)Assets;
    u8 *End = Ptr + AssetsSize;

    int Found = 0;
    while(Ptr < End)
    {
        char *CurrentName = (char *)Ptr;
        int Len = StrLen(CurrentName);
        if(StrEq(CurrentName, Name))
        {
            Found = 1;
            Result = Ptr + PAD(Len + 1, 4);
            break;
        }

        Ptr += PAD(Len + 1, 4);

        u32 W = *((u32 *)Ptr);
        Ptr += sizeof(u32);
        u32 H = *((u32 *)Ptr);
        Ptr += sizeof(u32);

        // Skip palette
        u32 PaletteSize = *((u32 *)Ptr);
        Ptr += sizeof(u32);
        Ptr += PaletteSize*sizeof(u32);

        // Skip pixels
        Ptr += PAD(W*H, 4);
    }

    if(!Found)
    {
        // TODO: Log
    }

    return(Result);
}

#define SCRW 400
#define SCRH 600
u32 FrameBuffer[SCRW*SCRH];

void *ImgSkyscraper;
void *ImgGround;
void *ImgBird;
void *ImgPipe;
void *ImgDigits;
void *ImgPressSpace;
void *ImgGameOver;

void
Clear(u32 Color)
{
    u32 *Ptr = (u32 *)FrameBuffer;
    for(int Y = 0;
        Y < SCRH;
        ++Y)
    {
        for(int X = 0;
            X < SCRW;
            ++X)
        {
            *Ptr++ = Color;
        }
    }
}

void
FillRectangle(
    int X1, int Y1,
    int X2, int Y2,
    u32 Color)
{
    X1 = MAX(0, X1);
    Y1 = MAX(0, Y1);
    X2 = MIN(SCRW-1, X2);
    Y2 = MIN(SCRH-1, Y2);

    for(int Y = Y1;
        Y <= Y2;
        ++Y)
    {
        if(Y >= SCRH)
        {
            break;
        }

        u32 *Ptr = &(FrameBuffer[Y*SCRW + X1]);
        for(int X = X1;
            X <= X2;
            ++X)
        {
            if(X >= SCRW)
            {
                break;
            }

            *Ptr++ = Color;
        }
    }
}

void
FillCircle(int CenterX, int CenterY, int Radius, u32 Color)
{
    int StartX = CenterX - Radius;
    int StartY = CenterY - Radius;
    int EndX = CenterX + Radius;
    int EndY = CenterY + Radius;

    StartX = MAX(0, StartX);
    StartY = MAX(0, StartY);
    EndX = MIN(SCRW, EndX);
    EndY = MIN(SCRH, EndY);

    int Radius2 = Radius*Radius;
    for(int Y = StartY;
        Y < EndY;
        ++Y)
    {
        for(int X = StartX;
            X < EndX;
            ++X)
        {
            int DX = (X - CenterX);
            int DY = (Y - CenterY);
            int Distance2 = DX*DX + DY*DY;

            if(Distance2 <= Radius2)
            {
                FrameBuffer[Y*SCRW + X] = Color;
            }
        }
    }
}

#define TRSPMASK 0xff00ff

void
DrawImageEx(
    void *Image,
    int SrcX, int SrcY, int SrcW, int SrcH,
    int DstX, int DstY)
{
    int ImageW;
    int ImageH;

    GetImageWH(Image, &ImageW, &ImageH);

    u32 *Palette = GetImagePalette(Image);
    ++Palette;

    u8 *Pixels = GetImagePixels(Image);

    if(SrcX >= ImageW || SrcY >= ImageH)
    {
        return;
    }

    if(DstX < 0)
    {
        SrcX -= DstX;
        SrcW += DstX;
        DstX = 0;
    }

    if(DstY < 0)
    {
        SrcY -= DstY;
        SrcH += DstY;
        DstY = 0;
    }

    SrcX = MAX(0, SrcX);
    SrcY = MAX(0, SrcY);
    SrcW = MIN(SrcW, ImageW - SrcX);
    SrcH = MIN(SrcH, ImageH - SrcY);

    for(int Y = 0;
        Y < SrcH;
        ++Y)
    {
        if(DstY + Y >= SCRH)
        {
            break;
        }

        for(int X = 0;
            X < SrcW;
            ++X)
        {
            if(DstX + X >= SCRW)
            {
                break;
            }

            u32 Color = Palette[Pixels[(SrcY + Y)*ImageW + SrcX + X]];
            if((Color & 0xffffff) == (TRSPMASK & 0xffffff))
            {
                continue;
            }

            FrameBuffer[(DstY + Y)*SCRW + DstX + X] = Color;
        }
    }
}

void
DrawImage(u32 *Image, int X, int Y)
{
    int ImageW = (int)Image[0];
    int ImageH = (int)Image[1];

    DrawImageEx(Image, 0, 0, ImageW, ImageH, X, Y);
}

#define SKYCLR 0x4ec0ca
#define SKYCLRN 0x008793

#define GRSCLR 0x5ee270
#define GRSCLRN 0x00a300

#define GRSH 145

#define SKYSCRW 70
#define SKYSCRH 50

#define CLDCLR 0xE9FCD9
#define CLDCLRN 0x00B3C2

i16 CloudsValues[] = {
    -10,  20, 30,
     40,  20, 40,
     90,  20, 40,
     70, -10, 20,
     120, 10, 20,
     130, 30, 50,
     180, 30, 50,
     200, 10, 30,
     240, 20, 30,
     260, 20, 30,
     280, 25, 35,
     310, 30, 50,
     325, 20, 30,
     340, 10, 35,
     370, 30, 50
};

#define STRCLR 0xaee8d2

i16 StarsValues[] = {
    30, 100, 2,
    60, 150, 1,
    100, 130, 1,
    130, 110, 2,
    150, 120, 1,
    190, 100, 1,
    220, 130, 2,
    250, 110, 1,
    250, 140, 1,
    280, 110, 1,
    310, 130, 1,
    330, 150, 2,
    360, 110, 1,
    380, 130, 1,
    380, 150, 1,
    390, 110, 1,
};

void DrawBackground(int NightMode)
{
#if GFX
    // Draw sky
    Clear(NightMode ? SKYCLRN : SKYCLR);

    if(NightMode)
    {
        // Draw stars
        i16 *StarPtr = (i16 *)&(StarsValues[0]);
        int StarLen = ARRCOUNT(StarsValues) / 3;
        for(int StarIndex = 0;
            StarIndex < StarLen;
            ++StarIndex)
        {
            // TODO: For size 2 draw the star as a + instead of a dot .
            int StarX = (int)*StarPtr++;
            int StarY = (int)*StarPtr++;
            int StarSize = (int)*StarPtr++;
            FillRectangle(
                StarX, StarY,
                StarX + StarSize, StarY + StarSize,
                STRCLR);
        }
    }


    // Draw clouds
    int CloudBaseY = SCRH - GRSH - SKYSCRH;
    u32 CloudColor = NightMode ? CLDCLRN : CLDCLR;
    
    i16 *CloudPtr = (i16 *)&(CloudsValues[0]);
    int CloudLen = ARRCOUNT(CloudsValues) / 3;
    for(int CloudIndex = 0;
        CloudIndex < CloudLen;
        ++CloudIndex)
    {
        int CloudX = (int)*CloudPtr++;
        int CloudOffY = (int)*CloudPtr++;
        int CloudRad = (int)*CloudPtr++;
        FillCircle(CloudX, CloudBaseY + CloudOffY, CloudRad, CloudColor);
    }

    // Draw grass
    FillRectangle(0, SCRH - GRSH, SCRW, SCRH, NightMode ? GRSCLRN : GRSCLR);

    // Draw skyscrapers
    int SrcX = (NightMode ? 1 : 0)*SKYSCRW;
    int DstY = SCRH - GRSH - SKYSCRH;
    int ToFill = (SCRW / SKYSCRW) + 1;
    for(int Index = 0;
        Index < ToFill;
        ++Index)
    {
        DrawImageEx(
            ImgSkyscraper,
            SrcX, 0,
            SKYSCRW, SKYSCRH,
            Index*SKYSCRW, DstY);
    }
#else
    Clear(NightMode ? SKYCLRN : SKYCLR);
#endif
}

#define GRNDTOPW 24
#define GRNDTOPH 22
#define GRNDH 120
#define GRNDY SCRH - GRNDH
#define GRNDCLR 0xded895

void DrawGround(int GroundX)
{
#if GFX
    FillRectangle(0, GRNDY, SCRW, SCRH, GRNDCLR);

    int Covered = 0;
    int Counter = 0;
    while(Covered < SCRW + GRNDTOPW)
    {
        DrawImageEx(
            ImgGround,
            0, 0,
            GRNDTOPW, GRNDTOPH,
            GroundX + Counter*GRNDTOPW, GRNDY);
        Covered += GRNDTOPW;
        ++Counter;
    }
#else
    UNREFERENCED_PARAMETER(GroundX);
    FillRectangle(0, GRNDY, SCRW, SCRH, GRNDCLR);
#endif
}

#define PLAYERW 34
#define PLAYERH 24

void DrawPlayer(float PlayerX, float PlayerY, float Index)
{
#if GFX
    DrawImageEx(
        ImgBird,
        (int)Index*PLAYERW, 0,
        PLAYERW, PLAYERH,
        (int)PlayerX, (int)PlayerY);
#else
    UNREFERENCED_PARAMETER(Index);
    FillRectangle(
        (int)PlayerX, (int)PlayerY,
        (int)(PlayerX+PLAYERW), (int)(PlayerY+PLAYERH),
        0xff0000);
#endif
}

#define DIGW 24
#define DIGH 36

char ScoreString[11] = {0};

void DrawScore(unsigned int Score)
{
#if GFX
    unsigned int ScoreStringLen = 0;
    unsigned int Div = 1000000000;
    unsigned int Digit;

    if(Score == 0)
    {
        ScoreString[0] = '0';
        ScoreString[1] = 0;
        ScoreStringLen = 1;
    }
    else
    {
        while(Score / Div == 0)
        {
            Div /= 10;
        }

        while(Div >= 1)
        {
            Digit = (Score / Div);
            ScoreString[ScoreStringLen++] = (char)(Score / Div) + '0';
            Score -= Digit*Div;
            Div /= 10;
        }
    }

    unsigned int TextLen = ScoreStringLen*DIGW;
    unsigned int X = SCRW/2 - TextLen/2;
    unsigned int Y = 20;

    for(unsigned int Index = 0;
        Index < ScoreStringLen;
        ++Index)
    {
        Digit = (unsigned int)(ScoreString[Index] - '0');
        DrawImageEx(
            ImgDigits,
            Digit*DIGW, 0,
            DIGW, DIGH,
            X, Y);
        X += DIGW;
    }
#else
    UNREFERENCED_PARAMETER(Score);
#endif
}

#define PIPW    52
#define PIPENDH 26
#define PIPTRKH 20

#define PIPGAP 100

#define PIP0CLR 0x1a8f00
#define PIP1CLR 0xb87049

void DrawPipe(int X, int Y, int Type)
{
#if GFX
    Type = MAX(0, Type);
    Type = MIN(1, Type);

    int EndY = Y - PIPENDH;
    int CurrY = 0;
    while(CurrY < EndY)
    {
        int H = PIPTRKH;
        if(EndY - CurrY < PIPTRKH)
        {
            H = EndY - CurrY;
        }

        DrawImageEx(
            ImgPipe,
            Type*PIPW, PIPENDH,
            PIPW, H,
            X, CurrY);

        CurrY += H;
    }
    DrawImageEx(
        ImgPipe,
        Type*PIPW, PIPENDH + PIPTRKH,
        PIPW, PIPENDH,
        X, CurrY);

    CurrY += PIPENDH;
    CurrY += PIPGAP;

    DrawImageEx(
        ImgPipe,
        Type*PIPW, 0,
        PIPW, PIPENDH,
        X, CurrY);

    CurrY += PIPENDH;

    EndY = SCRH - 1;
    while(CurrY < EndY)
    {
        int H = PIPTRKH;
        if(EndY - CurrY < PIPTRKH)
        {
            H = EndY - CurrY;
        }

        DrawImageEx(
            ImgPipe,
            Type*PIPW, PIPENDH,
            PIPW, H,
            X, CurrY);

        CurrY += H;
    }
#else
    u32 Color = Type ? PIP1CLR : PIP0CLR;
    FillRectangle(X, 0, X + PIPW, Y, Color);
    FillRectangle(X, Y + PIPGAP, X + PIPW, GRNDY, Color);
#endif
}

typedef struct pipe
{
    float X;
    float Y;
    int PlayerIn;
    int Type;
} pipe;

#define MAXPIPES 10
pipe Pipes[MAXPIPES];
int PipesCount;

void
ResetPipes(void)
{
    PipesCount = 0;
}

pipe *
PushPipe(float X, float Y, int Type)
{
    if(PipesCount >= MAXPIPES)
    {
        return(0);
    }

    pipe *Pipe = &(Pipes[PipesCount++]);
    Pipe->X = X;
    Pipe->Y = Y;
    Pipe->PlayerIn = 0;
    Pipe->Type = Type;
    return(Pipe);
}

pipe *
GetPipe(int Index)
{
    if(Index < 0 || Index >= MAXPIPES)
    {
        return(0);
    }

    return(&(Pipes[Index]));
}

void
RemovePipe(int Index)
{
    if(Index < 0 || Index >= MAXPIPES)
    {
        return;
    }

    for(int I = Index + 1;
        I < PipesCount;
        ++I)
    {
        Pipes[I - 1] = Pipes[I];
    }

    --PipesCount;
}

void
GeneratePipe(int PipeLevel)
{
    int HalfH = SCRH/2 - GRNDH/2;
    int Y = GetRandomIntBetween(HalfH - PipeLevel, HalfH + PipeLevel);
    int Type = GetRandomInt() % 2;
    PushPipe(SCRW, (float)Y, Type);
}

void
DrawPipes(void)
{
    for(int PipeIndex = 0;
        PipeIndex < PipesCount;
        ++PipeIndex)
    {
        pipe *Pipe = GetPipe(PipeIndex);
        if(!Pipe)
        {
            continue;
        }

        DrawPipe((int)Pipe->X, (int)Pipe->Y, Pipe->Type);
    }
}

#if GFX
#define PSPACEW 207
#define PSPACEH 36
#else
#define PSPACEBLOCK 4
#define PSPACECOLS 21
#define PSPACEROWS 11
#define PSPACEW PSPACECOLS*PSPACEBLOCK
#define PSPACEH PSPACEROWS*PSPACEBLOCK
u8 PressSpaceBlocks[PSPACECOLS*PSPACEROWS] = {
    0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,
    0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,0,0,1,0,0,0,
    0,1,1,1,0,1,1,1,0,1,1,0,0,1,1,1,0,1,1,1,0,
    0,1,0,0,0,1,1,0,0,1,0,0,0,0,0,1,0,0,0,1,0,
    0,1,0,0,0,1,0,1,0,1,1,1,0,1,1,1,0,1,1,1,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,
    0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,0,0,
    0,1,1,1,0,1,1,1,0,1,1,1,0,1,0,0,0,1,1,0,0,
    0,0,0,1,0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,0,0,
    0,1,1,1,0,1,0,0,0,1,0,1,0,1,1,1,0,1,1,1,0
};
#endif

void
DrawPressSpace(int X, int Y, int Index)
{
#if GFX
    DrawImageEx(
        ImgPressSpace,
        0, PSPACEH*Index,
        PSPACEW, PSPACEH,
        X, Y);
#else
    for(int BlockY = 0;
        BlockY < PSPACEROWS;
        ++BlockY)
    {
        for(int BlockX = 0;
            BlockX < PSPACECOLS;
            ++BlockX)
        {
            if(PressSpaceBlocks[BlockY*PSPACECOLS + BlockX])
            {
                FillRectangle(
                    X + BlockX*PSPACEBLOCK,
                    Y + BlockY*PSPACEBLOCK,
                    X + (BlockX + 1)*PSPACEBLOCK,
                    Y + (BlockY + 1)*PSPACEBLOCK,
                    0xffffffff);
            }
        }
    }
#endif
}

#if GFX
#define GOVERW 192
#define GOVERH 42
#else
#define GOVERBLOCK 8
#define GOVERCOLS 17
#define GOVERROWS 11
#define GOVERW GOVERCOLS*GOVERBLOCK
#define GOVERH GOVERROWS*GOVERBLOCK
u8 GameOverBlocks[GOVERCOLS*GOVERROWS] = {
    0,1,1,1,0,1,1,1,0,1,0,1,0,1,1,1,0,
    0,1,0,0,0,1,0,1,0,1,1,1,0,1,0,0,0,
    0,1,0,0,0,1,1,1,0,1,0,1,0,1,1,0,0,
    0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,0,0,
    0,1,1,1,0,1,0,1,0,1,0,1,0,1,1,1,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,1,1,1,0,1,0,1,0,1,1,1,0,1,1,1,0,
    0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,
    0,1,0,1,0,1,0,1,0,1,1,0,0,1,1,1,0,
    0,1,0,1,0,1,0,1,0,1,0,0,0,1,1,0,0,
    0,1,1,1,0,0,1,0,0,1,1,1,0,1,0,1,0
};
#endif

void
DrawGameOver(int X, int Y)
{
#if GFX
    DrawImageEx(ImgGameOver, 0, 0, GOVERW, GOVERH, X, Y);
#else
    for(int BlockY = 0;
        BlockY < GOVERROWS;
        ++BlockY)
    {
        for(int BlockX = 0;
            BlockX < GOVERCOLS;
            ++BlockX)
        {
            if(GameOverBlocks[BlockY*GOVERCOLS + BlockX])
            {
                FillRectangle(
                    X + BlockX*GOVERBLOCK,
                    Y + BlockY*GOVERBLOCK,
                    X + (BlockX + 1)*GOVERBLOCK,
                    Y + (BlockY + 1)*GOVERBLOCK,
                    0xffffffff);
            }
        }
    }
#endif
}

int BoxCollision(
    int X1, int Y1, int W1, int H1,
    int X2, int Y2, int W2, int H2)
{
    int XColl = ((X1 < X2 + W2) && (X1 + W1 > X2));
    int YColl = ((Y1 < Y2 + H2) && (Y1 + H1 > Y2));
    return((XColl && YColl) ? 1 : 0);
}

#define GRVTY 0.001f
#define MAXVEL 0.15f
#define MAXVELDEAD 0.30f
#define JMPVEL -0.3f
#define WRLDVEL -0.10f

enum
{
    GAME_IDLE,
    GAME_PLAY,
    GAME_DEAD,
    GAME_OVER,
};

void
main(void)
{
    ez Ez = {0};
    Ez.Canvas.Name = "flappy";
    Ez.Canvas.Width = SCRW;
    Ez.Canvas.Height = SCRH;
    Ez.Canvas.Pixels = (void *)FrameBuffer;
    if(!Ez.Canvas.Pixels)
    {
        ExitProcess(1);
    }

    if(!EzInitialize(&Ez))
    {
        ExitProcess(1);
    }

    /* Load assets */
#if GFX
    AssetsSize = gzdecsize((void *)CompAssets, sizeof(CompAssets));
    Assets = (u8 *)MemAlloc(AssetsSize);
    int DecompResult = gzdec(
        (void *)CompAssets, sizeof(CompAssets),
        Assets, AssetsSize);
    if(DecompResult != GZ_OK)
    {
        ExitProcess(2);
    }

    ImgSkyscraper = SearchAsset("ImgSkyscraper");
    ImgGround = SearchAsset("ImgGround");
    ImgBird = SearchAsset("ImgBird");
    ImgPipe = SearchAsset("ImgPipe");
    ImgDigits = SearchAsset("ImgDigits");
    ImgPressSpace = SearchAsset("ImgPressSpace");
    ImgGameOver = SearchAsset("ImgGameOver");
#endif

    // TODO: Assert that asset are found

    SYSTEMTIME SysTime;
    GetLocalTime(&SysTime);
    RandomSeed(SysTime.wMilliseconds);
    int NightMode = (SysTime.wHour >= 17) ? 1 : 0;

    float WorldV = WRLDVEL;

    float PlayerX = 30;
    float PlayerY = SCRH/2;
    float PlayerV = 0;
    float PlayerAnimation = 0.0f;
    float PlayerAnimationSpeed = 0.01f;
    float PlayerAnimationMax = 3.0f;
    unsigned int Score = 0;

    float GroundX = 0.0;

    float PipeToPipe = 100;
    int PipeLevel = 40;

    float PressSpaceAnimation = 0.0f;
    float PressSpaceAnimationSpeed = 0.005f;
    float PressSpaceAnimationMax = 2.0f;

    int GameState = GAME_IDLE;
    float Dt;
    while(Ez.Running)
    {
        EzPush(&Ez);
        EzPull(&Ez);

        Dt = Ez.Time.Delta;

        switch(GameState)
        {
            case GAME_IDLE:
            {
                // Player animation
                PlayerAnimation += PlayerAnimationSpeed*Dt;
                while(PlayerAnimation > PlayerAnimationMax)
                {
                    PlayerAnimation -= PlayerAnimationMax;
                }

                // Ground movement
                GroundX += WorldV*Dt;
                while(GroundX < (float)(-GRNDTOPW))
                {
                    GroundX += (float)GRNDTOPW;
                }

                // Jump
                if(Ez.Input.Keys[EZ_KEY_SPACE].Pressed)
                {
                    PlayerV = JMPVEL;
                    GameState = GAME_PLAY;
                }

                // Press space animation
                PressSpaceAnimation += PressSpaceAnimationSpeed*Dt;
                while(PressSpaceAnimation > PressSpaceAnimationMax)
                {
                    PressSpaceAnimation -= PressSpaceAnimationMax;
                }

                DrawBackground(NightMode);
                DrawGround((int)GroundX);
                DrawPlayer(PlayerX, PlayerY, PlayerAnimation);
                DrawPressSpace(
                    SCRW/2 - PSPACEW/2,
                    SCRH/2 - PSPACEH/2 + 70,
                    (int)PressSpaceAnimation);
            } break;

            case GAME_PLAY:
            {
                // Pipe creation
                if(PipesCount == 0)
                {
                    GeneratePipe(PipeLevel);
                }
                else
                {
                    pipe *Pipe = GetPipe(PipesCount - 1);
                    if(Pipe)
                    {
                        if(SCRW - (Pipe->X + PIPW) >= PipeToPipe)
                        {
                            GeneratePipe(PipeLevel);
                        }
                    }
                }

                // Player animation
                PlayerAnimation += PlayerAnimationSpeed*Dt;
                while(PlayerAnimation > PlayerAnimationMax)
                {
                    PlayerAnimation -= PlayerAnimationMax;
                }

                // Gravity
                PlayerV += GRVTY*Dt;
                if(PlayerV > MAXVEL) PlayerV = MAXVEL;

                // Flap
                if(Ez.Input.Keys[EZ_KEY_SPACE].Pressed)
                {
                    PlayerV = JMPVEL;
                }

                // Player movement
                PlayerY += PlayerV*Dt;

                // Ground movement
                GroundX += WorldV*Dt;
                while(GroundX < (float)(-GRNDTOPW))
                {
                    GroundX += (float)GRNDTOPW;
                }

                // Pipe movement
                for(int PipeIndex = 0;
                    PipeIndex < PipesCount;
                    ++PipeIndex)
                {
                    pipe *Pipe = GetPipe(PipeIndex);
                    if(!Pipe)
                    {
                        continue;
                    }

                    Pipe->X += WorldV*Dt;

                    if(Pipe->PlayerIn && Pipe->X + (float)PIPW < PlayerX)
                    {
                        ++Score;
                        Pipe->PlayerIn = 0;
                    }

                    if(Pipe->X + PIPW < 0)
                    {
                        RemovePipe(PipeIndex);
                    }
                }

                // Collisions
                if ((int)(PlayerY + PLAYERH) > GRNDY)
                {
                    PlayerY = (float)GRNDY - PLAYERH;
                    PlayerV = 0;
                    GameState = GAME_DEAD;
                }

                if(PlayerY < 0.0)
                {
                    PlayerY = 0;
                    PlayerV = 0;
                    GameState = GAME_DEAD;
                }

                for(int PipeIndex = 0;
                    PipeIndex < PipesCount;
                    ++PipeIndex)
                {
                    pipe *Pipe = GetPipe(PipeIndex);
                    if(!Pipe)
                    {
                        continue;
                    }

                    int TopCollision = BoxCollision(
                        (int)PlayerX, (int)PlayerY,
                        (int)PLAYERW, (int)PLAYERH,
                        (int)Pipe->X, 0,
                        PIPW, (int)Pipe->Y);
                    int BottomCollision = BoxCollision(
                        (int)PlayerX, (int)PlayerY,
                        (int)PLAYERW, (int)PLAYERH,
                        (int)Pipe->X, (int)Pipe->Y + PIPGAP,
                        PIPW, GRNDY - (int)Pipe->Y - PIPGAP);
                    int PlayerInCollision = BoxCollision(
                        (int)PlayerX, (int)PlayerY,
                        (int)PLAYERW, (int)PLAYERH,
                        (int)Pipe->X, 0,
                        PIPW, SCRH);

                    if(TopCollision || BottomCollision)
                    {
                        PlayerV = 0;
                        GameState = GAME_DEAD;
                    }

                    Pipe->PlayerIn = PlayerInCollision;
                }

                DrawBackground(NightMode);
                DrawPipes();
                DrawGround((int)GroundX);
                DrawPlayer(PlayerX, PlayerY, PlayerAnimation);
                DrawScore(Score);
            } break;

            case GAME_DEAD:
            {
                // Gravity
                PlayerV += GRVTY*Dt;
                if(PlayerV > MAXVELDEAD) PlayerV = MAXVELDEAD;

                // Player movement
                PlayerY += PlayerV*Dt;

                // Collision
                if ((int)(PlayerY + PLAYERH) > GRNDY)
                {
                    PlayerY = (float)GRNDY - PLAYERH;
                    GameState = GAME_OVER;
                }

                DrawBackground(NightMode);
                DrawPipes();
                DrawGround((int)GroundX);
                DrawPlayer(PlayerX, PlayerY, 3);
            } break;

            case GAME_OVER:
            {
                // Press space animation
                PressSpaceAnimation += PressSpaceAnimationSpeed*Dt;
                while(PressSpaceAnimation > PressSpaceAnimationMax)
                {
                    PressSpaceAnimation -= PressSpaceAnimationMax;
                }

                if(Ez.Input.Keys[EZ_KEY_SPACE].Pressed)
                {
                    PlayerX = 30;
                    PlayerY = SCRH/2;
                    PlayerV = 0;
                    Score = 0;
                    ResetPipes();
                    GameState = GAME_IDLE;
                }

                DrawBackground(NightMode);
                DrawPipes();
                DrawGround((int)GroundX);
                DrawPlayer(PlayerX, PlayerY, 3);
                DrawGameOver(
                    SCRW/2 - GOVERW/2,
                    SCRH/2 - GOVERH/2);
                DrawPressSpace(
                    SCRW/2 - PSPACEW/2,
                    SCRH/2 - PSPACEH/2 + GOVERH + 20,
                    (int)PressSpaceAnimation);
            } break;
        }
        
    }

    EzClose(&Ez);

    ExitProcess(0);
}
