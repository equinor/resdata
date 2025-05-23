#ifndef RD_UNITS_H
#define RD_UNITS_H

#ifdef __cplusplus
extern "C" {
#endif

#define RD_UNITS_CUBIC(x) ((x) * (x) * (x))
#define RD_UNITS_MILLI(x) ((x) * 0.001)
#define RD_UNITS_MEGA(x) ((x) * 1000000)

#define RD_UNITS_LENGTH_INCH 0.0254
#define RD_UNITS_LENGTH_FEET 12 * RD_UNITS_LENGTH_INCH

#define RD_UNITS_VOLUME_GALLON 231 * RD_UNITS_CUBIC(RD_UNITS_LENGTH_INCH)
#define RD_UNITS_VOLUME_BARREL RD_UNITS_VOLUME_GALLON * 42
#define RD_UNITS_VOLUME_LITER 0.001
#define RD_UNITS_VOLUME_MILLI_LITER RD_UNITS_MILLI(RD_UNITS_VOLUME_LITER)
#define RD_UNITS_VOLUME_GAS_FIELD                                              \
    RD_UNITS_MEGA(RD_UNITS_CUBIC(RD_UNITS_LENGTH_FEET))

#define RD_UNITS_TIME_HOUR 3600
#define RD_UNITS_TIME_DAY 24 * RD_UNITS_TIME_HOUR

#ifdef __cplusplus
}
#endif
#endif
