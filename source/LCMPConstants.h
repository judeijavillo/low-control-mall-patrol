//
//  LCMPConstants.h
//  LowControlMallPatrol
//
//  Created by Jude Javillo on 3/10/22.
//  Copyright © 2022 Game Design Initiative at Cornell. All rights reserved.
//

#ifndef __LCMP_CONSTANTS_H__
#define __LCMP_CONSTANTS_H__

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
#define WALLS_FIELD         1
#define COPS_FIELD          2
#define THIEF_FIELD         3

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

#endif /* __LCMP_CONSTANTS_H__ */
