//
//  LCMPConstants.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/18/22
//

#ifndef __LCMP_CONSTANTS_H__
#define __LCMP_CONSTANTS_H__

//  MARK: - JSON Constants

/** The global fields of the level model */
#define LAYERS_FIELD        "layers"
#define WIDTH_FIELD         "width"
#define HEIGHT_FIELD        "height"
#define T_SIZE_FIELD        "tilewidth"
#define X_FIELD             "x"
#define Y_FIELD             "y"
#define ROTATION_FIELD      "rotation"
#define OBJECTS_FIELD       "objects"
#define ELLIPSE_FIELD       "ellipse"
#define POLYGON_FIELD       "polygon"
#define FLOOR_FIELD         0
#define TRAPS_FIELD         1
#define WALLS_FIELD         2
#define COPS_FIELD          3
#define THIEF_FIELD         4
#define POINT_FIELD			"point"
#define PROPERTIES_FIELD    "properties"
#define ID_FIELD			"id"

/** The global fields for trap properties*/
enum JsonConstants :int {
	ACTIVATED,
	COP_COLLIDE,
	COP_EFFECT,
	COP_LINGER_DURATION,
	COP_LINGER_EFFECT,
	EFFECT_AREA,
	NUM_USAGES,
	THIEF_COLLIDE,
	THIEF_EFFECT,
	THIEF_LINGER_DURATION,
	THIEF_LINGER_EFFECT,
	TRIGGER_AREA,
	TRIGGER_DEACTIVATION_AREA,


	ESCALATOR,
	TELEPORT,
	STAIRS,
	VELOCITY_MODIFIER,


	NIL
};

#define VALUE_FIELD				    "value"
#define NAME_FIELD				    "name"
#define TRAP_TYPE					"Type"
#define LINGER_DURATION				"LingerDuration"
#define ESCALATOR_VELOCITY			"Escalator Velocity"
#define STAIRCASE_VELOCITY			"Staircase Vector"
#define TELEPORT_LOCATION			"Teleport Location"
#define SPEED_MODIFIER				"Speed Modifier"

/** The physics fields for each object */
#define POSITION_FIELD      "pos"
#define SIZE_FIELD          "size"
#define BODYTYPE_FIELD      "bodytype"
#define DENSITY_FIELD       "density"
#define FRICTION_FIELD      "friction"
#define RESTITUTION_FIELD   "restitution"
#define DAMPING_FIELD       "damping"
#define ROTATION_FIELD      "rotation"
#define STATIC_VALUE        "static"

/** The drawing fields for each object */
#define TEXTURE_FIELD       "texture"
#define DEBUG_COLOR_FIELD   "debugcolor"
#define DEBUG_OPACITY_FIELD "debugopacity"

/** The source for our level file */
#define LEVEL_ONE_FILE      "maps/example3.json"
/** The key for our loaded level */
#define LEVEL_ONE_KEY       "example3"

//  MARK: - Physics Constants

/** Music constants */
#define THIEF_COLLISION_SFX     "fuck"
#define COP_COLLISION_SFX       "ooh"
#define OBJ_COLLISION_SFX       "dude"
#define TRAP_COLLISION_SFX      "trap"
#define TACKLE_SFX              "tackle"
#define LAND_SFX                "land"
#define CLICK_SFX               "click"
#define BACK_SFX                "back"
#define GAME_MUSIC              "game"
#define LOADING_MUSIC           "loading"
#define MENU_MUSIC              "menu"
#define SFX_COOLDOWN            2.0f
#define MUSIC_VOLUME            0.5f
#define SFX_VOLUME              0.5f

/** Player constants */
// Thief Physics Constants
/** The thief's damping coefficient */
#define THIEF_DAMPING_DEFAULT 30.0f
/** The thief's maximum speed of this player */
#define THIEF_MAX_SPEED_DEFAULT 10.0f
/** The thief's acceleration of this player */
#define THIEF_ACCELERATION_DEFAULT 300.0f

// Cop Physics Constants
/** The cop's damping coefficient */
#define COP_DAMPING_DEFAULT 7.5f
/** The cop's maximum speed of this player */
#define COP_MAX_SPEED_DEFAULT 10.0f
/** The cops's acceleration of this player */
#define COP_ACCELERATION_DEFAULT 75.0f

// Tackle Physics Constants
/** The amount of time that the cop will be in the air while in a successful tackle */
#define TACKLE_AIR_TIME 0.25f
/** The amount of time that the cop will be in the air while in a failed tackle*/
#define TACKLE_COOLDOWN_TIME 1.5f
/** The movement multiplier for the cop during tackle */
#define TACKLE_MOVEMENT_MULT 2.0f
/** The damping multiplier for the cop during tackle */
#define TACKLE_DAMPING_MULT 5.0f
/** The distance the cop's successful tackle spans in Box2D units */
#define TACKLE_HIT_RADIUS 6.0f
/** The margin of error allowed by a ocp's tackle */
#define TACKLE_ANGLE_MAX_ERR (M_PI_4)

// MARK: - UI Constants

/** This is how long the game will be */
#define GAME_LENGTH     160

/** This is the size of the active portion of the screen */
#define SCENE_WIDTH     1024
/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT    576
/** Amount UI elements should be shifted down to remain on screen */
#define SCENE_HEIGHT_ADJUST 80
/** Amount UI elements should be shifted left or right to remain on screen */
#define SCENE_WIDTH_ADJUST 80

/** The radius of the joystick*/
#define JOYSTICK_RADIUS     100
/** This defines the joystick "deadzone" (how far we must move) */
#define JOYSTICK_DEADZONE   15


#endif /* __LCMP_CONSTANTS_H__ */ 
