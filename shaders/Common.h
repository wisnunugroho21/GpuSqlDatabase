// This file is part of the AMD & HSC Work Graph Playground.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc. and Coburg University of Applied Sciences and Arts.
// All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#define PI 3.14159265359

// This file contains helper functions and common resources for the
// Work Graph Playground Application.

// Pixel coordinates are 2D integer window coordinates (i.e., viewport coordinates).
// For (pixelX, pixelY) the upper-left corner is uint2(0, 0) the lower-right corner is
// (RenderSize.x-1 , RenderSize.y-1).

// 3D color values are always float3 RGB color values spanning the unit cube [0..1]^3, where
// float3(0, 0, 0) is black and float3(1, 1, 1) is white.

// RenderTarget is the output for all tutorials.
// You write a color to a pixel using integer window coordinates.
// (pixelX, pixelY) with
//
//   float3 color = float3(1, 1, 0);
//   RenderTarget[uint2(pixelX, pixelY)] = float4(color, 1);
//
// For output colors we encode them using 4D vectors whose first three components encode
// the red, green, and blue unit-cube color channels.
// The fourth component contains the alpha channel, which the Work Graph Playground Application
// does not use.
//
// Reading from pixels is possible but not recommended.
RWTexture2D<float4> RenderTarget : register(u0);

// 400kiB (= 100 * 1024 uints) input buffer that is cleared to zero every frame.
// You can use this buffer to read and write your user data.
RWByteAddressBuffer InputBuffer : register(u1);

// 400MiB (= 100 * 1024 * 1024 uints) output buffer that is cleared to zero when starting tutorials.
// You can use this buffer to read and write your user data.
RWByteAddressBuffer OutputBuffer : register(u2);

// Constants provided by Work Graph Playground Application.
cbuffer Constants : register(b0)
{
    // Size of the RenderTarget in pixels.
    uint2  RenderSize;
    // Mouse position in pixels.
    float2 MousePosition;
    // Encoded input state. See input namespace below for details.
    uint   InputState;
    // Time since the application start in seconds.
    float  Time;
};

/* Helper struct for printing text to the screen.
 You can use this to print text or number to the RenderTarget texture.

     Example usage:


     Cursor cursor = Cursor(                        // Create a cursor
                            int2(5, 5),             // at (5, 5)
                            2,                      // with size 2 and
                            float3(1, 1, 0));       // a yellow color.


     Println(cursor, "Hello World!");               // Print a string and advance to next line.

     Print(cursor, "The time is: ");                // Print a message with numbers
     PrintFloat(cursor, Time);                      // Print a number.

     cursor.Newline();                              // Advance to next line

     Print(cursor, "The render target size is ");   // Print another message,
     PrintUint(cursor, RenderSize.x);               // a number,
     Print(cursor, "x");                            // a character
     PrintUint(cursor, RenderSize.y);               // another number
*/
struct Cursor {
    // The position of the cursor in window-coordinates. See RenderTarget documentation for more details.
    int2   position;
    // Internal member for handling new lines.
    int    nextX;
    // The font size in the pixels.
    int    size;
    // The RGB font color.
    float3 color;

    // Creates a cursor with the provided window coordinate "position" in, "size" in pixels, and the provided color.
    static Cursor ctor(in const int2   position = int2(0, 0),
                       in const int    size     = 2,
                       in const float3 color    = float3(0, 0, 0))
    {
        Cursor cursor;

        cursor.position = position;
        cursor.nextX    = position.x;
        cursor.size     = size;
        cursor.color    = color;

        return cursor;
    }

    // Get the top-left position of the cursor.
    int2 GetTopLeft()
    {
        return position;
    }

    // Get the bottom-right position of the cursor.
    int2 GetBottomRight()
    {
        return position + size * 8;
    }

    // Sets a "newSize" of the cursor.
    void SetSize(in const int newSize)
    {
        size = newSize;
    }

