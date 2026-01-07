#pragma once

#include <eepp/config.hpp>
#include <eepp/math/vector2.hpp>

using namespace EE::Math;

namespace EE::Graphics {

class FontTrueType;

/** TextDirection values are the same as in hb_direction_t from HarfBuzz */
enum class TextDirection : Uint8 {
	Unspecified = 0, //!< Unspecified
	LeftToRight = 4, //!< Left-to-right
	RightToLeft,	 //!< Right-to-left
	TopToBottom,	 //!< Top-to-bottom
	BottomToTop		 //!< Bottom-to-top
};

/** LangScript values are the same as in hb_script_t from HarfBuzz */
enum class LangScript : Uint32 {
	COMMON = EE_TAG( 'Z', 'y', 'y', 'y' ),	  /*1.1*/
	INHERITED = EE_TAG( 'Z', 'i', 'n', 'h' ), /*1.1*/
	UNKNOWN = EE_TAG( 'Z', 'z', 'z', 'z' ),	  /*5.0*/

	ARABIC = EE_TAG( 'A', 'r', 'a', 'b' ),	   /*1.1*/
	ARMENIAN = EE_TAG( 'A', 'r', 'm', 'n' ),   /*1.1*/
	BENGALI = EE_TAG( 'B', 'e', 'n', 'g' ),	   /*1.1*/
	CYRILLIC = EE_TAG( 'C', 'y', 'r', 'l' ),   /*1.1*/
	DEVANAGARI = EE_TAG( 'D', 'e', 'v', 'a' ), /*1.1*/
	GEORGIAN = EE_TAG( 'G', 'e', 'o', 'r' ),   /*1.1*/
	GREEK = EE_TAG( 'G', 'r', 'e', 'k' ),	   /*1.1*/
	GUJARATI = EE_TAG( 'G', 'u', 'j', 'r' ),   /*1.1*/
	GURMUKHI = EE_TAG( 'G', 'u', 'r', 'u' ),   /*1.1*/
	HANGUL = EE_TAG( 'H', 'a', 'n', 'g' ),	   /*1.1*/
	HAN = EE_TAG( 'H', 'a', 'n', 'i' ),		   /*1.1*/
	HEBREW = EE_TAG( 'H', 'e', 'b', 'r' ),	   /*1.1*/
	HIRAGANA = EE_TAG( 'H', 'i', 'r', 'a' ),   /*1.1*/
	KANNADA = EE_TAG( 'K', 'n', 'd', 'a' ),	   /*1.1*/
	KATAKANA = EE_TAG( 'K', 'a', 'n', 'a' ),   /*1.1*/
	LAO = EE_TAG( 'L', 'a', 'o', 'o' ),		   /*1.1*/
	LATIN = EE_TAG( 'L', 'a', 't', 'n' ),	   /*1.1*/
	MALAYALAM = EE_TAG( 'M', 'l', 'y', 'm' ),  /*1.1*/
	ORIYA = EE_TAG( 'O', 'r', 'y', 'a' ),	   /*1.1*/
	TAMIL = EE_TAG( 'T', 'a', 'm', 'l' ),	   /*1.1*/
	TELUGU = EE_TAG( 'T', 'e', 'l', 'u' ),	   /*1.1*/
	THAI = EE_TAG( 'T', 'h', 'a', 'i' ),	   /*1.1*/

	TIBETAN = EE_TAG( 'T', 'i', 'b', 't' ), /*2.0*/

