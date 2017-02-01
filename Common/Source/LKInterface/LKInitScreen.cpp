/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
 */

#include "externs.h"
#include "LKInterface.h"
#include "DoInits.h"
#include "ScreenGeometry.h"
#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#endif
#ifdef KOBO
#include "Kobo/Model.hpp"
#endif
//#ifdef WIN32
//#include <windows.h>
//#endif
#ifdef USE_X11
#include <X11/Xlib.h>
#endif

// The default size of lcd monitor in 1/10 of inches (50= 5")
// Can be overridden by command line -lcdsize=45
unsigned short LcdSize=50;
// The DpiSize, when 0 it is calculated by GetScreenDensity, otherwise forced 
// Can be overridden by command line -dpi=nnn 
unsigned short DpiSize=0; 
#if RESCALE_PIXEL
unsigned short ReferenceDpi=0;
#endif

// InitLKScreen can be called anytime, and should be called upon screen changed from portrait to landscape,
// or windows size is changed for any reason. We dont support dynamic resize of windows, though, because each
// resolution has its own tuned settings. This is thought for real devices, not for PC emulations.
// Attention: after InitLKScreen, also InitLKFonts should be called.

// NOTES
//
// ScreenDScale (double)
//
//   Ratio between shortest size of the screen in pixels, and 240.
//   Range is 0.83 (200px) and up. We support any resolution starting from 200px, example 266x200
//   but some geometries below 200px can also work. 
//
// ScreenScale (int)
//
//   The integer part of ScreenDScale, can be used to rescale by simple integer operation 
//   On a 800x480 ScreenScale is 2, but on a 1920x1080 it is 4 (Dscale 4.5) and thus not good.
//  
// ScreenIntScale (bool)
//
//   If ScreenScale and ScreenDScale correspond, then we can safely use ScreenScale for accurate rescaling 
//   It is used by only one (macro) function, the most important IBLSCALE as
//   define IBLSCALE(x) (   (ScreenIntScale) ? ((x)*ScreenScale) : ((int)((x)*ScreenDScale)))
//   
// IBLSCALE(x)
//  
//   Normally usable for accurate rescaling when accuracy is needed.
//   When accuracy is not needed, ScreenScale is faster. 
//
// NIBLSCALE(x)
//
//   It is an array of IBLSCALE values ready as an array. A lookup table limited to MAXIBLSCALE 
//   Normally 100 but can be enlarged. In TESTBENCH mode the boundary are checked automatically
//   and an error is given on screen to the programmer or user.
//
// ALL OF THE ABOVE is used to rescale properly in respect to the geometry of LK.
// We adopted for historical reasons 240 as base resolution (effectively the lowest, so it is the unity).
// But geometry (the size in pixels and the shape of the screen, 5:3, 16:9 etc.) is one thing, 
// size of screen is another matter. There are cases we want to enlarge things because the screen density
// is way too large in respect of the low resolutions existing in windows devices so far.
// So we now have also a ScreenPixelRatio (int) and relative RescalePixelSize() function for this purpose.
// ScreenPixelRatio is the difference in size of one pixel on current screen in respect of a 
// "standard" look chosen for a certain density. A 480x272 5" is around 110dpi, while a 800x480 5" is 186dpi.
// We assume a "reference dpi" is good for rescaling. 80, 96, 110, anything, and we use it.
//
// FONTS and Screen0Ratio
//
// We tuned geometries (4:3 5:3 16:9 etc) around templates and we rescale in their respect.
// If we need to rescale 1600x960 , ratio 1.66, we use 800x480 template for fonts and we rescale.
// Screen0Ratio is made for this purpose. And it is a vertical ratio, because we rescale fonts
// vertically (that's the way to do it). Screen0Ratio is otherwise pretty useless for the programmer.
// Since we manage each supported geometry separately, we also have a :
//
// ScreenGeometry
// a simple reference to the enumerated gemetries we manage. 
// If you need to rescale a bitmap, the original should be *optimistically* with the same 
// geometry of ScreenGeometry, otherwise you must stretch it. This is an important thing to know.
//
// Why do we have so many parameters for rescaling, so many geometries, so many differences between
// landscape and portrait? Simply because we cannot split bits and pixels and LK wouldnt
// shine on any device, giving out the best looking and best sizes automatically for all the fonts
// we use. In addition, when you deal with low resolution devices, things get even more tough because
// a pixel difference makes a shape good or bad looking, and a line become a z/line.
//
// Things can be simplified a lot by choosing only one scaling approach.
// Dealing with 240 only for dialogs for example, and rescale upon DPI.
// But in such case, the entire code must be revised and adjusted, not to mention the tests
// on smaller devices where "you cannot split pixels".
// One example for all: if you have a small resolution device, which you normally look in flight
// from a distance of 30-35cm, some things cannot be drawn or written (like some units in infopages)
// and some items must be drastically rescaled or enlarged, it depends.
// Since these low resolution devices such as 480x272 are still well in use, changing ScreenScale
// adopting a unified method means loosing the previously set tuned parameters.
// Dropping support for low res devices would open a totally different landscape.
// 1.2.2017 paolo
//


