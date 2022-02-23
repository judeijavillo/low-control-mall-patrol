//
//  JSLevelConstants.h
//  JSONDemo
//
//  This file contains all of the details that define the JSON structure of a LevelModel.
//  Created by Walker White on 1/20/16.
//
//  Author: Walker White
//  Version: 1/20/16
//
#ifndef __LCMP_LEVEL_CONSTANTS_H__
#define __LCMP_LEVEL_CONSTANTS_H__

/** The global fields of the level model */
#define WIDTH_FIELD         "width"
#define HEIGHT_FIELD        "height"
#define GRAVITY_FIELD       "gravity"

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
#define LEVEL_ONE_FILE      "json/testmall.json"
/** The key for our loaded level */
#define LEVEL_ONE_KEY       "testmall"


#endif /* defined(__LCMP_LEVEL_CONSTANTS_H__) */