	BOPOMOFO = EE_TAG( 'B', 'o', 'p', 'o' ),		   /*3.0*/
	BRAILLE = EE_TAG( 'B', 'r', 'a', 'i' ),			   /*3.0*/
	CANADIAN_SYLLABICS = EE_TAG( 'C', 'a', 'n', 's' ), /*3.0*/
	CHEROKEE = EE_TAG( 'C', 'h', 'e', 'r' ),		   /*3.0*/
	ETHIOPIC = EE_TAG( 'E', 't', 'h', 'i' ),		   /*3.0*/
	KHMER = EE_TAG( 'K', 'h', 'm', 'r' ),			   /*3.0*/
	MONGOLIAN = EE_TAG( 'M', 'o', 'n', 'g' ),		   /*3.0*/
	MYANMAR = EE_TAG( 'M', 'y', 'm', 'r' ),			   /*3.0*/
	OGHAM = EE_TAG( 'O', 'g', 'a', 'm' ),			   /*3.0*/
	RUNIC = EE_TAG( 'R', 'u', 'n', 'r' ),			   /*3.0*/
	SINHALA = EE_TAG( 'S', 'i', 'n', 'h' ),			   /*3.0*/
	SYRIAC = EE_TAG( 'S', 'y', 'r', 'c' ),			   /*3.0*/
	THAANA = EE_TAG( 'T', 'h', 'a', 'a' ),			   /*3.0*/
	YI = EE_TAG( 'Y', 'i', 'i', 'i' ),				   /*3.0*/

	DESERET = EE_TAG( 'D', 's', 'r', 't' ),	   /*3.1*/
	GOTHIC = EE_TAG( 'G', 'o', 't', 'h' ),	   /*3.1*/
	OLD_ITALIC = EE_TAG( 'I', 't', 'a', 'l' ), /*3.1*/

	BUHID = EE_TAG( 'B', 'u', 'h', 'd' ),	 /*3.2*/
	HANUNOO = EE_TAG( 'H', 'a', 'n', 'o' ),	 /*3.2*/
	TAGALOG = EE_TAG( 'T', 'g', 'l', 'g' ),	 /*3.2*/
	TAGBANWA = EE_TAG( 'T', 'a', 'g', 'b' ), /*3.2*/

	CYPRIOT = EE_TAG( 'C', 'p', 'r', 't' ),	 /*4.0*/
	LIMBU = EE_TAG( 'L', 'i', 'm', 'b' ),	 /*4.0*/
	LINEAR_B = EE_TAG( 'L', 'i', 'n', 'b' ), /*4.0*/
	OSMANYA = EE_TAG( 'O', 's', 'm', 'a' ),	 /*4.0*/
	SHAVIAN = EE_TAG( 'S', 'h', 'a', 'w' ),	 /*4.0*/
	TAI_LE = EE_TAG( 'T', 'a', 'l', 'e' ),	 /*4.0*/
	UGARITIC = EE_TAG( 'U', 'g', 'a', 'r' ), /*4.0*/

	BUGINESE = EE_TAG( 'B', 'u', 'g', 'i' ),	 /*4.1*/
	COPTIC = EE_TAG( 'C', 'o', 'p', 't' ),		 /*4.1*/
	GLAGOLITIC = EE_TAG( 'G', 'l', 'a', 'g' ),	 /*4.1*/
	KHAROSHTHI = EE_TAG( 'K', 'h', 'a', 'r' ),	 /*4.1*/
	NEW_TAI_LUE = EE_TAG( 'T', 'a', 'l', 'u' ),	 /*4.1*/
	OLD_PERSIAN = EE_TAG( 'X', 'p', 'e', 'o' ),	 /*4.1*/
	SYLOTI_NAGRI = EE_TAG( 'S', 'y', 'l', 'o' ), /*4.1*/
	TIFINAGH = EE_TAG( 'T', 'f', 'n', 'g' ),	 /*4.1*/

	BALINESE = EE_TAG( 'B', 'a', 'l', 'i' ),   /*5.0*/
	CUNEIFORM = EE_TAG( 'X', 's', 'u', 'x' ),  /*5.0*/
	NKO = EE_TAG( 'N', 'k', 'o', 'o' ),		   /*5.0*/
	PHAGS_PA = EE_TAG( 'P', 'h', 'a', 'g' ),   /*5.0*/
	PHOENICIAN = EE_TAG( 'P', 'h', 'n', 'x' ), /*5.0*/