void InitLKScreen() {

#if (WINDOWSPC>0) || defined(__linux__)
    const PixelRect Rect(MainWindow.GetClientRect());
    ScreenSizeX = Rect.GetSize().cx;
    ScreenSizeY = Rect.GetSize().cy;
#else
    ScreenSizeX = GetSystemMetrics(SM_CXSCREEN);
    ScreenSizeY = GetSystemMetrics(SM_CYSCREEN);
#endif

    ScreenSize = ssnone; // This is "ssnone"

    // -----------------------------------------------------
    // These are the embedded known resolutions, fine tuned.
    // -----------------------------------------------------

    if (ScreenSizeX == 240 && ScreenSizeY == 320) ScreenSize = ss240x320; // QVGA      portrait
    if (ScreenSizeX == 234 && ScreenSizeY == 320) ScreenSize = ss240x320; // use the same config of 240x320
    if (ScreenSizeX == 272 && ScreenSizeY == 480) ScreenSize = ss272x480;
    if (ScreenSizeX == 240 && ScreenSizeY == 400) ScreenSize = ss240x320; //           portrait
    if (ScreenSizeX == 480 && ScreenSizeY == 640) ScreenSize = ss480x640; //  VGA
    if (ScreenSizeX == 640 && ScreenSizeY == 480) ScreenSize = ss640x480; //   VGA
    if (ScreenSizeX == 320 && ScreenSizeY == 240) ScreenSize = ss320x240; //  QVGA
    if (ScreenSizeX == 320 && ScreenSizeY == 234) ScreenSize = ss320x240; //  QVGA
    if (ScreenSizeX == 480 && ScreenSizeY == 800) ScreenSize = ss480x800;
    if (ScreenSizeX == 600 && ScreenSizeY == 800) ScreenSize = ss600x800;
    if (ScreenSizeX == 400 && ScreenSizeY == 240) ScreenSize = ss400x240; // landscape
    if (ScreenSizeX == 480 && ScreenSizeY == 272) ScreenSize = ss480x272; // WQVGA     landscape
    if (ScreenSizeX == 480 && ScreenSizeY == 234) ScreenSize = ss480x234; //   iGo
    if (ScreenSizeX == 800 && ScreenSizeY == 480) ScreenSize = ss800x480; //  WVGA
    if (ScreenSizeX == 800 && ScreenSizeY == 600) ScreenSize = ss800x600; //  WVGA


    ScreenGeometry = GetScreenGeometry(ScreenSizeX, ScreenSizeY);

    if (ScreenSize == ssnone) {
        StartupStore(_T(". InitLKScreen: AUTORES %dx%d%s"), ScreenSizeX, ScreenSizeY, NEWLINE);

        ScreenLandscape = (ScreenSizeX >= ScreenSizeY);

        // ScreenGeometry and ScreenLandscape need to be set before call GetScreen0Ratio;
        Screen0Ratio = GetScreen0Ratio();

    } else {
        StartupStore(_T(". InitLKScreen: %dx%d%s"), ScreenSizeX, ScreenSizeY, NEWLINE);

        ScreenLandscape = ((ScreenSize_t) ScreenSize > sslandscape);
        Screen0Ratio = 1;
    }

    
    // -----------------------------
    // Calculate Screen Scale Factor
    // -----------------------------
    
    //  int maxsize = std::max(ScreenSizeX, ScreenSizeY);
    int minsize = std::min(ScreenSizeX, ScreenSizeY);
    
    ScreenDScale = std::max(0.83, minsize / 240.0); // min.200 (240*.83)
    ScreenScale = std::max(1,(int)lround(ScreenDScale));

    ScreenIntScale = (((double) ScreenScale) == ScreenDScale);

    for (int i = 0; i <= MAXIBLSCALE; i++) {
        LKIBLSCALE[i] = IBLSCALE(i);
    }    
    
    // This is used by RescalePixelSize(), defined in Makefile when needed.
    // Some functions using ScreenScale have been changed to use rescaled pixels.
    // We must check that pixelratio is never lower than ScreenScale.
    ScreenDensity = GetScreenDensity();
#ifdef RESCALE_PIXEL
  if (!ReferenceDpi) ReferenceDpi=LK_REFERENCE_DPI;
  ScreenPixelRatio = std::max(1<<10, (ScreenDensity<<10)/ReferenceDpi);
#endif

    
    // -----------------------------
    // Initialize some Global variable 
    // -----------------------------
    
    
    // Initially, this is the default. Eventually retune it for each resolution.
    // We might in the future also set a UseStretch, with or without Hires.
    UseHiresBitmap = (ScreenScale > 1);
    
    //
    // The thinnest line somehow visible on screen from 35cm distance.
    //
    ScreenThinSize = RescalePixelSize(1);

    GestureSize = RescalePixelSize(50);

    // Override defaults for custom settings
    switch ((ScreenSize_t) ScreenSize) {
        case ss600x800:
            LKVarioSize = 45;
            break;
        case ss240x320:
            LKVarioSize = 13;
            break;
        case ss272x480:
            LKVarioSize = 30;
            break;
        default:
            if (ScreenLandscape) {
                LKVarioSize = ScreenSizeX / 16;
            } else {
                LKVarioSize = ScreenSizeX / 11;
            }
            break;
    }

    AircraftMenuSize = NIBLSCALE(28) + 14;
    CompassMenuSize = AircraftMenuSize + NIBLSCALE(17);

#ifdef TESTBENCH
    StartupStore(_T("..... ScreenSizeX      = %d" NEWLINE), ScreenSizeX);
    StartupStore(_T("..... ScreenSizeY      = %d" NEWLINE), ScreenSizeY);
    StartupStore(_T("..... ScreenDensity    = %d" NEWLINE), ScreenDensity);
    StartupStore(_T("..... ScreenGeometry   = %d" NEWLINE), ScreenGeometry);
    StartupStore(_T("..... ScreenSize(enum) = %d" NEWLINE), ScreenSize);
    StartupStore(_T("..... ScreenDScale     = %.3f" NEWLINE), ScreenDScale);
    StartupStore(_T("..... ScreenScale      = %d" NEWLINE), ScreenScale);
    StartupStore(_T("..... ScreenIntScale   = %s" NEWLINE), ScreenIntScale ? _T("true") : _T("false"));
    StartupStore(_T("..... Screen0Ratio     = %f" NEWLINE), Screen0Ratio);

#ifdef RESCALE_PIXEL  
    StartupStore(_T("..... ReferenceDpi     = %d" NEWLINE), ReferenceDpi);
    StartupStore(_T("..... ScreenPixelRatio = %d.%d" NEWLINE), ScreenPixelRatio >> 10, ScreenPixelRatio & 0x3FF);
#endif

    StartupStore(_T("..... ThinSize         = %d" NEWLINE), ScreenThinSize);
    StartupStore(_T("..... NIBLSCALE(1)     = %d" NEWLINE), NIBLSCALE(1));
    StartupStore(_T("..... NIBLSCALE(2)     = %d" NEWLINE), NIBLSCALE(2));

    StartupStore(_T("..... GestureSize      = %d" NEWLINE), GestureSize);
    StartupStore(_T("..... LKVarioSize      = %d" NEWLINE), LKVarioSize);
    StartupStore(_T("..... AircraftMenuSize = %d" NEWLINE), AircraftMenuSize);
    StartupStore(_T("..... CompassMenuSize  = %d" NEWLINE), CompassMenuSize);
#endif  
} // End of LKInitScreen


