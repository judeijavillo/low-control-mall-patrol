//
//  CUMathBase.h
//  CUGL
//
//  Created by Walker White on 5/30/16.
//  Copyright © 2016 Game Design Initiative at Cornell. All rights reserved.
//

// TODO: COMMENT THIS FILE
#ifndef __CU_MATH_BASE_H__
#define __CU_MATH_BASE_H__
#include <string>
#include <math.h>
#include <algorithm>
#include <cugl/base/CUBase.h>

#ifndef M_PI
    #define M_PI        3.14159265358979323846264338327
#endif

#if defined (__WINDOWS__)
    #define M_PI_2     1.57079632679489661923f   // pi/2
    #define M_PI_4     0.785398163397448309616f  // pi/4
	#define __attribute__(x)	/** Not visual studio compatible */
#endif

// More PI variations
#define M_3_PI_4    (3 * M_PI / 4)


/** Util macro for conversion from degrees to radians.*/
#define CU_MATH_DEG_TO_RAD(x)          ((x) * 0.0174532925f)
/** Util macro for conversion from radians to degrees.*/
#define CU_MATH_RAD_TO_DEG(x)          ((x)* 57.29577951f)
/** Compare two values for approximate equality */
#define CU_MATH_APPROX(x,y,t)          ( -(t) < (x)-(y) && (x)-(y) < (t) )

/** Small epsilon for high precision */
#define CU_MATH_FLOAT_SMALL            1.0e-30f     // Set by SSE
/** Normal epsilon for testing and other applications */
#define CU_MATH_EPSILON                5.0e-4f      // Set by SSE

#if defined (__WINDOWS__)
    #define NOMAXMIN
#endif

#if defined (__ANDROID__)
    #include <android/cpu-features.h>
#endif

// Define the vectorization support
// By experimentation, there are only two vectorizations worth supporting.
// And even Neon64 is questionable on -Os (autovectoization is better).
#if defined (CU_VECTORIZE) && defined (__arm64__)
    #define CU_MATH_VECTOR_NEON64
    #include <arm_neon.h>
#elif defined (CU_VECTORIZE) && defined (__SSE__)
    #define CU_MATH_VECTOR_SSE
    #include "immintrin.h"
    #include "smmintrin.h"
    #include "xmmintrin.h"
#endif

/**
 * Returns value, clamped to the range [min,max]
 *
 * This function only works on floats
 *
 * @param value	The original value
 * @param min	The range minimum
 * @param max	The range maximum
 *
 * @return value, clamped to the range [min,max]
 */
inline float clampf(float value, float min, float max) {
    return value < min ? min : value < max? value : max;
}

/**
 * Returns value, clamped to the range [min,max]
 *
 * This function only works on bytes
 *
 * @param value	The original value
 * @param min	The range minimum
 * @param max	The range maximum
 *
 * @return value, clamped to the range [min,max]
 */
inline GLubyte clampb(GLuint value, GLubyte min, GLubyte max) {
    return static_cast<GLubyte>(value < min ? min : value < max? value : max);
}

/**
 * Returns value, clamped to the range [min,max]
 *
 * This function only works on integers
 *
 * @param value    The original value
 * @param min    The range minimum
 * @param max    The range maximum
 *
 * @return value, clamped to the range [min,max]
 */
inline int clampi(int value, int min, int max) {
    return value < min ? min : value < max? value : max;
}

/**
 * Returns the number of segments necessary for the given tolerance
 *
 * This function is used to compute the number of segments to approximate
 * a radial curve at the given level of tolerance.
 *
 * @param rad   The circle radius
 * @param arc   The arc in radians
 * @param tol   The error tolerance
 *
 * @return the number of segments necessary for the given tolerance
 */
inline Uint32 curveSegs(float rad, float arc, float tol) {
    float da = acosf(rad / (rad + tol)) * 2.0f;
    return std::max(2, (int)(ceilf(arc / da)));
}

/**
 * Returns the power of two greater than or equal to x
 *
 * @param x		The original integer
 *
 * @return the power of two greater than or equal to x
 */
Uint32 nextPOT(Uint32 x);

#endif /* CU_MATH_BASE_H */