	CARIAN = EE_TAG( 'C', 'a', 'r', 'i' ),	   /*5.1*/
	CHAM = EE_TAG( 'C', 'h', 'a', 'm' ),	   /*5.1*/
	KAYAH_LI = EE_TAG( 'K', 'a', 'l', 'i' ),   /*5.1*/
	LEPCHA = EE_TAG( 'L', 'e', 'p', 'c' ),	   /*5.1*/
	LYCIAN = EE_TAG( 'L', 'y', 'c', 'i' ),	   /*5.1*/
	LYDIAN = EE_TAG( 'L', 'y', 'd', 'i' ),	   /*5.1*/
	OL_CHIKI = EE_TAG( 'O', 'l', 'c', 'k' ),   /*5.1*/
	REJANG = EE_TAG( 'R', 'j', 'n', 'g' ),	   /*5.1*/
	SAURASHTRA = EE_TAG( 'S', 'a', 'u', 'r' ), /*5.1*/
	SUNDANESE = EE_TAG( 'S', 'u', 'n', 'd' ),  /*5.1*/
	VAI = EE_TAG( 'V', 'a', 'i', 'i' ),		   /*5.1*/

	AVESTAN = EE_TAG( 'A', 'v', 's', 't' ),				   /*5.2*/
	BAMUM = EE_TAG( 'B', 'a', 'm', 'u' ),				   /*5.2*/
	EGYPTIAN_HIEROGLYPHS = EE_TAG( 'E', 'g', 'y', 'p' ),   /*5.2*/
	IMPERIAL_ARAMAIC = EE_TAG( 'A', 'r', 'm', 'i' ),	   /*5.2*/
	INSCRIPTIONAL_PAHLAVI = EE_TAG( 'P', 'h', 'l', 'i' ),  /*5.2*/
	INSCRIPTIONAL_PARTHIAN = EE_TAG( 'P', 'r', 't', 'i' ), /*5.2*/
	JAVANESE = EE_TAG( 'J', 'a', 'v', 'a' ),			   /*5.2*/
	KAITHI = EE_TAG( 'K', 't', 'h', 'i' ),				   /*5.2*/
	LISU = EE_TAG( 'L', 'i', 's', 'u' ),				   /*5.2*/
	MEETEI_MAYEK = EE_TAG( 'M', 't', 'e', 'i' ),		   /*5.2*/
	OLD_SOUTH_ARABIAN = EE_TAG( 'S', 'a', 'r', 'b' ),	   /*5.2*/
	OLD_TURKIC = EE_TAG( 'O', 'r', 'k', 'h' ),			   /*5.2*/
	SAMARITAN = EE_TAG( 'S', 'a', 'm', 'r' ),			   /*5.2*/
	TAI_THAM = EE_TAG( 'L', 'a', 'n', 'a' ),			   /*5.2*/
	TAI_VIET = EE_TAG( 'T', 'a', 'v', 't' ),			   /*5.2*/

	BATAK = EE_TAG( 'B', 'a', 't', 'k' ),	/*6.0*/
	BRAHMI = EE_TAG( 'B', 'r', 'a', 'h' ),	/*6.0*/
	MANDAIC = EE_TAG( 'M', 'a', 'n', 'd' ), /*6.0*/

	CHAKMA = EE_TAG( 'C', 'a', 'k', 'm' ),				 /*6.1*/
	MEROITIC_CURSIVE = EE_TAG( 'M', 'e', 'r', 'c' ),	 /*6.1*/
	MEROITIC_HIEROGLYPHS = EE_TAG( 'M', 'e', 'r', 'o' ), /*6.1*/
	MIAO = EE_TAG( 'P', 'l', 'r', 'd' ),				 /*6.1*/
	SHARADA = EE_TAG( 'S', 'h', 'r', 'd' ),				 /*6.1*/
	SORA_SOMPENG = EE_TAG( 'S', 'o', 'r', 'a' ),		 /*6.1*/
	TAKRI = EE_TAG( 'T', 'a', 'k', 'r' ),				 /*6.1*/