//
// Inside LKFonts we support special resolutions at best possible tuned settings.
// These resolutions are used as a base for resizing, considering their geometry ratio.
// Most modern screens have a 1,777 ratio, so in any case there is no need to think
// about dozens of geometries and we can take it easy with a simple approach here.
//

unsigned short GetScreenGeometry(unsigned int x, unsigned int y) {

#if TESTBENCH
    LKASSERT(x < 5000 && y < 5000);
#endif
    LKASSERT(x > 0 && y > 0);

    double ratio = x >= y ? x / (double) y : y / (double) x;

    //
    // Table of internally tuned ratios in LK8000
    //
    // Ratio   Aspect     Examples
    // -----   ------     --------
    // 1,333    4:3        320x240 640x480 800x600
    // 1,666    5:3        800x480
    // 1,777    16:9       480x272 960x540 1280x720 1920x1080
    // 2,05     2:1        480x234
    //
    // Aspect change thresholds:
    //
    // 1,000
    //   1,166
    // 1,333
    //   1,500
    // 1,666
    //   1,721
    // 1,777
    //   1,888
    // 2,000
    //

    // Here we decide which is the closest ratio
    while (1) {
        if (ratio < 1.166) return SCREEN_GEOMETRY_21; // yet unsupported SCREEN_GEOMETRY_SQUARED!
        if (ratio < 1.500) return SCREEN_GEOMETRY_43; // 1,33
        if (ratio < 1.721) return SCREEN_GEOMETRY_53; // 1,66
        if (ratio < 1.888) return SCREEN_GEOMETRY_169; // 1,77
        if (ratio < 2.112) return SCREEN_GEOMETRY_21;
        ratio /= 2;
    }
}

