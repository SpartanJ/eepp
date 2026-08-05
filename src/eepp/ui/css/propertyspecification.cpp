#include <eepp/core/core.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/propertyspecification.hpp>

namespace EE { namespace UI { namespace CSS {

SINGLETON_DECLARE_IMPLEMENTATION( PropertySpecification )

PropertySpecification::~PropertySpecification() {}

PropertyDefinition& PropertySpecification::registerProperty( PropertyId id,
															 const std::string& propertyName,
															 const std::string& defaultValue,
															 bool inherited ) {
	if ( !mPropertyIds.addBuiltin( id, propertyName ) ) {
		// A built-in ID already bound to another name is a programming error.
		Log::error( "PropertySpecification: failed to register built-in property \"%s\".",
					propertyName.c_str() );
		eeASSERT( false );
	}

	PropertyDefinition* propDef =
		new PropertyDefinition( id, propertyName, defaultValue, inherited );

	const size_t idx = static_cast<size_t>( id );
	if ( mPropertiesById.size() <= idx )
		mPropertiesById.resize( idx + 1 );
	mPropertiesById[idx] = std::shared_ptr<PropertyDefinition>( propDef );

	if ( inherited )
		mInheritableProperties.insert( id );

	for ( auto& sep : { "-", "_" } ) {
		if ( propDef->getName().find( sep ) != std::string::npos ) {
			std::string alias( propDef->getName() );
			String::replaceAll( alias, sep, "" );
			propDef->addAlias( alias );
		}
	}

	return *propDef;
}

PropertyDefinition* PropertySpecification::registerProperty( const std::string& propertyName,
															 const std::string& defaultValue,
															 bool inherited ) {
	PropertyId id = mPropertyIds.getId( propertyName );
	PropertyDefinition* existing = ( id != PropertyId::Invalid )
									   ? const_cast<PropertyDefinition*>( getProperty( id ) )
									   : nullptr;

	if ( nullptr != existing && !String::startsWith( propertyName, "-" ) ) {
		Log::warning( "Property \"%s\" already registered.", propertyName.c_str() );
		return existing;
	}

	if ( id == PropertyId::Invalid ) {
		id = mPropertyIds.getOrCreateId( propertyName );
		if ( id == PropertyId::Invalid ) {
			Log::error( "PropertySpecification: could not allocate an ID for property \"%s\".",
						propertyName.c_str() );
			return nullptr;
		}
	}

	PropertyDefinition* propDef =
		new PropertyDefinition( id, propertyName, defaultValue, inherited );

	const size_t idx = static_cast<size_t>( id );
	if ( mPropertiesById.size() <= idx )
		mPropertiesById.resize( idx + 1 );
	mPropertiesById[idx] = std::shared_ptr<PropertyDefinition>( propDef );

	if ( inherited )
		mInheritableProperties.insert( id );
	else
		mInheritableProperties.erase( id );

	for ( auto& sep : { "-", "_" } ) {
		if ( propDef->getName().find( sep ) != std::string::npos ) {
			std::string alias( propDef->getName() );
			String::replaceAll( alias, sep, "" );
			propDef->addAlias( alias );
		}
	}

	return propDef;
}

const PropertyIdSet& PropertySpecification::getInheritableProperties() const {
	return mInheritableProperties;
}

const PropertyDefinition* PropertySpecification::getProperty( const PropertyId& id ) const {
	const size_t idx = static_cast<size_t>( id );
	if ( 0 == idx || idx >= mPropertiesById.size() )
		return nullptr;
	return mPropertiesById[idx].get();
}

const PropertyDefinition* PropertySpecification::getProperty( const char* name ) const {
	return getProperty( mPropertyIds.getId( name ) );
}

const PropertyDefinition* PropertySpecification::getProperty( std::string_view name ) const {
	return getProperty( mPropertyIds.getId( name ) );
}

const PropertyDefinition* PropertySpecification::getProperty( const std::string& name ) const {
	return getProperty( mPropertyIds.getId( name ) );
}

ShorthandDefinition&
PropertySpecification::registerShorthand( ShorthandId id, const std::string& name,
										  const std::vector<std::string>& properties,
										  const std::string& shorthandParserName ) {
	if ( !mShorthandIds.addBuiltin( id, name ) ) {
		// A built-in ID already bound to another name is a programming error.
		Log::error( "PropertySpecification: failed to register built-in shorthand \"%s\".",
					name.c_str() );
		eeASSERT( false );
	}

	ShorthandDefinition* shorthandDef =
		new ShorthandDefinition( id, name, properties, shorthandParserName );

	const size_t idx = static_cast<size_t>( id );
	if ( mShorthandsById.size() <= idx )
		mShorthandsById.resize( idx + 1 );
	mShorthandsById[idx] = std::shared_ptr<ShorthandDefinition>( shorthandDef );

	return *shorthandDef;
}

ShorthandDefinition*
PropertySpecification::registerShorthand( const std::string& name,
										  const std::vector<std::string>& properties,
										  const std::string& shorthandParserName ) {
	ShorthandId id = mShorthandIds.getId( name );
	ShorthandDefinition* existing = ( id != ShorthandId::Invalid )
										? const_cast<ShorthandDefinition*>( getShorthand( id ) )
										: nullptr;
	if ( nullptr != existing ) {
		Log::warning( "Shorthand %s already registered.", name.c_str() );
		return existing;
	}

	id = mShorthandIds.getOrCreateId( name );
	if ( id == ShorthandId::Invalid ) {
		Log::error( "PropertySpecification: could not allocate an ID for shorthand \"%s\".",
					name.c_str() );
		return nullptr;
	}

	ShorthandDefinition* shorthandDef =
		new ShorthandDefinition( id, name, properties, shorthandParserName );

	const size_t idx = static_cast<size_t>( id );
	if ( mShorthandsById.size() <= idx )
		mShorthandsById.resize( idx + 1 );
	mShorthandsById[idx] = std::shared_ptr<ShorthandDefinition>( shorthandDef );

	return shorthandDef;
}

const ShorthandDefinition* PropertySpecification::getShorthand( const ShorthandId& id ) const {
	const size_t idx = static_cast<size_t>( id );
	if ( 0 == idx || idx >= mShorthandsById.size() )
		return nullptr;
	return mShorthandsById[idx].get();
}

const ShorthandDefinition* PropertySpecification::getShorthand( const std::string& name ) const {
	return getShorthand( mShorthandIds.getId( name ) );
}

bool PropertySpecification::isShorthand( const std::string& name ) const {
	return getShorthand( name ) != nullptr;
}

const PropertyDefinition*
PropertySpecification::addPropertyAlias( const std::string& alias,
										 const PropertyDefinition* propDef ) {
	mPropertyIds.addAlias( alias, propDef->getPropertyId() );
	return propDef;
}

const ShorthandDefinition*
PropertySpecification::addShorthandAlias( const std::string& alias,
										  const ShorthandDefinition* shorthandDef ) {
	mShorthandIds.addAlias( alias, shorthandDef->getShorthandId() );
	return shorthandDef;
}

void PropertySpecification::finalizeBuiltins() {
	const bool propsOk = mPropertyIds.finalizeBuiltins();
	const bool shorthandsOk = mShorthandIds.finalizeBuiltins();
	if ( !propsOk || !shorthandsOk ) {
		Log::error( "PropertySpecification: built-in registration validation failed." );
		eeASSERT( false );
	}
}

}}} // namespace EE::UI::CSS