    // Sets a "newColor" of the cursor.
    void SetColor(in const float3 newColor)
    {
        color = newColor;
    }

    // Advances the cursor to a new line.
    // The x position is the same as the starting x position of the previous line.
    void Newline()
    {
        position.x = nextX;
        position.y += 8 * (size + 1);
        nextX = position.x;
    }

    // Advances the cursors "characters" to the right.
    void Right(int characters)
    {
        position.x += characters * size * 8;
    }

    // Advances the cursors "characters" to the left.
    void Left(int characters)
    {
        Right(-characters);
    }

    // Advances the cursors "characters" to the right.
    void Advance(int characters = 1)
    {
        Right(characters);
    }

    // Advances the cursors "characters" down.
    void Down(int lines)
    {
        position.y += lines * size * 8;
    }

    // Advances the cursors "characters" up.
    void Up(int lines)
    {
        Down(-lines);
    }
};

// Constructor workaround for cursor struct.
// Use this define to conveniently create your Cursor in C++-like constructor fashion.
#define Cursor(...) Cursor::ctor(__VA_ARGS__)

namespace printutil {

    // Computes the length in characters of the provided string "str".
    template <typename T, int N>
    int StrLen(in const T str[N])
    {
        return N - 1;
    }

    // Maps a character to an integer.
    // This is required because HLSL cannot do char-to-integer casts.
    template <typename T>
    inline int CharToInt(in const T c)
    {
        if (c == '\n')
            return 10;
        if (c == '!')
            return 33;
        if (c == '"')
            return 34;
        if (c == '#')
            return 35;
        if (c == '$')
            return 36;
        if (c == '%')
            return 37;
        if (c == '&')
            return 38;
        if (c == '\'')
            return 39;
        if (c == '(')
            return 40;
        if (c == ')')
            return 41;
        if (c == '*')
            return 42;
        if (c == '+')
            return 43;
        if (c == ',')
            return 44;
        if (c == '-')
            return 45;
        if (c == '.')
            return 46;
        if (c == '/')
            return 47;
        if (c == '0')
            return 48;
        if (c == '1')
            return 49;
        if (c == '2')
            return 50;
        if (c == '3')
            return 51;
        if (c == '4')
            return 52;
        if (c == '5')
            return 53;
        if (c == '6')
            return 54;
        if (c == '7')
            return 55;
        if (c == '8')
            return 56;
        if (c == '9')
            return 57;
        if (c == ':')
            return 58;
        if (c == ';')
            return 59;
        if (c == '<')
            return 60;
        if (c == '=')
            return 61;
        if (c == '>')
            return 62;
        if (c == '?')
            return 63;
        if (c == '@')
            return 64;
        if (c == 'A')
            return 65;
        if (c == 'B')
            return 66;
        if (c == 'C')
            return 67;
        if (c == 'D')
            return 68;
        if (c == 'E')
            return 69;
        if (c == 'F')
            return 70;
        if (c == 'G')
            return 71;
        if (c == 'H')
            return 72;
        if (c == 'I')
            return 73;
        if (c == 'J')
            return 74;
        if (c == 'K')
            return 75;
        if (c == 'L')
            return 76;
        if (c == 'M')
            return 77;
        if (c == 'N')
            return 78;
        if (c == 'O')
            return 79;
        if (c == 'P')
            return 80;
        if (c == 'Q')
            return 81;
        if (c == 'R')
            return 82;
        if (c == 'S')
            return 83;
        if (c == 'T')
            return 84;
        if (c == 'U')
            return 85;
        if (c == 'V')
            return 86;
        if (c == 'W')
            return 87;
        if (c == 'X')
            return 88;
        if (c == 'Y')
            return 89;
        if (c == 'Z')
            return 90;
        if (c == '[')
            return 91;
        if (c == '\\')
            return 92;
        if (c == ']')
            return 93;
        if (c == '^')
            return 94;
        if (c == '_')
            return 95;
        if (c == '`')
            return 96;
        if (c == 'a')
            return 97;
        if (c == 'b')
            return 98;
        if (c == 'c')
            return 99;
        if (c == 'd')
            return 100;
        if (c == 'e')
            return 101;
        if (c == 'f')
            return 102;
        if (c == 'g')
            return 103;
        if (c == 'h')
            return 104;
        if (c == 'i')
            return 105;
        if (c == 'j')
            return 106;
        if (c == 'k')
            return 107;
        if (c == 'l')
            return 108;
        if (c == 'm')
            return 109;
        if (c == 'n')
            return 110;
        if (c == 'o')
            return 111;
        if (c == 'p')
            return 112;
        if (c == 'q')
            return 113;
        if (c == 'r')
            return 114;
        if (c == 's')
            return 115;
        if (c == 't')
            return 116;
        if (c == 'u')
            return 117;
        if (c == 'v')
            return 118;
        if (c == 'w')
            return 119;
        if (c == 'x')
            return 120;
        if (c == 'y')
            return 121;
        if (c == 'z')
            return 122;
        if (c == '{')
            return 123;
        if (c == '|')
            return 124;
        if (c == '}')
            return 125;
        if (c == '~')
            return 126;
        return 0;
    }

