#ifndef XMATH_H
#define XMATH_H

#include "xstyle.h"

#include <math.h>

#define Max(value1, value2) (value1 >= value2) ? value1 : value2
#define Min(value1, value2) (value1 <= value2) ? value1 : value2
#define IsBetween(value, lower, upper) (((lower) <= (value)) && (upper) >= (value))
#define Sq(v) ((v) * (v))
#define Sqrt(v) (sqrt(v)) // TODO

// == Vec2 ====================================================================

#define Vec2(type) { \
    struct { type x;     type y;      }; \
    struct { type width; type height; }; \
    type elements[2];                    \
}
typedef union Vec2(u8)  Vec2u8;
typedef union Vec2(u16) Vec2u16;
typedef union Vec2(u32) Vec2u32;
typedef union Vec2(u64) Vec2u64;
typedef union Vec2(i8)  Vec2i8;
typedef union Vec2(i16) Vec2i16;
typedef union Vec2(i32) Vec2i32;
typedef union Vec2(i64) Vec2i64;
typedef union Vec2(f32) Vec2f32;
typedef union Vec2(f64) Vec2f64;

#define Vec2Mul(a, b) { ((a).x * (b).x),  ((a).y * (b).y) }
#define Vec2MulT(type, a, b) { (type) ((a).x * (b).x),  (type) ((a).y * (b).y) }

#define Vec2Add(a, b) { ((a).x + (b).x), ((a).y + (b).y) }
#define Vec2AddT(type, a, b) { (type) ((a).x + (b).x), (type) ((a).y + (b).y) }

#define Vec2Sub(a, b) { ((a).x - (b).x), ((a).y - (b).y) }
#define Vec2SubT(type, a, b) { (type) ((a).x - (b).x), (type) ((a).y - (b).y) }

#define Vec2Min(a, b) { (Min((a).x, (b).x)), (Min((a).y, (b).y)) }
#define Vec2MinT(type, a, b) { (type) (Min((a).x, (b).x)), (type) (Min((a).y, (b).y)) }

#define Vec2Max(a, b) { (Max((a).x, (b).x)), (Max((a).y, (b).y)) }
#define Vec2MaxT(type, a, b) { (type) (Max((a).x, (b).x)), (type) (Max((a).y, (b).y))}

#define Vec2Dot(a, b) (((a).x * (b).x) + ((a).y * (b).y))

#define Vec2LenSq(v) (Sq((v).x) + Sq((v).y))
#define Vec2Len(v) (Sqrt(Vec2LenSq(v)))

#define Vec2Dist(a, b) (Sq((a).x - (b).x) + Sq((a).y - (b).y))
#define Vec2DistSq(a, b) (Sq(Vec2Dist(a, b)))

// Vec2Norm
// Vec2Perp
// Vec2IsPerp
// Vec2IsEq
// Vec2Clamp01
// Vec2Lerp
// Vec2MoveTo

// == Vec3 ====================================================================

#define Vec3(type) { \
    struct { type x;     type y;      type z;     }; \
    struct { type width; type height; type depth; }; \
    type elements[3];                                \
}
typedef union Vec3(u8)  Vec3u8;
typedef union Vec3(u16) Vec3u16;
typedef union Vec3(u32) Vec3u32;
typedef union Vec3(u64) Vec3u64;
typedef union Vec3(i8)  Vec3i8;
typedef union Vec3(i16) Vec3i16;
typedef union Vec3(i32) Vec3i32;
typedef union Vec3(i64) Vec3i64;
typedef union Vec3(f32) Vec3f32;
typedef union Vec3(f64) Vec3f64;

#define Vec3Mul(a, b) { ((a).x * (b).x),  ((a).y * (b).y), ((a).z * (b).z) }
#define Vec3MulT(type, a, b) { (type) ((a).x * (b).x),  (type) ((a).y * (b).y), (type) ((a).z * (b).z) }

#define Vec3Add(a, b) { ((a).x + (b).x), ((a).y + (b).y), ((a).z + (b).z) }
#define Vec3AddT(type, a, b) { (type) ((a).x + (b).x), (type) ((a).y + (b).y), (type) ((a).z + (b).z) }

#define Vec3Sub(a, b) { ((a).x - (b).x), ((a).y - (b).y), ((a).z - (b).z) }
#define Vec3SubT(type, a, b) { (type) ((a).x - (b).x), (type) ((a).y - (b).y), (type) ((a).z - (b).z) }

#define Vec3Min(a, b) { (Min((a).x, (b).x)), (Min((a).y, (b).y)), (Min((a).z, (b).z)) }
#define Vec3MinT(type, a, b) { (type) (Min((a).x, (b).x)), (type) (Min((a).y, (b).y)), (type) (Min((a).z, (b).z)) }

#define Vec3Max(a, b) { (Max((a).x, (b).x)), (Max((a).y, (b).y)), (Max((a).z, (b).z)) }
#define Vec3MaxT(type, a, b) { (type) (Max((a).x, (b).x)), (type) (Max((a).y, (b).y)), (type) (Max((a).z, (b).z)) }

#define Vec2Dot(a, b) (((a).x * (b).x) + ((a).y * (b).y))

#define Vec3LenSq(v) (Sq((v).x) + Sq((v).y) + Sq((v).z))
#define Vec3Len(v) (Sqrt(Vec3LenSq(v)))

#define Vec3Dist(a, b) (Sq((a).x - (b).x) + Sq((a).y - (b).y) + Sq((a).z - (b).z))
#define Vec3DistSq(a, b) (Sq(Vec3Dist(a, b)))

// Vec3Norm
// Vec3Perp
// Vec3IsPerp
// Vec3IsEq
// Vec3Clamp01
// Vec3Lerp
// Vec3MoveTo

// == Matrix ==================================================================
// TODO

#endif // XMATH_H