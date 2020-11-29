//
//  CelestiaController.h
//  celestia
//
//  Created by Bob Ippolito on Tue May 28 2002.
//  Copyright (C) 2007, Celestia Development Team
//

#import "CelestiaAppCore.h"
#import "CelestiaSettings.h"
#import "FavoritesDrawerController.h"
#import "RenderPanelController.h"
#import "BrowserWindowController.h"

#define CELESTIA_RESOURCES_FOLDER @"CelestiaResources"

@class CelestiaOpenGLView;
@class SplashWindow;
@class SplashWindowController;
@class EclipseFinderController;
@class ScriptsController;
@class ConfigSelectionWindowController;

@interface CelestiaController : NSWindowController <NSWindowDelegate>
{
    CelestiaSettings* settings;
    CelestiaAppCore* appCore;
    BOOL ready;
    BOOL isDirty;
    BOOL needsRelaunch;
    BOOL forceQuit;
    IBOutlet SplashWindowController *splashWindowController;
    IBOutlet NSTextView *glInfo;
    IBOutlet NSPanel *glInfoPanel;
    IBOutlet CelestiaOpenGLView *glView;
    NSWindow *origWindow;
    IBOutlet FavoritesDrawerController *favoritesDrawerController;
    IBOutlet RenderPanelController *renderPanelController;
    IBOutlet ScriptsController *scriptsController;
    BrowserWindowController *browserWindowController;
    EclipseFinderController *eclipseFinderController;
    NSWindowController *helpWindowController;
    ConfigSelectionWindowController *configSelectionWindowController;
    NSTimer* timer;

    NSConditionLock* startupCondition;
    NSInteger keyCode, keyTime;
    NSString* lastScript;
    NSString *pendingScript;
    NSString *pendingUrl;
}
-(void)setNeedsRelaunch:(BOOL)needsRelaunch;
-(BOOL)applicationShouldTerminate:(id)sender;
-(BOOL)windowShouldClose:(id)sender;
-(IBAction)back:(id)sender;
-(IBAction)forward:(id)sender;
-(IBAction)showGLInfo:(id)sender;
-(IBAction)showInfoURL:(id)sender;
-(void)runScript: (NSString*) path;
-(IBAction)openScript:(id)sender;
-(IBAction)rerunScript: (id) sender;
-(void)setDirty;
-(void)forceDisplay;
-(void)resize;
-(BOOL)startInitialization;
-(void)finishInitialization;
-(void)display;
-(void)awakeFromNib;
-(void)delegateKeyDown:(NSEvent *)theEvent;
-(void)keyPress:(NSInteger)code hold:(NSInteger)time;
-(void)setupResourceDirectory;
+(CelestiaController*) shared;
-(void) fatalError: (NSString *) msg;

-(IBAction) showPanel: (id) sender;

-(IBAction) captureMovie: (id) sender;

-(BOOL)validateMenuItem:(id)item;
-(IBAction)activateMenuItem:(id)item;

-(void)showHelp:(id)sender;
@end