    // Font atlas buffer. Contents are defined and provided to D3D12 in Application.cpp.
    StructuredBuffer<uint64_t> Font : register(t0);

    // Prints a character "c" at the "cursor" location and updates the cursors location for the next character.
    void PrintChar(inout Cursor cursor, in const int c)
    {
        if (c == 10) {
            cursor.Newline();
            return;
        }

        const uint64_t bitmap = Font[c];

        for (int i = 0; i < 64; ++i) {
            const int      row   = i / 8;
            const int      col   = i % 8;
            const uint64_t shift = 8 * (7 - row) + col;
            const bool     set   = uint(bitmap >> shift) & 1;

            if (set) {
                int xfrom = clamp(cursor.position.x + cursor.size * col, 0, RenderSize.x);
                int xto   = clamp(cursor.position.x + cursor.size * (col + 1), 0, RenderSize.x);
                int yfrom = clamp(cursor.position.y + cursor.size * row, 0, RenderSize.y);
                int yto   = clamp(cursor.position.y + cursor.size * (row + 1), 0, RenderSize.y);

                for (int y = yfrom; y < yto; ++y) {
                    for (int x = xfrom; x < xto; ++x) {
                        RenderTarget[uint2(x, y)] = float4(cursor.color, 1);
                    }
                }
            }
        }
        cursor.Advance();
    }

}  // namespace printutil

// Prints a string "STR" at right of the location "CURSOR".
#define Print(CURSOR, STR)                                    \
    {                                                         \
        static const uint len = printutil::StrLen(STR);            \
        for (int i = 0; i < len; ++i) {                       \
            printutil::PrintChar(CURSOR, printutil::CharToInt(STR[i])); \
        }                                                     \
    }

// Prints a string "STR" right of the location "CURSOR", and moves the CURSOR to a newline.
#define Println(CURSOR, STR) \
    do {                     \
        Print(STR, CURSOR);  \
        CURSOR.Newline();    \
    } while (0);

// Prints a string "STR" horizontally centered around the "CURSOR".
#define PrintCentered(CURSOR, STR)          \
    do {                                    \
        CURSOR.Left(printutil::StrLen(STR) / 2); \
        Print(CURSOR, STR);                 \
    } while (0);

// Prints an unsigned integer "n" at the provided "cursor" location.
// Uses at least "minDigits" digits and adds leading zeros if neccessary.
void PrintUint(inout Cursor cursor, in uint n, in const int minDigits = 1)
{
    // Compute number of digits
    const uint digits = max(log10(max(1, n)) + 1, minDigits);

    cursor.Right(digits + 1);
    for (int digit = 0; digit < digits; ++digit) {
        cursor.Left(2);
        printutil::PrintChar(cursor, 48 + (n % 10));
        n /= 10;
    }
    cursor.Right(digits - 1);
}

