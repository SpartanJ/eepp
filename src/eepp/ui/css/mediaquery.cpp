#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/window/displaymanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

using namespace EE::Window;

namespace EE { namespace UI { namespace CSS {

#define MediaOrientationStrings "portrait;landscape"

#define MediaFeatureStrings                                                                    \
	"none;width;min-width;max-width;height;min-height;max-height;device-width;min-device-"     \
	"width;max-device-width;device-height;min-device-height;max-device-height;orientation;"    \
	"aspect-ratio;min-aspect-ratio;max-aspect-ratio;device-aspect-ratio;min-device-aspect-"    \
	"ratio;max-device-aspect-ratio;color;min-color;max-color;color-index;min-color-index;max-" \
	"color-index;monochrome;min-monochrome;max-monochrome;resolution;min-resolution;max-"      \
	"resolution;pixel-density;min-pixel-density;max-pixel-density;prefers-color-scheme"

#define MediaTypeStrings "none;all;screen;print;braille;embossed;handheld;projection;speech;tty;tv"

MediaQuery::MediaQuery() {
	mMediaType = media_type_all;
	mNot = false;
}

MediaQuery::MediaQuery( const MediaQuery& val ) {
	mNot = val.mNot;
	mExpressions = val.mExpressions;
	mMediaType = val.mMediaType;
}

MediaQuery::ptr MediaQuery::parse( const std::string& str ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	int currentDisplayIndex = Engine::instance()->getCurrentWindow()->getCurrentDisplayIndex();
	Display* currentDisplay = displayManager->getDisplayIndex( currentDisplayIndex );
	Float dpi = currentDisplay->getDPI();
	MediaQuery::ptr query = std::make_shared<MediaQuery>();

	std::vector<std::string> tokens = String::split( str, " \t\r\n", "", "(" );

	for ( auto& tok : tokens ) {
		if ( tok == "not" ) {
			query->mNot = true;
		} else if ( tok.at( 0 ) == '(' ) {
			tok.erase( 0, 1 );

			if ( tok.at( tok.length() - 1 ) == ')' ) {
				tok.erase( tok.length() - 1, 1 );
			}

			MediaQueryExpression expr;
			std::vector<std::string> exprTokens = String::split( tok, ':' );
			if ( !exprTokens.empty() ) {
				String::trimInPlace( exprTokens[0] );

				expr.feature = (MediaFeature)String::valueIndex( exprTokens[0], MediaFeatureStrings,
																 media_feature_none );

				if ( expr.feature != media_feature_none ) {
					if ( exprTokens.size() == 1 ) {
						expr.checkAsBool = true;
					} else {
						String::trimInPlace( exprTokens[1] );
						expr.checkAsBool = false;

						if ( expr.feature == media_feature_orientation ) {
							expr.val = String::valueIndex( exprTokens[1], MediaOrientationStrings,
														   media_orientation_landscape );
						} else {
							std::string::size_type slash_pos = exprTokens[1].find( '/' );
							if ( slash_pos != std::string::npos ) {
								std::string val1 = exprTokens[1].substr( 0, slash_pos );
								std::string val2 = exprTokens[1].substr( slash_pos + 1 );
								String::trimInPlace( val1 );
								String::trimInPlace( val2 );

								int intVal1, intVal2;
								float fVal1, fVal2;

								if ( String::fromString( intVal1, val1 ) &&
									 String::fromString( intVal2, val2 ) ) {
									expr.val = intVal1;
									expr.val2 = intVal2;
								}

								if ( String::fromString( fVal1, val1 ) &&
									 String::fromString( fVal2, val2 ) ) {
									expr.fval = fVal1;
									expr.fval2 = fVal2;
								}
							} else {
								StyleSheetLength length =
									StyleSheetLength::fromString( exprTokens[1] );
								expr.valStr = String::toLower( exprTokens[1] );

								if ( length.getUnit() == StyleSheetLength::Unit::Dpcm ||
									 length.getUnit() == StyleSheetLength::Unit::Dpi ) {
									expr.val = (int)( length.getValue() * 2.54 );
								} else {
									expr.val = (int)length.asPixels( 0, Sizef::Zero, dpi );
								}

								expr.fval = length.getValue();
							}
						}
					}
					query->mExpressions.push_back( expr );
				}
			}
		} else {
			query->mMediaType =
				(MediaType)String::valueIndex( tok, MediaTypeStrings, media_type_all );
		}
	}

	return query;
}

bool MediaQuery::check( const MediaFeatures& features ) const {
	bool res = false;

	if ( mMediaType == media_type_all || mMediaType == features.type ) {
		res = true;

		for ( auto& expr : mExpressions ) {
			if ( !expr.check( features ) ) {
				res = false;
				break;
			}
		}
	}

	if ( mNot ) {
		res = !res;
	}

	return res;
}

MediaQueryList::ptr MediaQueryList::parse( const std::string& str ) {
	MediaQueryList::ptr list = std::make_shared<MediaQueryList>();

	std::vector<std::string> tokens = String::split( str, "," );

	for ( auto& tok : tokens ) {
		String::trimInPlace( tok );
		String::toLowerInPlace( tok );

		MediaQuery::ptr query = MediaQuery::parse( tok );

		if ( query ) {
			list->mQueries.push_back( query );
		}
	}

	if ( list->mQueries.empty() ) {
		list = 0;
	}

	list->mQueryStr = str;

	return list;
}

bool MediaQueryList::applyMediaFeatures( const MediaFeatures& features ) {
	bool apply = false;

	for ( auto& query : mQueries ) {
		if ( query->check( features ) ) {
			apply = true;
			break;
		}
	}

	bool ret = ( apply != mUsed );
	mUsed = apply;
	return ret;
}

const Uint32& MediaQueryList::getMarker() const {
	return mMarker;
}

void MediaQueryList::setMarker( const Uint32& marker ) {
	mMarker = marker;
}

MediaQueryExpression::MediaQueryExpression() {
	checkAsBool = false;
	feature = media_feature_none;
	val = 0;
	val2 = 0;
}

bool MediaQueryExpression::check( const MediaFeatures& features ) const {
	switch ( feature ) {
		case media_feature_width:
			if ( checkAsBool ) {
				return ( features.width != 0 );
			} else if ( features.width == val ) {
				return true;
			}
			break;
		case media_feature_min_width:
			if ( features.width >= val ) {
				return true;
			}
			break;
		case media_feature_max_width:
			if ( features.width <= val ) {
				return true;
			}
			break;
		case media_feature_height:
			if ( checkAsBool ) {
				return ( features.height != 0 );
			} else if ( features.height == val ) {
				return true;
			}
			break;
		case media_feature_min_height:
			if ( features.height >= val ) {
				return true;
			}
			break;
		case media_feature_max_height:
			if ( features.height <= val ) {
				return true;
			}
			break;
		case media_feature_device_width:
			if ( checkAsBool ) {
				return ( features.deviceWidth != 0 );
			} else if ( features.deviceWidth == val ) {
				return true;
			}
			break;
		case media_feature_min_device_width:
			if ( features.deviceWidth >= val ) {
				return true;
			}
			break;
		case media_feature_max_device_width:
			if ( features.deviceWidth <= val ) {
				return true;
			}
			break;
		case media_feature_device_height:
			if ( checkAsBool ) {
				return ( features.deviceHeight != 0 );
			} else if ( features.deviceHeight == val ) {
				return true;
			}
			break;
		case media_feature_min_device_height:
			if ( features.deviceHeight >= val ) {
				return true;
			}
			break;
		case media_feature_max_device_height:
			if ( features.deviceHeight <= val ) {
				return true;
			}
			break;

		case media_feature_orientation:
			if ( features.height >= features.width ) {
				if ( val == media_orientation_portrait ) {
					return true;
				}
			} else {
				if ( val == media_orientation_landscape ) {
					return true;
				}
			}
			break;
		case media_feature_aspect_ratio:
			if ( features.height && val2 ) {
				int ratio_this = Math::round( (double)val / (double)val2 * 100 );
				int ratio_feat =
					Math::round( (double)features.width / (double)features.height * 100.0 );
				if ( ratio_this == ratio_feat ) {
					return true;
				}
			}
			break;
		case media_feature_min_aspect_ratio:
			if ( features.height && val2 ) {
				int ratio_this = Math::round( (double)val / (double)val2 * 100 );
				int ratio_feat =
					Math::round( (double)features.width / (double)features.height * 100.0 );
				if ( ratio_feat >= ratio_this ) {
					return true;
				}
			}
			break;
		case media_feature_max_aspect_ratio:
			if ( features.height && val2 ) {
				int ratio_this = Math::round( (double)val / (double)val2 * 100 );
				int ratio_feat =
					Math::round( (double)features.width / (double)features.height * 100.0 );
				if ( ratio_feat <= ratio_this ) {
					return true;
				}
			}
			break;
		case media_feature_device_aspect_ratio:
			if ( features.deviceHeight && val2 ) {
				int ratio_this = Math::round( (double)val / (double)val2 * 100 );
				int ratio_feat = Math::round( (double)features.deviceWidth /
											  (double)features.deviceHeight * 100.0 );
				if ( ratio_feat == ratio_this ) {
					return true;
				}
			}
			break;
		case media_feature_min_device_aspect_ratio:
			if ( features.deviceHeight && val2 ) {
				int ratio_this = Math::round( (double)val / (double)val2 * 100 );
				int ratio_feat = Math::round( (double)features.deviceWidth /
											  (double)features.deviceHeight * 100.0 );
				if ( ratio_feat >= ratio_this ) {
					return true;
				}
			}
			break;
		case media_feature_max_device_aspect_ratio:
			if ( features.deviceHeight && val2 ) {
				int ratio_this = Math::round( (double)val / (double)val2 * 100 );
				int ratio_feat = Math::round( (double)features.deviceWidth /
											  (double)features.deviceHeight * 100.0 );
				if ( ratio_feat <= ratio_this ) {
					return true;
				}
			}
			break;
		case media_feature_color:
			if ( checkAsBool ) {
				return ( features.color != 0 );
			} else if ( features.color == val ) {
				return true;
			}
			break;
		case media_feature_min_color:
			if ( features.color >= val ) {
				return true;
			}
			break;
		case media_feature_max_color:
			if ( features.color <= val ) {
				return true;
			}
			break;
		case media_feature_color_index:
			if ( checkAsBool ) {
				return ( features.colorIndex != 0 );
			} else if ( features.colorIndex == val ) {
				return true;
			}
			break;
		case media_feature_min_color_index:
			if ( features.colorIndex >= val ) {
				return true;
			}
			break;
		case media_feature_max_color_index:
			if ( features.colorIndex <= val ) {
				return true;
			}
			break;
		case media_feature_monochrome:
			if ( checkAsBool ) {
				return ( features.monochrome != 0 );
			} else if ( features.monochrome == val ) {
				return true;
			}
			break;
		case media_feature_min_monochrome:
			if ( features.monochrome >= val ) {
				return true;
			}
			break;
		case media_feature_max_monochrome:
			if ( features.monochrome <= val ) {
				return true;
			}
			break;
		case media_feature_resolution:
			if ( features.resolution == val ) {
				return true;
			}
			break;
		case media_feature_min_resolution:
			if ( features.resolution >= val ) {
				return true;
			}
			break;
		case media_feature_max_resolution:
			if ( features.resolution <= val ) {
				return true;
			}
			break;
		case media_feature_pixel_density:
			if ( features.pixelDensity == val )
				return true;
			break;
		case media_feature_min_pixel_density:
			if ( features.pixelDensity >= fval )
				return true;
			break;
		case media_feature_max_pixel_density:
			if ( features.pixelDensity <= fval )
				return true;
			break;
		case media_feature_prefers_color_scheme:
			if ( features.prefersColorScheme == valStr )
				return true;
			break;
		default:
			return false;
	}

	return false;
}

MediaQueryList::MediaQueryList( const MediaQueryList& val ) {
	mUsed = val.mUsed;
	mQueries = val.mQueries;
}

MediaQueryList::MediaQueryList() {
	mUsed = false;
}

bool MediaQueryList::isUsed() const {
	return mUsed;
}

}}} // namespace EE::UI::CSS
