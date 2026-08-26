#include "settingspage.hpp"

namespace ecode {

bool SettingsPage::parseNonNegativeSettingsTime( const std::string& text, Time& time ) {
	auto parts = String::split( text, " " );
	bool hasValue = false;
	for ( auto& part : parts ) {
		String::trimInPlace( part );
		if ( part.empty() )
			continue;
		size_t suffixLength =
			String::endsWith( part, "ms" )
				? 2
				: ( String::endsWith( part, "s" ) || String::endsWith( part, "m" ) ? 1 : 0 );
		std::string number = part.substr( 0, part.size() - suffixLength );
		double value;
		if ( number.empty() || !String::fromString( value, number ) || value < 0 )
			return false;
		hasValue = true;
	}
	if ( !hasValue )
		return false;
	time = Time::fromString( text );
	return true;
}

SettingsPage::SettingsPage( SettingsModel& model, std::shared_ptr<SettingsDocument> document,
							std::string category, std::string idPrefix ) :
	mModel( model ),
	mDocument( std::move( document ) ),
	mCategory( std::move( category ) ),
	mIdPrefix( std::move( idPrefix ) ) {}

bool SettingsPage::addGroup( String name ) {
	return mModel.addGroup( { mCategory, std::move( name ), mModel.settings().size() } );
}

bool SettingsPage::addBool( std::string id, std::string jsonPointer, String name,
							String description, bool defaultValue, String group ) {
	if ( !mDocument )
		return false;
	auto document = mDocument;
	return mModel.addSetting( { { mIdPrefix + "." + id, mCategory, std::move( name ),
								  std::move( description ), std::move( group ) },
								BoolSetting{ [document, jsonPointer, defaultValue] {
												return document->get<bool>( jsonPointer,
																			defaultValue );
											},
											 [document, jsonPointer]( bool value ) {
												 document->set( jsonPointer, value );
											 } } } );
}

bool SettingsPage::addChoice( std::string id, std::string jsonPointer, String name,
							  String description, std::vector<String> choices,
							  std::vector<nlohmann::json> values, nlohmann::json defaultValue,
							  std::vector<String> choiceDescriptions, String group ) {
	if ( !mDocument || choices.empty() || choices.size() != values.size() )
		return false;
	auto document = mDocument;
	auto mappedValues = std::make_shared<const std::vector<nlohmann::json>>( std::move( values ) );
	return mModel.addSetting(
		{ { mIdPrefix + "." + id, mCategory, std::move( name ), std::move( description ),
			std::move( group ) },
		  ChoiceSetting{
			  std::move( choices ), std::move( choiceDescriptions ),
			  [document, jsonPointer, mappedValues, defaultValue = std::move( defaultValue )] {
				  const auto current = document->getValue( jsonPointer, defaultValue );
				  auto found = std::find( mappedValues->begin(), mappedValues->end(), current );
				  return found == mappedValues->end()
							 ? size_t{ 0 }
							 : static_cast<size_t>( std::distance( mappedValues->begin(), found ) );
			  },
			  [document, jsonPointer, mappedValues]( size_t selected ) {
				  if ( selected < mappedValues->size() )
					  document->setValue( jsonPointer, ( *mappedValues )[selected] );
			  } } } );
}

bool SettingsPage::addEditableChoice( std::string id, std::string jsonPointer, String name,
									  String description, std::vector<String> choices,
									  std::string defaultValue,
									  std::function<bool( const std::string& )> validate,
									  String group ) {
	if ( !mDocument )
		return false;
	auto document = mDocument;
	return mModel.addSetting(
		{ { mIdPrefix + "." + id, mCategory, std::move( name ), std::move( description ),
			std::move( group ) },
		  EditableChoiceSetting{
			  std::move( choices ),
			  [document, jsonPointer, defaultValue = std::move( defaultValue )] {
				  return String::fromUtf8(
					  document->get<std::string>( jsonPointer, defaultValue ) );
			  },
			  [document, jsonPointer, validate = std::move( validate )]( const String& value ) {
				  const std::string text = value.toUtf8();
				  if ( validate && !validate( text ) )
					  return false;
				  document->set( jsonPointer, text );
				  return true;
			  } } } );
}

bool SettingsPage::addInteger( std::string id, std::string jsonPointer, String name,
							   String description, int min, int max, int defaultValue,
							   String group ) {
	if ( !mDocument || min > max )
		return false;
	auto document = mDocument;
	return mModel.addSetting( { { mIdPrefix + "." + id, mCategory, std::move( name ),
								  std::move( description ), std::move( group ) },
								IntegerSetting{ min, max,
												[document, jsonPointer, defaultValue] {
													return document->get<int>( jsonPointer,
																			   defaultValue );
												},
												[document, jsonPointer]( int value ) {
													document->set( jsonPointer, value );
												} } } );
}

bool SettingsPage::addText( std::string id, std::string jsonPointer, String name,
							String description, std::string defaultValue,
							std::function<bool( const std::string& )> validate,
							bool commitOnFocusLoss, String group ) {
	if ( !mDocument )
		return false;
	auto document = mDocument;
	return mModel.addSetting(
		{ { mIdPrefix + "." + id, mCategory, std::move( name ), std::move( description ),
			std::move( group ) },
		  TextSetting{ [document, jsonPointer, defaultValue = std::move( defaultValue )] {
						  return document->get<std::string>( jsonPointer, defaultValue );
					  },
					   [document, jsonPointer,
						validate = std::move( validate )]( const std::string& value ) {
						   if ( validate && !validate( value ) )
							   return false;
						   document->set( jsonPointer, value );
						   return true;
					   },
					   commitOnFocusLoss } } );
}

bool SettingsPage::addSecret( std::string id, std::string jsonPointer, String name,
							  String description, std::string defaultValue, String group,
							  std::string legacyJsonPointer ) {
	if ( !mDocument )
		return false;
	auto document = mDocument;
	return mModel.addSetting(
		{ { mIdPrefix + "." + id, mCategory, std::move( name ), std::move( description ),
			std::move( group ) },
		  TextSetting{
			  [document, jsonPointer, legacyJsonPointer, defaultValue = std::move( defaultValue )] {
				  if ( !legacyJsonPointer.empty() && !document->contains( jsonPointer ) )
					  return document->get<std::string>( legacyJsonPointer, defaultValue );
				  return document->get<std::string>( jsonPointer, defaultValue );
			  },
			  [document, jsonPointer]( const std::string& value ) {
				  document->set( jsonPointer, value );
				  return true;
			  },
			  true, true } } );
}

bool SettingsPage::addJsonObject( std::string id, std::string jsonPointer, String name,
								  String description, nlohmann::json defaultValue, String group ) {
	if ( !mDocument || !defaultValue.is_object() )
		return false;
	auto document = mDocument;
	return mModel.addSetting(
		{ { mIdPrefix + "." + id, mCategory, std::move( name ), std::move( description ),
			std::move( group ) },
		  TextSetting{ [document, jsonPointer, defaultValue = std::move( defaultValue )] {
						  return document->getValue( jsonPointer, defaultValue ).dump();
					  },
					   [document, jsonPointer]( const std::string& value ) {
						   auto parsed = nlohmann::json::parse( value, nullptr, false );
						   if ( !parsed.is_object() )
							   return false;
						   document->setValue( jsonPointer, std::move( parsed ) );
						   return true;
					   },
					   false } } );
}

bool SettingsPage::addStringList( std::string id, std::string jsonPointer, String name,
								  String description, std::vector<std::string> defaultValue,
								  String group ) {
	if ( !mDocument )
		return false;
	auto document = mDocument;
	return mModel.addSetting(
		{ { mIdPrefix + "." + id, mCategory, std::move( name ), std::move( description ),
			std::move( group ) },
		  TextSetting{ [document, jsonPointer, defaultValue = std::move( defaultValue )] {
						  return String::join(
							  document->get<std::vector<std::string>>( jsonPointer, defaultValue ),
							  ',' );
					  },
					   [document, jsonPointer]( const std::string& value ) {
						   std::vector<std::string> values;
						   for ( auto& item : String::split( value, "," ) ) {
							   String::trimInPlace( item );
							   if ( !item.empty() )
								   values.emplace_back( std::move( item ) );
						   }
						   document->set( jsonPointer, std::move( values ) );
						   return true;
					   },
					   true } } );
}

bool SettingsPage::addFloat( std::string id, std::string jsonPointer, String name,
							 String description, double min, double max, double step,
							 double defaultValue, String group ) {
	if ( !mDocument || min > max || step <= 0 )
		return false;
	auto document = mDocument;
	return mModel.addSetting( { { mIdPrefix + "." + id, mCategory, std::move( name ),
								  std::move( description ), std::move( group ) },
								FloatSetting{ min, max, step,
											  [document, jsonPointer, defaultValue] {
												  return document->get<double>( jsonPointer,
																				defaultValue );
											  },
											  [document, jsonPointer]( double value ) {
												  document->set( jsonPointer, value );
											  } } } );
}

bool SettingsPage::addAction( std::string id, String name, String description, String buttonText,
							  std::function<void()> action, String group ) {
	if ( !action )
		return false;
	return mModel.addSetting( { { mIdPrefix + "." + id, mCategory, std::move( name ),
								  std::move( description ), std::move( group ) },
								ActionSetting{ std::move( buttonText ), std::move( action ) } } );
}

} // namespace ecode