// Prints a signed integer "n" at the provided "cursor" location.
// Uses at least "minDigits" digits and adds leading zeros if neccessary.
void PrintInt(inout Cursor cursor, in const int n, in const int minDigits = 1)
{
    // Handle negative numbers
    if (n < 0) {
        Print(cursor, "-");
    }

    // Print decimals as uint
    PrintUint(cursor, abs(n), minDigits);
}

// Prints a floating point number "v" at the provided "cursor" location.
// Uses "decimialDigits" after the decimial separator (which is a point here).
void PrintFloat(inout Cursor cursor, in float v, in int decimalDigits = 3)
{
    // Handle NaN
    if (isnan(v)) {
        Print(cursor, "NaN");
        return;
    }

    // Handle negative numbers
    if (v < 0) {
        Print(cursor, "-");
        // Handle the rest as positive number
        v = abs(v);
    }

    // Handle +INF & -INF
    if (!isfinite(v)) {
        Print(cursor, "INF");
        return;
    }

    // Print integer part of float
    PrintUint(cursor, uint(v));
    // Print decimal point
    Print(cursor, ".");

    // compute n = pow(10, ddecimalDigits) as uint
    uint n = 10;
    for (int i = 0; i < decimalDigits - 1; i++) {
        n *= 10;
    }

    // Print remainder
    PrintUint(cursor, uint((v * n)) % n, decimalDigits);
}

// Returns true if a point "p" is inside or on the boundary of a capsule
// that goes from "a" to "b" and has radius "r".
bool InsideCapsule(float2 p, float2 a, float2 b, float r)
{
    float2 pa = p - a, ba = b - a;
    float  h = saturate(dot(pa, ba) / dot(ba, ba));
    return length(pa - ba * h) <= r;
}

// Draws a line between two window coordinates points ("from" and "to") using "thickness" provided in pixels.
// "Thickness is the number of pixels that you find left and as well right of the line.
// Use "color" to specify the RGB values of the rendered line pixels.
void DrawLine(in const float2 from,
              in const float2 to,
              in const float  thickness = 1,
              in const float3 color     = float3(0, 0, 0))
{
    int2 bbmin = min(floor(from), floor(to));
    int2 bbmax = max(ceil(from), ceil(to));

    // extend bbox by thickness & clip to screen
    bbmin = max(bbmin - thickness, float2(0, 0));
    bbmax = min(bbmax + thickness, int2(RenderSize) - 1);

    for (int y = bbmin.y; y <= bbmax.y; ++y) {
        for (int x = bbmin.x; x <= bbmax.x; ++x) {
            if (InsideCapsule(float2(x, y), from, to, thickness)) {
                RenderTarget[uint2(x, y)] = float4(color, 1);
            }
        }
    }
}

// Fills circle centered at "position" and "radius" given in pixels.
// Use "color" to specify the RGB values of the fill color.
void FillCircle(in const float2 position,
                in const float  radius,
                in const float3 color = float3(0, 0, 0))
{
    DrawLine(position, position, radius, color);
}

// Draws the outline of a rectangle spanning from "topLeft" to "bottomRight" in window coordinates with a "thickness" in
// pixels. Thickness extends symmetrically to inside and outside of outline. Use "color" to specify the RGB values of
// the rendered rectangle outline pixels.
void DrawRect(in const float2 topLeft,
              in const float2 bottomRight,
              in const float  thickness = 1,
              in const float3 color     = float3(0, 0, 0))
{
    const float2 size = bottomRight - topLeft;

    DrawLine(topLeft,     topLeft     + float2(size.x, 0     ), thickness, color);
    DrawLine(topLeft,     topLeft     + float2(0     , size.y), thickness, color);
    DrawLine(bottomRight, bottomRight - float2(size.x, 0     ), thickness, color);
    DrawLine(bottomRight, bottomRight - float2(0     , size.y), thickness, color);
}

