#ifndef EE_UI_UIVALUEVALIDATION_HPP
#define EE_UI_UIVALUEVALIDATION_HPP

#include <eepp/config.hpp>
#include <eepp/core/observablevalue.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace EE { namespace UI {

/** Numeric error codes reserved by eepp's built-in value converters. */
enum class UIValueValidationError : Uint32 {
	ConversionFailed = 1,
};

/**
 * @brief Machine-readable result of converting or accepting a widget value.
 *
 * A numeric code is the normal way to identify an error. Applications can map that code, together
 * with the binding or form context, to localized UI text. debugMessage is optional diagnostic
 * information for logs, tests, and inspection; it should not be shown to users implicitly.
 *
 * Codes are owned by the subsystem or application defining the validation rule. Apart from values
 * in UIValueValidationError, eepp does not require codes to be globally unique or stable for
 * serialization. Code 0 is valid: the optional itself represents the absence of a code.
 */
struct UIValueValidationResult {
	using Code = Uint32;

	bool valid{ true };
	std::optional<Code> code;
	std::optional<std::string> debugMessage;

	UIValueValidationResult() = default;

	static UIValueValidationResult success() { return {}; }

	static UIValueValidationResult error( Code code ) {
		UIValueValidationResult result;
		result.valid = false;
		result.code = code;
		return result;
	}

	static UIValueValidationResult error( Code code, std::string debugMessage ) {
		UIValueValidationResult result = error( code );
		result.debugMessage = std::move( debugMessage );
		return result;
	}

	static UIValueValidationResult error( std::string debugMessage ) {
		UIValueValidationResult result;
		result.valid = false;
		result.debugMessage = std::move( debugMessage );
		return result;
	}

	explicit operator bool() const { return valid; }

	bool operator==( const UIValueValidationResult& other ) const {
		return valid == other.valid && code == other.code && debugMessage == other.debugMessage;
	}

	bool operator!=( const UIValueValidationResult& other ) const { return !( *this == other ); }
};

/**
 * @brief A converted value together with its failure information.
 *
 * Successful results always contain a value. Failures contain no value and preserve the numeric
 * code and optional diagnostic produced by a converter.
 */
template <typename T> struct UIValueResult {
	std::optional<T> value;
	UIValueValidationResult validation;

	UIValueResult() = default;
	UIValueResult( T value ) : value( std::move( value ) ) {}

	static UIValueResult success( T value ) { return UIValueResult( std::move( value ) ); }

	static UIValueResult error( UIValueValidationResult validation ) {
		UIValueResult result;
		result.validation = std::move( validation );
		return result;
	}

	static UIValueResult error( UIValueValidationResult::Code code ) {
		return error( UIValueValidationResult::error( code ) );
	}

	static UIValueResult error( UIValueValidationResult::Code code, std::string debugMessage ) {
		return error( UIValueValidationResult::error( code, std::move( debugMessage ) ) );
	}

	explicit operator bool() const { return validation.valid && value.has_value(); }
};

/**
 * @brief Observable current validation result shared by UI value binding implementations.
 *
 * Reading validity never allocates. Observer storage is created lazily by observe(), keeping the
 * ordinary unobserved binding path inexpensive. Notifications are synchronous and use the same
 * snapshot semantics as ObservableValue.
 */
class UIValueValidationState {
  public:
	using Callback = std::function<void( const UIValueValidationResult& result )>;
	using Connection = ObservableValue<UIValueValidationResult>::Connection;

	UIValueValidationState() = default;
	UIValueValidationState( const UIValueValidationState& ) = delete;
	UIValueValidationState& operator=( const UIValueValidationState& ) = delete;
	UIValueValidationState( UIValueValidationState&& ) = delete;
	UIValueValidationState& operator=( UIValueValidationState&& ) = delete;

	bool isValid() const { return mResult.valid; }
	const UIValueValidationResult& result() const { return mResult; }
	const std::optional<UIValueValidationResult::Code>& code() const { return mResult.code; }
	const std::optional<std::string>& debugMessage() const { return mResult.debugMessage; }

	/** Observes later result changes. The current result is available through result(). */
	Connection observe( Callback callback ) {
		if ( !mObservable )
			mObservable = std::make_unique<ObservableValue<UIValueValidationResult>>( mResult );
		return mObservable->observe( std::move( callback ) );
	}

	/** Updates the current result and notifies observers only when it actually changed. */
	void set( UIValueValidationResult result ) {
		if ( mResult == result )
			return;
		mResult = std::move( result );
		if ( mObservable )
			mObservable->set( mResult );
	}

	void clear() { set( UIValueValidationResult::success() ); }

  private:
	UIValueValidationResult mResult;
	std::unique_ptr<ObservableValue<UIValueValidationResult>> mObservable;
};

}} // namespace EE::UI

#endif