//
// We calculate the correct scaling factor based on vertical extension.
// That is because all fonts are rescaled by their height by the function
// ApplyFontSize() using formula:  new_height=(old_height * Screen0Ratio)
// If we change this function, let's update also ScreenGeometry.h for memo.
//

double GetScreen0Ratio(void) {
    double ratio = 0;
    if (ScreenLandscape) {
        switch (ScreenGeometry) {
            case SCREEN_GEOMETRY_43:
                ratio = (double) ScreenSizeY / 480.0;
                break;
            case SCREEN_GEOMETRY_53:
                ratio = (double) ScreenSizeY / 480.0;
                break;
            case SCREEN_GEOMETRY_169:
                ratio = (double) ScreenSizeY / 272.0;
                break;
            case SCREEN_GEOMETRY_21:
                ratio = (double) ScreenSizeY / 234.0;
                break;
            default:
                ratio = (double) ScreenSizeY / 272.0;
                break;
        }
    } else {
        switch (ScreenGeometry) {
            case SCREEN_GEOMETRY_43:
                ratio = (double) ScreenSizeY / 640.0;
                break;
            case SCREEN_GEOMETRY_53:
                ratio = (double) ScreenSizeY / 800.0;
                break;
            case SCREEN_GEOMETRY_169:
                ratio = (double) ScreenSizeY / 480.0;
                break;
            case SCREEN_GEOMETRY_21:
                ratio = (double) ScreenSizeY / 480.0;
                break;
            default:
                ratio = (double) ScreenSizeY / 480.0;
                break;
        }
    }
    return ratio;
}

//
// Screen DPI estimation for some platform.
//
int GetScreenDensity(void) {

    if (DpiSize) return((int)DpiSize);

#ifdef KOBO
    switch (DetectKoboModel()) {
        case KoboModel::GLOHD:
            return 300;
        case KoboModel::TOUCH2:
            return 167;
        default:
            return 200; // Kobo Mini 200 dpi; Kobo Glo 212 dpi (according to Wikipedia)
    }
#endif

#ifdef  ANDROID
    return native_view->GetXDPI();
#endif

    //#ifdef WIN32    // for the moment we mantain default value for WIN32 as LOGPIXELSX always return 96 ?
    //	HDC dc = GetDC(NULL);
    //	return GetDeviceCaps(dc, LOGPIXELSX);
    //#endif

#if 0 // def USE_X11
    _XDisplay* disp = XOpenDisplay(NULL);
    double xres = ((((double) DisplayWidth(disp, 0)) * 25.4) / ((double) DisplayWidthMM(disp, 0)));
    XCloseDisplay(disp);
    return (short) xres;
#endif

    // if we are not able to get correct value just return default estimation
    return (sqrt(ScreenSizeX * ScreenSizeX + ScreenSizeY * ScreenSizeY) *10) / LcdSize; 
}