// Fills a rectangle spanning from "topLeft" to "bottomRight" in window-coordinates with a solid fill "color".
// Use "color" to specify the RGB values of the rendered rectangle pixels.
void FillRect(in const float2 topLeft, in const float2 bottomRight, in const float3 color = float3(0, 0, 0))
{
    int2 bbmin = floor(min(topLeft, bottomRight));
    int2 bbmax = ceil(max(topLeft, bottomRight));

    // clip bbox to screen
    bbmin = max(bbmin, 0);
    bbmax = min(bbmax, int2(RenderSize) - 1);

    for (int y = bbmin.y; y <= bbmax.y; ++y) {
        for (int x = bbmin.x; x <= bbmax.x; ++x) {
            RenderTarget[uint2(x, y)] = float4(color, 1);
        }
    }
}

// Converts unsigned integer "value" to RGB color.
// For convenience, the color is returned as float4 with a = 1.f, such that
// it can be directly written to RenderTarget[...]
float4 UintToColor(in uint value)
{
    const float red   = (((value + 23) % 11) + 1) / 11.f;
    const float green = (((value + 16) % 12) + 1) / 12.f;
    const float blue  = (((value + 7) % 10) + 1) / 10.f;

    return float4(red, green, blue, 1.0);
}

// Divides divident by divisor and rounds the result to the next larger integer.
int DivideAndRoundUp(in int dividend, in int divisor)
{
    return (dividend + divisor - 1) / divisor;
}

int2 DivideAndRoundUp(in int2 dividend, in int2 divisor)
{
    return (dividend + divisor - 1) / divisor;
}

// Random & Noise functions.
namespace random {
    // Computes a random gradient at 2D "position".
    float2 PerlinNoiseDir2D(in int2 position)
    {
        const int2 pos = position % 289;

        float f = 0;
        f       = (34 * pos.x + 1);
        f       = f * pos.x % 289 + pos.y;
        f       = (34 * f + 1) * f % 289;
        f       = frac(f / 43) * 2 - 1;

        float x = f - round(f);
        float y = abs(f) - 0.5;

        return normalize(float2(x, y));
    }

    // Returns a Perlin-noise value at a given 2D "position".
    float PerlinNoise2D(in float2 position)
    {
        const int2   gridPositon = floor(position);
        const float2 gridOffset  = frac(position);

        const float d00 = dot(PerlinNoiseDir2D(gridPositon + int2(0, 0)), gridOffset - float2(0, 0));
        const float d01 = dot(PerlinNoiseDir2D(gridPositon + int2(0, 1)), gridOffset - float2(0, 1));
        const float d10 = dot(PerlinNoiseDir2D(gridPositon + int2(1, 0)), gridOffset - float2(1, 0));
        const float d11 = dot(PerlinNoiseDir2D(gridPositon + int2(1, 1)), gridOffset - float2(1, 1));

        const float2 interpolationWeights =
            gridOffset * gridOffset * gridOffset * (gridOffset * (gridOffset * 6 - 15) + 10);

        const float d0 = lerp(d00, d01, interpolationWeights.y);
        const float d1 = lerp(d10, d11, interpolationWeights.y);

        return lerp(d0, d1, interpolationWeights.x);
    }

    // Returns a random hash value based on a "seed".
    uint Hash(uint seed)
    {
        seed = (seed ^ 61u) ^ (seed >> 16u);
        seed *= 9u;
        seed = seed ^ (seed >> 4u);
        seed *= 0x27d4eb2du;
        seed = seed ^ (seed >> 15u);
        return seed;
    }

