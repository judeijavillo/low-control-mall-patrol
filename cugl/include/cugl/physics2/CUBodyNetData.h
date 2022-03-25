#ifndef __CU_BODY_NET_DATA_H__
#define __CU_BODY_NET_DATA_H__

#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>

namespace cugl {
    /**
     * The classes to represent 2-d physics.
     *
     * This namespace was chosen to future-proof the game engine. We will
     * eventually want to add a 3-d physics engine as well, and this namespace
     * will prevent any collisions with those scene graph nodes.
     */
    namespace physics2 {
        class BodyNetData {
        public:
            unsigned long id;
            b2BodyType type;
            b2Vec2 position;
            float angle;
            bool enabled;
            bool awake;
            bool bullet;
            b2Vec2 linearVelocity;
            bool sleepingAllowed;
            bool fixedRotation;
            float gravityScale;
            float angularDamping;
            float linearDamping;

            BodyNetData() {}
        };

    }
}

#endif
