#ifndef ETERM_BOXDRAWDATA_HPP
#define ETERM_BOXDRAWDATA_HPP
/*
 * Copyright 2018 Avi Halachmi (:avih) avihpit@yahoo.com https://github.com/avih
 * MIT/X Consortium License
 */

/*
 * U+25XX codepoints data
 *
 * References:
 *   http://www.unicode.org/charts/PDF/U2500.pdf
 *   http://www.unicode.org/charts/PDF/U2580.pdf
 *
 * Test page:
 *   https://github.com/GNOME/vte/blob/master/doc/boxes.txt
 */

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

/* Data for U+2500 - U+259F except dashes/diagonals */
static const unsigned short boxdata[256] = {
	BDL + LH,
	BDL + HH,
	BDL + LV,
	BDL + HV,

	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	BDL + LD + LR,
	BDL + HR + LD,
	BDL + HD + LR,
	BDL + HD + HR,

	BDL + LD + LL,
	BDL + HL + LD,
	BDL + HD + LL,
	BDL + HD + HL,

	BDL + LU + LR,
	BDL + HR + LU,
	BDL + HU + LR,
	BDL + HU + HR,

	BDL + LU + LL,
	BDL + HL + LU,
	BDL + HU + LL,
	BDL + HU + HL,

	BDL + LV + LR,
	BDL + HR + LV,
	BDL + HU + LD + LR,
	BDL + HD + LR + LU,

	BDL + HV + LR,
	BDL + HU + HR + LD,
	BDL + HD + HR + LU,
	BDL + HV + HR,

	BDL + LV + LL,
	BDL + HL + LV,
	BDL + HU + LD + LL,
	BDL + HD + LU + LL,

	BDL + HV + LL,
	BDL + HU + HL + LD,
	BDL + HD + HL + LU,
	BDL + HV + HL,

	BDL + LH + LD,
	BDL + HL + LD + LR,
	BDL + HR + LL + LD,
	BDL + HH + LD,

	BDL + HD + LH,
	BDL + HD + HL + LR,
	BDL + HR + HD + LL,
	BDL + HH + HD,

	BDL + LH + LU,
	BDL + HL + LU + LR,
	BDL + HR + LU + LL,
	BDL + HH + LU,

	BDL + HU + LH,
	BDL + HU + HL + LR,
	BDL + HU + HR + LL,
	BDL + HH + HU,

	BDL + LV + LH,
	BDL + HL + LV + LR,
	BDL + HR + LV + LL,
	BDL + HH + LV,

	BDL + HU + LH + LD,
	BDL + HD + LH + LU,
	BDL + HV + LH,
	BDL + HU + HL + LD + LR,

	BDL + HU + HR + LD + LL,
	BDL + HD + HL + LU + LR,
	BDL + HD + HR + LU + LL,
	BDL + HH + HU + LD,

	BDL + HH + HD + LU,
	BDL + HV + HL + LR,
	BDL + HV + HR + LL,
	BDL + HV + HH,

	0,
	0,
	0,
	0,

	BDL + DH,
	BDL + DV,
	BDL + DR + LD,
	BDL + DD + LR,

	BDL + DR + DD,
	BDL + DL + LD,
	BDL + DD + LL,
	BDL + DL + DD,

	BDL + DR + LU,
	BDL + DU + LR,
	BDL + DU + DR,
	BDL + DL + LU,

	BDL + DU + LL,
	BDL + DL + DU,
	BDL + DR + LV,
	BDL + DV + LR,

	BDL + DV + DR,
	BDL + DL + LV,
	BDL + DV + LL,
	BDL + DV + DL,

	BDL + DH + LD,
	BDL + DD + LH,
	BDL + DD + DH,
	BDL + DH + LU,

	BDL + DU + LH,
	BDL + DH + DU,
	BDL + DH + LV,
	BDL + DV + LH,

	BDL + DH + DV,
	BDA + LD + LR,
	BDA + LD + LL,
	BDA + LU + LL,

	BDA + LU + LR,
	0,
	0,
	0,
	BDL + LL,

	BDL + LU,
	BDL + LR,
	BDL + LD,
	BDL + HL,

	BDL + HU,
	BDL + HR,
	BDL + HD,

	BDL + HR + LL,
	BDL + HD + LU,
	BDL + HL + LR,
	BDL + HU + LD,

	BBU + 4,
	BBD + 7,
	BBD + 6,
	BBD + 5,

	BBD + 4,
	BBD + 3,
	BBD + 2,
	BBD + 1,

	BBD + 0,
	BBL + 7,
	BBL + 6,
	BBL + 5,

	BBL + 4,
	BBL + 3,
	BBL + 2,
	BBL + 1,

	BBR + 4,
	BBS + 1,
	BBS + 2,
	BBS + 3,

	BBU + 1,
	BBR + 7,
	BBQ + BL,
	BBQ + BR,

	BBQ + TL,
	BBQ + TL + BL + BR,
	BBQ + TL + BR,
	BBQ + TL + TR + BL,

	BBQ + TL + TR + BR,
	BBQ + TR,
	BBQ + BL + TR,
	BBQ + BL + TR + BR,
};

#endif
