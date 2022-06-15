/*
 * Copyright 2018 Avi Halachmi (:avih) avihpit@yahoo.com https://github.com/avih
 * MIT/X Consortium License
 */
/* Each shape is encoded as 16-bits. Higher bits are category, lower are data */
/* Categories (mutually exclusive except BDB): */
/* For convenience, BDL/BDA/BBS/BDB are 1 bit each, the rest are enums */
#define BDL ( 1 << 8 ) /* Box Draw Lines (light/double/heavy) */
#define BDA ( 1 << 9 ) /* Box Draw Arc (light) */

#define BBD ( 1 << 10 ) /* Box Block Down (lower) X/8 */
#define BBL ( 2 << 10 ) /* Box Block Left X/8 */
#define BBU ( 3 << 10 ) /* Box Block Upper X/8 */
#define BBR ( 4 << 10 ) /* Box Block Right X/8 */
#define BBQ ( 5 << 10 ) /* Box Block Quadrants */
#define BRL ( 6 << 10 ) /* Box Braille (data is lower byte of U28XX) */

#define BBS ( 1 << 14 ) /* Box Block Shades */
#define BDB ( 1 << 15 ) /* Box Draw is Bold */

/* (BDL/BDA) Light/Double/Heavy x Left/Up/Right/Down/Horizontal/Vertical      */
/* Heavy is light+double (literally drawing light+double align to form heavy) */
#define LL ( 1 << 0 )
#define LU ( 1 << 1 )
#define LR ( 1 << 2 )
#define LD ( 1 << 3 )
#define LH ( LL + LR )
#define LV ( LU + LD )

#define DL ( 1 << 4 )
#define DU ( 1 << 5 )
#define DR ( 1 << 6 )
#define DD ( 1 << 7 )
#define DH ( DL + DR )
#define DV ( DU + DD )

#define HL ( LL + DL )
#define HU ( LU + DU )
#define HR ( LR + DR )
#define HD ( LD + DD )
#define HH ( HL + HR )
#define HV ( HU + HD )

/* (BBQ) Quadrants Top/Bottom x Left/Right */
#define TL ( 1 << 0 )
#define TR ( 1 << 1 )
#define BL ( 1 << 2 )
#define BR ( 1 << 3 )