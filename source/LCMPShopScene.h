//
//  LCMPShopScene.h
//  Low Control Mall Patrol
//
//  This class provides the shop logic.
//
//  Author: Kevin Games
//  Version: 5/4/22
//

#ifndef LCMPShopScene_h
#define LCMPShopScene_h

#include <cugl/cugl.h>
#include <vector>
#include "LCMPAudioController.h"

class ShopScene : public cugl::Scene2 {
public:
//  MARK: - Enumerations
    /** The menu choice. */
    enum Choice {
        NONE,
        BACK,
        BUY
    };
    
protected:
//  MARK: - Properties
    /** The amount to move the world node by to center it in the scene */
    cugl::Vec2 _offset;
    /** The asset manager for this scene. */
    std::shared_ptr<cugl::AssetManager> _assets;
    /** The sound controller for the game */
    std::shared_ptr<AudioController> _audio;
    
    std::shared_ptr<cugl::scene2::SpriteNode> _thief;
    std::unordered_map<std::string, std::shared_ptr<cugl::scene2::PolygonNode>> _skins;
//    std::vector<std::shared_ptr<cugl::scene2::Button>> _buttons;
    std::unordered_map<std::string, std::shared_ptr<cugl::scene2::Button>> _buttons;
    std::shared_ptr<cugl::scene2::Button> _backButton;
    std::shared_ptr<cugl::scene2::Button> _buyButton;
    std::unordered_map<std::string, bool> _purchases;
    std::string _selected = "";
    std::shared_ptr<cugl::scene2::Label> _title;
    std::vector<std::string> _keys;
    cugl::Size _dimen;
    /** The previous timestep. */
    float _prevTime;
    
    /** The player menu choice */
    Choice _choice;

public:
//  MARK: - Constructors
    /**
     * Creates a new  menu scene with the default values.
     *
     * This constructor does not allocate any objects or start the game.
     * This allows us to use the object without a heap pointer.
     */
    ShopScene() : cugl::Scene2() {}
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     *
     * This method is different from dispose() in that it ALSO shuts off any
     * static resources, like the input controller.
     */
    ~ShopScene() { dispose(); }
    
    /**
     * Disposes of all (non-static) resources allocated to this mode.
     */
    void dispose() override;
    
    void update(float timestep) override;

    /**
     * Initializes the controller contents.
     *
     * In previous labs, this method "started" the scene.  But in this
     * case, we only use to initialize the scene user interface.  We
     * do not activate the user interface yet, as an active user
     * interface will still receive input EVEN WHEN IT IS HIDDEN.
     *
     * That is why we have the method {@link #setActive}.
     *
     * @param assets    The (loaded) assets for this game mode
     *
     * @return true if the controller is initialized properly, false otherwise.
     */
    bool init(const std::shared_ptr<cugl::AssetManager>& assets,
              std::shared_ptr<AudioController>& audio);
    
//  MARK: - Methods
    /**
     * Sets whether the scene is currently active
     *
     * This method should be used to toggle all the UI elements.  Buttons
     * should be activated when it is made active and deactivated when
     * it is not.
     *
     * @param value whether the scene is currently active
     */
    virtual void setActive(bool value) override;
    
    Choice getChoice() const { return _choice; }
    
    void resetSkins();
    
    void buttonListener(std::shared_ptr<cugl::scene2::Button> button, string key);

};

#endif /* LCMPShopScene_h */
