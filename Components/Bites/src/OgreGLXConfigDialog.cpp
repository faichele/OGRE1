/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "OgreException.h"
#include "OgreImage.h"
#include "OgreLogManager.h"
#include "OgreRoot.h"
#include "OgreConfigPaths.h"
#include "OgreFileSystemLayer.h"

#include <cstdlib>
#include <iostream>

#include <string>
#include "OgreConfigDialogImp.h"

#define XTSTRINGDEFINES

#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Box.h>
#include <X11/Shell.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>

#include <list>

using namespace Ogre;

namespace OgreBites {
    using Ogre::String;

/**
 * Single X window with image backdrop, making it possible to configure
 * OGRE in a graphical way.
 * XaW uses a not-very-smart widget positioning system, so I override it to use
 * fixed positions. This works great, but it means you need to define the various
 * positions manually.
 * Furthermore, it has no OptionMenu by default, so I simulate this with dropdown
 * buttons.
 */
class GLXConfigurator {
    /* GUI constants */
    static const int wWidth = 500;      // Width of window
    static const int wHeight = 400;     // Height of window
    static const int col1x = 20;        // Starting x of column 1 (labels)
    static const int col2x = 230;       // Starting x of column 2 (options)
    static const int col1w = 200;       // Width of column 1 (labels)
    static const int col2w = 250;       // Width of column 2 (options)
    static const int ystart = 105;      // Starting y of option table rows
    static const int rowh = 20;     // Height of one row in the option table

public:
    GLXConfigurator();
    virtual ~GLXConfigurator();

    bool CreateWindow();
    void Main();
    /**
     * Exit from main loop.
     */
    void Exit();
protected:
    Display *mDisplay;
    Window mWindow;
    Pixmap mBackDrop;

    int mWidth, mHeight;
    // Xt
    XtAppContext appContext;
    Widget toplevel;

    /**
     * Create backdrop image, and return it as a Pixmap.
     */
    virtual Pixmap CreateBackdrop(Window rootWindow, int depth);
    /**
     * Called after window initialisation.
     */
    virtual bool Init();
    /**
     * Called initially, and on expose.
     */
    virtual void Draw();
public:
    /* Local */
    bool accept;
    /* Class that binds a callback to a RenderSystem */
    class RendererCallbackData {
    public:
        RendererCallbackData(GLXConfigurator *parent_, RenderSystem *renderer_, Widget optionmenu_):
            parent(parent_),
            renderer(renderer_),
            optionmenu(optionmenu_) {
        }
        GLXConfigurator *parent;
        RenderSystem *renderer;
        Widget optionmenu;
    };
    std::list<RendererCallbackData> mRendererCallbackData;

    RenderSystem *mRenderer;
    Widget box;                 // Box'o control widgets
    std::list<Widget> mRenderOptionWidgets; // List of RenderSystem specific
                        // widgets for visibility management (cleared when another rendersystem is selected)
    /* Class that binds a callback to a certain configuration option/value */
    class ConfigCallbackData {
    public:
        ConfigCallbackData(GLXConfigurator *parent_, const String &optionName_, const String &valueName_, Widget optionmenu_):
            parent(parent_),
            optionName(optionName_),
            valueName(valueName_),
            optionmenu(optionmenu_) {
        }
        GLXConfigurator *parent;
        String optionName, valueName;
        Widget optionmenu;
    };
    std::list<ConfigCallbackData> mConfigCallbackData;

    void SetRenderSystem(RenderSystem *sys) {
        mRenderer = sys;
    }
private:
    /* Callbacks that terminate modal dialog loop */
    static void acceptHandler(Widget w, GLXConfigurator *obj, XtPointer callData) {
        // Check if a renderer was selected, if not, don't accept
        if(!obj->mRenderer)
            return;
        obj->accept = true;
        obj->Exit();
    }
    static void cancelHandler(Widget w, GLXConfigurator *obj, XtPointer callData) {
        obj->Exit();
    }
    /* Callbacks that set a setting */
    static void renderSystemHandler(Widget w, RendererCallbackData *cdata, XtPointer callData) {
        // Set selected renderer its name
            XtVaSetValues(cdata->optionmenu, XtNlabel, cdata->renderer->getName().c_str(), 0, NULL);
        // Notify Configurator (and Ogre)
        cdata->parent->SetRenderer(cdata->renderer);
    }
    static void configOptionHandler(Widget w, ConfigCallbackData *cdata, XtPointer callData) {
        // Set selected renderer its name
            XtVaSetValues(cdata->optionmenu, XtNlabel, cdata->valueName.c_str(), 0, NULL);
        // Notify Configurator (and Ogre)
        cdata->parent->SetConfigOption(cdata->optionName, cdata->valueName);
    }