	/*
	 * Since: 0.9.30
	 */
	BASSA_VAH = EE_TAG( 'B', 'a', 's', 's' ),		   /*7.0*/
	CAUCASIAN_ALBANIAN = EE_TAG( 'A', 'g', 'h', 'b' ), /*7.0*/
	DUPLOYAN = EE_TAG( 'D', 'u', 'p', 'l' ),		   /*7.0*/
	ELBASAN = EE_TAG( 'E', 'l', 'b', 'a' ),			   /*7.0*/
	GRANTHA = EE_TAG( 'G', 'r', 'a', 'n' ),			   /*7.0*/
	KHOJKI = EE_TAG( 'K', 'h', 'o', 'j' ),			   /*7.0*/
	KHUDAWADI = EE_TAG( 'S', 'i', 'n', 'd' ),		   /*7.0*/
	LINEAR_A = EE_TAG( 'L', 'i', 'n', 'a' ),		   /*7.0*/
	MAHAJANI = EE_TAG( 'M', 'a', 'h', 'j' ),		   /*7.0*/
	MANICHAEAN = EE_TAG( 'M', 'a', 'n', 'i' ),		   /*7.0*/
	MENDE_KIKAKUI = EE_TAG( 'M', 'e', 'n', 'd' ),	   /*7.0*/
	MODI = EE_TAG( 'M', 'o', 'd', 'i' ),			   /*7.0*/
	MRO = EE_TAG( 'M', 'r', 'o', 'o' ),				   /*7.0*/
	NABATAEAN = EE_TAG( 'N', 'b', 'a', 't' ),		   /*7.0*/
	OLD_NORTH_ARABIAN = EE_TAG( 'N', 'a', 'r', 'b' ),  /*7.0*/
	OLD_PERMIC = EE_TAG( 'P', 'e', 'r', 'm' ),		   /*7.0*/
	PAHAWH_HMONG = EE_TAG( 'H', 'm', 'n', 'g' ),	   /*7.0*/
	PALMYRENE = EE_TAG( 'P', 'a', 'l', 'm' ),		   /*7.0*/
	PAU_CIN_HAU = EE_TAG( 'P', 'a', 'u', 'c' ),		   /*7.0*/
	PSALTER_PAHLAVI = EE_TAG( 'P', 'h', 'l', 'p' ),	   /*7.0*/
	SIDDHAM = EE_TAG( 'S', 'i', 'd', 'd' ),			   /*7.0*/
	TIRHUTA = EE_TAG( 'T', 'i', 'r', 'h' ),			   /*7.0*/
	WARANG_CITI = EE_TAG( 'W', 'a', 'r', 'a' ),		   /*7.0*/

	AHOM = EE_TAG( 'A', 'h', 'o', 'm' ),				  /*8.0*/
	ANATOLIAN_HIEROGLYPHS = EE_TAG( 'H', 'l', 'u', 'w' ), /*8.0*/
	HATRAN = EE_TAG( 'H', 'a', 't', 'r' ),				  /*8.0*/
	MULTANI = EE_TAG( 'M', 'u', 'l', 't' ),				  /*8.0*/
	OLD_HUNGARIAN = EE_TAG( 'H', 'u', 'n', 'g' ),		  /*8.0*/
	SIGNWRITING = EE_TAG( 'S', 'g', 'n', 'w' ),			  /*8.0*/

	/*
	 * Since 1.3.0
	 */
	ADLAM = EE_TAG( 'A', 'd', 'l', 'm' ),	  /*9.0*/
	BHAIKSUKI = EE_TAG( 'B', 'h', 'k', 's' ), /*9.0*/
	MARCHEN = EE_TAG( 'M', 'a', 'r', 'c' ),	  /*9.0*/
	OSAGE = EE_TAG( 'O', 's', 'g', 'e' ),	  /*9.0*/
	TANGUT = EE_TAG( 'T', 'a', 'n', 'g' ),	  /*9.0*/
	NEWA = EE_TAG( 'N', 'e', 'w', 'a' ),	  /*9.0*/