    // Combines two values "a" and "b" to a common seed value.
    uint CombineSeed(uint a, uint b)
    {
        return a ^ Hash(b) + 0x9e3779b9 + (a << 6) + (a >> 2);
    }

    // Combines three values "a", "b", and "c" to a common seed value.
    uint CombineSeed(uint a, uint b, uint c)
    {
        return CombineSeed(CombineSeed(a, b), c);
    }

    // Combines four values "a", "b", "c", and "d" to a common seed value.
    uint CombineSeed(uint a, uint b, uint c, uint d)
    {
        return CombineSeed(CombineSeed(a, b), c, d);
    }

    // Computes the has value of floating-point number "seed".
    uint Hash(in float seed)
    {
        return Hash(asuint(seed));
    }

    // Computes the has value of 3D floating-point vector "vec".
    uint Hash(in float3 vec)
    {
        return CombineSeed(Hash(vec.x), Hash(vec.y), Hash(vec.z));
    }

    // Computes the has value of 4D floating-point vector "vec".
    uint Hash(in float4 vec)
    {
        return CombineSeed(Hash(vec.x), Hash(vec.y), Hash(vec.z), Hash(vec.w));
    }

    // Returns random floating-point number in [0; 1] from an integer "seed".
    float Random(uint seed)
    {
        return Hash(seed) / float(~0u);
    }

    // Returns random floating-point number in [0; 1] from a 2D integer seed "a" and "b".
    float Random(uint a, uint b)
    {
        return Random(CombineSeed(a, b));
    }

    // Returns random floating-point number in [0; 1] from a 3D integer seed "a", "b", and "c".
    float Random(uint a, uint b, uint c)
    {
        return Random(CombineSeed(a, b), c);
    }

    // Returns random floating-point number in [0; 1] from a 4D integer seed "a", "b", "c", and "d".
    float Random(uint a, uint b, uint c, uint d)
    {
        return Random(CombineSeed(a, b), c, d);
    }

    // Returns random floating-point number in [0; 1) from a 5D integer seed "a", "b", "c", "d", "e".
    float Random(uint a, uint b, uint c, uint d, uint e)
    {
        return Random(CombineSeed(a, b), c, d, e);
    }
}  // namespace random

// Utils for decoding input state. These must be in sync with the input state encoding in Application.cpp
namespace input {

    // Returns true, if the left mouse button is down.
    bool IsMouseLeftDown()
    {
        return InputState & (1 << 0);
    }

    // Returns true, if the middle mouse button is down.
    bool IsMouseMiddleDown()
    {
        return InputState & (1 << 1);
    }

    // Returns true, if the right mouse button is down.
    bool IsMouseRightDown()
    {
        return InputState & (1 << 2);
    }

    // Returns true, if the space-key is down.
    bool IsKeySpaceDown()
    {
        return InputState & (1 << 3);
    }

    // Returns true, if the up-arrow key is down.
    bool IsKeyUpArrowDown()
    {
        return InputState & (1 << 4);
    }

    // Returns true, if the left-arrow key is down.
    bool IsKeyLeftArrowDown()
    {
        return InputState & (1 << 5);
    }

    // Returns true, if the down-arrow key is down.
    bool IsKeyDownArrowDown()
    {
        return InputState & (1 << 6);
    }

    // Returns true, if the right-arrow key is down.
    bool IsKeyRightArrowDown()
    {
        return InputState & (1 << 7);
    }

    // Returns true, if the W-key is down.
    bool IsKeyWDown()
    {
        return InputState & (1 << 8);
    }

    // Returns true, if the A-key is down.
    bool IsKeyADown()
    {
        return InputState & (1 << 9);
    }

    // Returns true, if the S-key is down.
    bool IsKeySDown()
    {
        return InputState & (1 << 10);
    }

    // Returns true, if the D-key is down.
    bool IsKeyDDown()
    {
        return InputState & (1 << 11);
    }

}  // namespace input