    /* Functions reacting to GUI */
    void SetRenderer(RenderSystem *);
    void SetConfigOption(const String &optionName, const String &valueName);
};

GLXConfigurator::GLXConfigurator():
    mDisplay(0), mWindow(0), mBackDrop(0),
    mWidth(wWidth), mHeight(wHeight),
    appContext(0), toplevel(0),

    accept(false),
    mRenderer(0) {
}
GLXConfigurator::~GLXConfigurator() {
    if(mBackDrop)
        XFreePixmap(mDisplay, mBackDrop);
    if(toplevel) {
        XtUnrealizeWidget(toplevel);
        XtDestroyWidget(toplevel);
    }
    if(mDisplay) {
        XCloseDisplay(mDisplay);
    }
}

bool GLXConfigurator::CreateWindow() {


    const char *bla[] = {"Rendering Settings", "-bg", "honeydew3", "-fg", "black","-bd","darkseagreen4"};
    int argc = sizeof(bla)/sizeof(*bla);

    toplevel = XtVaOpenApplication(&appContext, "OGRE", NULL, 0, &argc, const_cast<char**>(bla), NULL,sessionShellWidgetClass,
        XtNwidth, mWidth,
        XtNheight, mHeight,
        XtNminWidth, mWidth,
        XtNmaxWidth, mWidth,
        XtNminHeight, mHeight,
        XtNmaxHeight, mHeight,
        XtNallowShellResize, False,
        XtNborderWidth, 0,
        XtNoverrideRedirect, False,
        NULL, NULL);

    /* Find out display and screen used */
    mDisplay = XtDisplay(toplevel);
    int screen = DefaultScreen(mDisplay);
    Window rootWindow = RootWindow(mDisplay,screen);

    /* Move to center of display */
    int w = DisplayWidth(mDisplay, screen);
    int h = DisplayHeight(mDisplay, screen);
    XtVaSetValues(toplevel,
            XtNx, w/2-mWidth/2,
            XtNy, h/2-mHeight/2, 0, NULL);

    /* Backdrop stuff */
    mBackDrop = CreateBackdrop(rootWindow, DefaultDepth(mDisplay,screen));

    /* Create toplevel */
    box = XtVaCreateManagedWidget("box",formWidgetClass,toplevel,
        XtNbackgroundPixmap, mBackDrop,
        0,NULL);

    /* Create renderer selection */
    int cury = ystart + 0*rowh;

    XtVaCreateManagedWidget("topLabel", labelWidgetClass, box, XtNlabel, "Select Renderer", XtNborderWidth, 0,
        XtNwidth, col1w,    // Fixed width
        XtNheight, 18,
        XtNleft, XawChainLeft,
        XtNtop, XawChainTop,
        XtNright, XawChainLeft,
        XtNbottom, XawChainTop,
        XtNhorizDistance, col1x,
        XtNvertDistance, cury,
        XtNjustify, XtJustifyLeft,
        NULL);

    const char *curRenderName = " Select One "; // Name of current renderer, or hint to select one
    if(mRenderer)
        curRenderName = mRenderer->getName().c_str();
    Widget mb1 = XtVaCreateManagedWidget("Menu", menuButtonWidgetClass, box, XtNlabel,curRenderName,
        XtNresize, false,
        XtNresizable, false,
        XtNwidth, col2w,    // Fixed width
        XtNheight, 18,
        XtNleft, XawChainLeft,
        XtNtop, XawChainTop,
        XtNright, XawChainLeft,
        XtNbottom, XawChainTop,
        XtNhorizDistance, col2x,
        XtNvertDistance, cury,
        NULL);

    Widget menu = XtVaCreatePopupShell("menu", simpleMenuWidgetClass, mb1,
        0, NULL);

    const RenderSystemList& renderers = Root::getSingleton().getAvailableRenderers();
    for (RenderSystemList::const_iterator pRend = renderers.begin();
                    pRend != renderers.end(); pRend++) {
        // Create callback data
        mRendererCallbackData.push_back(RendererCallbackData(this, *pRend, mb1));

        Widget entry = XtVaCreateManagedWidget("menuentry", smeBSBObjectClass, menu,
            XtNlabel, (*pRend)->getName().c_str(),
            0, NULL);
        XtAddCallback(entry, XtNcallback, (XtCallbackProc)&GLXConfigurator::renderSystemHandler, &mRendererCallbackData.back());
    }

    Widget bottomPanel = XtVaCreateManagedWidget("bottomPanel", formWidgetClass, box,
        XtNsensitive, True,
        XtNborderWidth, 0,
        XtNwidth, 150,  // Fixed width
        XtNleft, XawChainLeft,
        XtNtop, XawChainTop,
        XtNright, XawChainLeft,
        XtNbottom, XawChainTop,
        XtNhorizDistance, mWidth - 160,
        XtNvertDistance, mHeight - 40,
        NULL);

    Widget helloButton = XtVaCreateManagedWidget("cancelButton", commandWidgetClass, bottomPanel, XtNlabel," Cancel ", NULL);
    XtAddCallback(helloButton, XtNcallback, (XtCallbackProc)&GLXConfigurator::cancelHandler, this);

    Widget exitButton = XtVaCreateManagedWidget("acceptButton", commandWidgetClass, bottomPanel, XtNlabel," Accept ", XtNfromHoriz,helloButton, NULL);
    XtAddCallback(exitButton, XtNcallback, (XtCallbackProc)&GLXConfigurator::acceptHandler, this);

    XtRealizeWidget(toplevel);

    if(mRenderer)
        /* There was already a renderer selected; display its options */
        SetRenderer(mRenderer);

    return true;
}

Pixmap GLXConfigurator::CreateBackdrop(Window rootWindow, int depth) {
    int bpl;
    /* Find out number of bytes per pixel */
    switch(depth) {
    default:
        LogManager::getSingleton().logMessage("GLX backdrop: Unsupported bit depth");
        /* Unsupported bit depth */
        return 0;
    case 15:
    case 16:
        bpl = 2; break;
    case 24:
    case 32:
        bpl = 4; break;
    }
    /* Create background pixmap */
    unsigned char *data = 0; // Must be allocated with malloc

    try {
        Image img;

        // Load backdrop image using OGRE
        String path = FileSystemLayer::resolveBundlePath(OGRE_MEDIA_DIR "/../GLX_backdrop.png");
        DataStreamPtr imgStreamPtr = Root::openFileStream(path);
        img.load(imgStreamPtr, "png");

        PixelBox src = img.getPixelBox(0, 0);

        // Convert and copy image
        data = (unsigned char*)malloc(mWidth * mHeight * bpl); // Must be allocated with malloc
        PixelBox dst(mWidth, mHeight, 1, bpl == 2 ? PF_B5G6R5 : PF_A8R8G8B8, data);

        memset(data, 0, dst.getConsecutiveSize());
        dst.bottom = src.bottom;
        PixelUtil::bulkPixelConversion(src, dst);
    } catch(Exception &e) {
        // Could not find image; never mind
        LogManager::getSingleton().logWarning("Can not load backdrop for config dialog: " + e.getDescription());
        return 0;
    }

    GC context = XCreateGC (mDisplay, rootWindow, 0, NULL);

    /* put my pixmap data into the client side X image data structure */
    XImage *image = XCreateImage (mDisplay, NULL, depth, ZPixmap, 0,
        (char*)data,
        mWidth, mHeight, 8,
        mWidth*bpl);
#if OGRE_ENDIAN == OGRE_ENDIAN_BIG
    image->byte_order = MSBFirst;
#else
    image->byte_order = LSBFirst;
#endif

    /* tell server to start managing my pixmap */
    Pixmap rv = XCreatePixmap(mDisplay, rootWindow, mWidth,
        mHeight, depth);

    /* copy from client to server */
    XPutImage(mDisplay, rv, context, image, 0, 0, 0, 0,
        mWidth, mHeight);

    /* free up the client side pixmap data area */
    XDestroyImage(image); // also cleans data
    XFreeGC(mDisplay, context);

    return rv;
}
bool GLXConfigurator::Init() {
    // Init misc resources
    return true;
}
void GLXConfigurator::Draw() {
}
void GLXConfigurator::Main() {
    XtAppMainLoop(appContext);
}
void GLXConfigurator::Exit() {
    XtAppSetExitFlag(appContext);
}

void GLXConfigurator::SetRenderer(RenderSystem *r) {
    mRenderer = r;

    // Destroy each widget of GUI of previously selected renderer
    for(std::list<Widget>::iterator i=mRenderOptionWidgets.begin(); i!=mRenderOptionWidgets.end(); i++)
        XtDestroyWidget(*i);
    mRenderOptionWidgets.clear();

    // Create option GUI
    int cury = ystart + 1*rowh + 10;

    ConfigOptionMap options = mRenderer->getConfigOptions();
    // Process each option and create an optionmenu widget for it
    for (ConfigOptionMap::iterator it = options.begin();
                    it != options.end(); it++) {
        // if the config option does not have any possible value, then skip it.
        // if we create a popup with zero entries, it will crash when you click
        // on it.
        if (it->second.possibleValues.empty())
            continue;

        Widget lb1 = XtVaCreateManagedWidget("topLabel", labelWidgetClass, box, XtNlabel, it->second.name.c_str(), XtNborderWidth, 0,
            XtNwidth, col1w,    // Fixed width
            XtNheight, 18,
            XtNleft, XawChainLeft,
            XtNtop, XawChainTop,
            XtNright, XawChainLeft,
            XtNbottom, XawChainTop,
            XtNhorizDistance, col1x,
            XtNvertDistance, cury,
            XtNjustify, XtJustifyLeft,
            NULL);
        mRenderOptionWidgets.push_back(lb1);
        Widget mb1 = XtVaCreateManagedWidget("Menu", menuButtonWidgetClass, box, XtNlabel, it->second.currentValue.c_str(),
            XtNresize, false,
            XtNresizable, false,
            XtNwidth, col2w,    // Fixed width
            XtNheight, 18,
            XtNleft, XawChainLeft,
            XtNtop, XawChainTop,
            XtNright, XawChainLeft,
            XtNbottom, XawChainTop,
            XtNhorizDistance, col2x,
            XtNvertDistance, cury,
            NULL);
        mRenderOptionWidgets.push_back(mb1);

        Widget menu = XtVaCreatePopupShell("menu", simpleMenuWidgetClass, mb1,
            0, NULL);

        // Process each choice
        StringVector::iterator opt_it;
        for (opt_it = it->second.possibleValues.begin();
                        opt_it != it->second.possibleValues.end(); opt_it++) {
            // Create callback data
            mConfigCallbackData.push_back(ConfigCallbackData(this, it->second.name, *opt_it, mb1));

            Widget entry = XtVaCreateManagedWidget("menuentry", smeBSBObjectClass, menu,
                XtNlabel, (*opt_it).c_str(),
                0, NULL);
            XtAddCallback(entry, XtNcallback, (XtCallbackProc)&GLXConfigurator::configOptionHandler, &mConfigCallbackData.back());
        }
        cury += rowh;
    }
}

void GLXConfigurator::SetConfigOption(const String &optionName, const String &valueName) {
    if(!mRenderer)
        // No renderer set -- how can this be called?
        return;
    mRenderer->setConfigOption(optionName, valueName);
    SetRenderer(mRenderer);
}

//------------------------------------------------------------------------------------//
ConfigDialog::ConfigDialog() : mImpl(NULL)
{
}
//------------------------------------------------------------------------------------//
ConfigDialog::~ConfigDialog()
{
}
//------------------------------------------------------------------------------------//
bool ConfigDialog::display()
{
    GLXConfigurator test;
    /* Select previously selected rendersystem */
    if(Root::getSingleton().getRenderSystem())
        test.SetRenderSystem(Root::getSingleton().getRenderSystem());
    /* Attempt to create the window */
    if(!test.CreateWindow())
        OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Could not create configuration dialog",
               "GLXConfig::display");

    // Modal loop
    test.Main();
    if(!test.accept) // User did not accept
        return false;

    /* All done */
    Root::getSingleton().setRenderSystem(test.mRenderer);

    return true;
}
}

