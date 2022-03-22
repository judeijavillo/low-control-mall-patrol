//
//  LCMPConstants.h
//  LowControlMallPatrol
//
//  Created by Jude Javillo on 3/10/22.
//  Copyright © 2022 Game Design Initiative at Cornell. All rights reserved.
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
#define TRAP_ACTIVATED			    0
#define TRAP_COP_COLLIDE		    1
#define TRAP_COP_SPEED_MODIFIER	    2
#define TRAP_EFFECT_AREA		    3
#define TRAP_THIEF_COLLIDE		    4
#define TRAP_THIEF_SPEED_MODIFIER   5
#define TRAP_TRIGGER_AREA		    6
#define VALUE_FIELD				    "value"

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
#define LEVEL_ONE_FILE      "maps/example1.json"
/** The key for our loaded level */
#define LEVEL_ONE_KEY       "example1"

//  MARK: - Physics Constants

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
/** The amount of time that the cop will be in the air while tackling */
#define TACKLE_AIR_TIME 0.25f
/** The movement multiplier for the cop during tackle */
#define TACKLE_MOVEMENT_MULT 2.0f
/** The damping multiplier for the cop during tackle */
#define TACKLE_DAMPING_MULT 2.5f

// MARK: - UI Constants

/** This is the size of the active portion of the screen */
#define SCENE_WIDTH     1024
/** Regardless of logo, lock the height to this */
#define SCENE_HEIGHT    576
/** Amount UI elements should be shifted down to remain on screen */
#define SCENE_HEIGHT_ADJUST 80

/** The radius of the joystick*/
#define JOYSTICK_RADIUS     100
/** This defines the joystick "deadzone" (how far we must move) */
#define JOYSTICK_DEADZONE   15


#endif /* __LCMP_CONSTANTS_H__ */
