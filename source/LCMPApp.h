//
//  LCMPApp.h
//  Low Control Mall Patrol
//
//  This is the root class for Low Control Mall Patrol.
//
//  Originator: Walker White, Aidan Hobler
//  Author: Kevin Games
//  Version: 2/18/22
//

#ifndef __LCMP_APP_H__
#define __LCMP_APP_H__
#include <cugl/cugl.h>
#include "LCMPLoadingScene.h"
#include "LCMPMenuScene.h"
#include "LCMPHostScene.h"
#include "LCMPClientScene.h"
#include "LCMPFindScene.h"
//#include "LCMPCustomizeScene.h"
#include "LCMPLevelSelectScene.h"
#include "LCMPGameScene.h"
#include "LCMPVictoryScene.h"
#include "LCMPShopScene.h"
#include "LCMPGachaScene.h"

/**
 * This class represents the application root for Low Control Mall Patrol
 */
class LCMPApp : public cugl::Application {
protected:
//  MARK: - Enumerations
    
    /** The current active scene */
    enum State {
        /** The loading scene */
        LOAD,
        /** The main menu scene */
        MENU,
        /** The scene to host a game */
        HOST,
        /** The scene to join a game */
        CLIENT,
        /** The scene to find a game */
        FIND,
        /** The scene to customize characters */
        CUSTOM,
        /** The scene to choose a level */
        LEVEL,
        /** The scene to play the game */
        GAME,
        /** The scene to show the victory screen */
        VICTORY,
        /** The scene to show the shop */
        SHOP,
        /** The scene to show the gacha */
        GACHA
    };
    
//  MARK: - Properties
    
    /** The global sprite batch for drawing (only want one of these) */
    std::shared_ptr<cugl::SpriteBatch> _batch;
    /** The global asset manager */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** A reference to the Network Controller instance */
    std::shared_ptr<NetworkController> _network;
    /** A reference to the Audio Controller instance */
    std::shared_ptr<AudioController> _audio;
    /** A reference to the Action Manager */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;

    /** The controller for the loading screen */
    LoadingScene _loading;
    /** The menu scene to chose what to do */
    MenuScene _menu;
    /** The scene to host a game */
    HostScene _host;
    /** The scene to join a game */
    ClientScene _client;
    /** The scene to find a game */
    FindScene _find;
    /** The scene to choose a level */
    LevelSelectScene _levelselect;
    /** The primary controller for the game world */
    GameScene _game;
    /** The scene to show win / loss messages */
    VictoryScene _victory;
    /** The scene to show the shop */
    ShopScene _shop;
    /** The scene to show the gacha game */
    GachaScene _gacha;

    /** The key for which level the game will take place in */
    string _levelKey;

    /** The current active scene */
    State _scene;
    /** The previous frame's active scene */
    State _prevScene;

    /** Whether the game is in 16:9 aspect ratio or not */
    bool _sixteenNineAspectRatio;
    
public:
//  MARK: - Constructors
    
    /**
     * Creates, but does not initialized a new application.
     *
     * This constructor is called by main.cpp.  You will notice that, like
     * most of the classes in CUGL, we do not do any initialization in the
     * constructor.  That is the purpose of the init() method.  Separation
     * of initialization from the constructor allows main.cpp to perform
     * advanced configuration of the application before it starts.
     */
    LCMPApp() : cugl::Application() { _scene = State::LOAD; }
    
    /**
     * Disposes of this application, releasing all resources.
     *
     * This destructor is called by main.cpp when the application quits.
     * It simply calls the dispose() method in Application.  There is nothing
     * special to do here.
     */
    ~LCMPApp() {}
    
//  MARK: - Lifecycle
    
    /**
     * The method called after OpenGL is initialized, but before running the application.
     *
     * This is the method in which all user-defined program intialization should
     * take place.  You should not create a new init() method.
     *
     * When overriding this method, you should call the parent method as the
     * very last line.  This ensures that the state will transition to FOREGROUND,
     * causing the application to run.
     */
    virtual void onStartup() override;
    
    /**
     * The method called when the application is ready to quit.
     *
     * This is the method to dispose of all resources allocated by this
     * application.  As a rule of thumb, everything created in onStartup()
     * should be deleted here.
     *
     * When overriding this method, you should call the parent method as the
     * very last line.  This ensures that the state will transition to NONE,
     * causing the application to be deleted.
     */
    virtual void onShutdown() override;
    
    /**
     * The method called to update the application data.
     *
     * This is your core loop and should be replaced with your custom implementation.
     * This method should contain any code that is not an OpenGL call.
     *
     * When overriding this method, you do not need to call the parent method
     * at all. The default implmentation does nothing.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    virtual void update(float timestep) override;
    
    /**
     * The method called to draw the application to the screen.
     *
     * This is your core loop and should be replaced with your custom implementation.
     * This method should OpenGL and related drawing calls.
     *
     * When overriding this method, you do not need to call the parent method
     * at all. The default implmentation does nothing.
     */
    virtual void draw() override;

private:
//  MARK: - Helpers
    
    /**
     * Individualized update method for the loading scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the loading scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateLoadingScene(float timestep);

    /**
     * Individualized update method for the menu scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the menu scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateMenuScene(float timestep);
    
    void updateShopScene(float timestep);
    
    void updateGachaScene(float timestep);

    /**
     * Individualized update method for the host scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the host scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateHostScene(float timestep);
    
    /**
     * Individualized update method for the client scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the client scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateClientScene(float timestep);
    
    /**
     * Individualized update method for the find scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the find scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateFindScene(float timestep);
    
    /**
     * Individualized update method for the client scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the client scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateCustomizeScene(float timestep);

    /**
     * Individualized update method for the client scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the client scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateLevelSelectScene(float timestep);

    /**
     * Individualized update method for the game scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the game scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateGameScene(float timestep);
    
    /**
     * Individualized update method for the victory scene.
     *
     * This method keeps the primary {@link #update} from being a mess of switch
     * statements. It also handles the transition logic from the game scene.
     *
     * @param timestep  The amount of time (in seconds) since the last frame
     */
    void updateVictoryScene(float timestep);
    
};

#endif /* __LCMP_APP_H__ */
