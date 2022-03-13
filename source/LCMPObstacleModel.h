//
//  LCMPObstacleModel.h
//  LowControlMallPatrol
//
//  Author: Kevin Games
//  Version: 3/04/22
//

#ifndef __LCMP_OBSTACLE_MODEL_H__
#define __LCMP_OBSTACLE_MODEL_H__
#include <cugl/cugl.h>

class ObstacleModel : public cugl::physics2::PolygonObstacle {
public:
//  MARK: - Enumerations
    
    /** The different type of obstacles you can make */
    enum Type {
        /** A hexagonal tree */
        TREE,
        /** A long bush */
        BUSH,
        /** A ferris wheel */
        FARIS
    };

    /** Defining the filter bits for the obstacle model*/
    b2Filter filter;
    
protected:
//  MARK: - Properties
    
    /** The type of obstacle this model represents */
    Type _type;
    
public:
//  MARK: - Constructors
    
    /**
     * Constructs an Obstacle Model
     */
    ObstacleModel() {};
    
    /**
     * Destructs an Obstacle Model
     */
    ~ObstacleModel() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of Obstacle Model
     */
    void dispose() {};
    
    /**
     * Initializes an Obstacle Model
     */
    bool init(float scale, shared_ptr<cugl::Texture>& texture, Type type);
    
};

#endif /* __LCMP_TRAP_MODEL_H__ */