	/*
	 * Since 1.6.0
	 */
	MASARAM_GONDI = EE_TAG( 'G', 'o', 'n', 'm' ),	 /*10.0*/
	NUSHU = EE_TAG( 'N', 's', 'h', 'u' ),			 /*10.0*/
	SOYOMBO = EE_TAG( 'S', 'o', 'y', 'o' ),			 /*10.0*/
	ZANABAZAR_SQUARE = EE_TAG( 'Z', 'a', 'n', 'b' ), /*10.0*/

	/*
	 * Since 1.8.0
	 */
	DOGRA = EE_TAG( 'D', 'o', 'g', 'r' ),			/*11.0*/
	GUNJALA_GONDI = EE_TAG( 'G', 'o', 'n', 'g' ),	/*11.0*/
	HANIFI_ROHINGYA = EE_TAG( 'R', 'o', 'h', 'g' ), /*11.0*/
	MAKASAR = EE_TAG( 'M', 'a', 'k', 'a' ),			/*11.0*/
	MEDEFAIDRIN = EE_TAG( 'M', 'e', 'd', 'f' ),		/*11.0*/
	OLD_SOGDIAN = EE_TAG( 'S', 'o', 'g', 'o' ),		/*11.0*/
	SOGDIAN = EE_TAG( 'S', 'o', 'g', 'd' ),			/*11.0*/

	/*
	 * Since 2.4.0
	 */
	ELYMAIC = EE_TAG( 'E', 'l', 'y', 'm' ),				   /*12.0*/
	NANDINAGARI = EE_TAG( 'N', 'a', 'n', 'd' ),			   /*12.0*/
	NYIAKENG_PUACHUE_HMONG = EE_TAG( 'H', 'm', 'n', 'p' ), /*12.0*/
	WANCHO = EE_TAG( 'W', 'c', 'h', 'o' ),				   /*12.0*/

	/*
	 * Since 2.6.7
	 */
	CHORASMIAN = EE_TAG( 'C', 'h', 'r', 's' ),			/*13.0*/
	DIVES_AKURU = EE_TAG( 'D', 'i', 'a', 'k' ),			/*13.0*/
	KHITAN_SMALL_SCRIPT = EE_TAG( 'K', 'i', 't', 's' ), /*13.0*/
	YEZIDI = EE_TAG( 'Y', 'e', 'z', 'i' ),				/*13.0*/

	/*
	 * Since 3.0.0
	 */
	CYPRO_MINOAN = EE_TAG( 'C', 'p', 'm', 'n' ), /*14.0*/
	OLD_UYGHUR = EE_TAG( 'O', 'u', 'g', 'r' ),	 /*14.0*/
	TANGSA = EE_TAG( 'T', 'n', 's', 'a' ),		 /*14.0*/
	TOTO = EE_TAG( 'T', 'o', 't', 'o' ),		 /*14.0*/
	VITHKUQI = EE_TAG( 'V', 'i', 't', 'h' ),	 /*14.0*/

	/*
	 * Since 3.4.0
	 */
	MATH = EE_TAG( 'Z', 'm', 't', 'h' ),

	/*
	 * Since 5.2.0
	 */
	KAWI = EE_TAG( 'K', 'a', 'w', 'i' ),		/*15.0*/
	NAG_MUNDARI = EE_TAG( 'N', 'a', 'g', 'm' ), /*15.0*/

	/* No script set. */
	INVALID = EE_TAG_NONE,
};

struct ShapedGlyph {
	FontTrueType* font{ nullptr };
	Uint32 glyphIndex{ 0 };
	Uint32 stringIndex{ 0 };
	Vector2f position;
	Vector2f advance;
	LangScript script;
	TextDirection direction;
};

} // namespace EE::Graphics
