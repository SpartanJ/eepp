#include <eepp/ui/uidatabind.hpp>

namespace EE { namespace UI {

template <typename T> class UIProperty {
  public:
	UIProperty() {}

	UIProperty( T defaultValue, UIWidget* widget,
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mValue( std::move( defaultValue ) ),
		mBindedData( &mValue, widget, converter, valueKey, eventType ) {}

	UIProperty( T defaultValue, const std::unordered_set<UIWidget*>& widgets = {},
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mValue( std::move( defaultValue ) ),
		mBindedData( &mValue, widgets, converter, valueKey, eventType ) {}

	UIProperty( const std::unordered_set<UIWidget*>& widgets = {},
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mBindedData( &mValue, widgets, converter, valueKey, eventType ) {}

	UIProperty( UIWidget* widget,
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mBindedData( &mValue, widget, converter, valueKey, eventType ) {}

	void operator=( const T& newVal ) { mBindedData.set( newVal ); }

	void operator=( T&& newVal ) { mBindedData.set( std::move( newVal ) ); }

	const T& value() const { return mBindedData.get(); }

	const UIDataBind<T>& databind() const { return mBindedData; }

	UIProperty& connect( UIWidget* widget ) {
		mBindedData.bind( widget );
		return *this;
	}

	UIProperty& disconnect( UIWidget* widget ) {
		mBindedData.unbind( widget );
		return *this;
	}

	const T& operator*() const noexcept { return value(); }

	const T* operator->() const noexcept { return &value(); }

	operator const T&() const noexcept { return value(); }

	UIProperty& changed( const std::function<void( const T& newVal )>& fn ) {
		mBindedData.onValueChangeCb = fn;
		return *this;
	}

	UIProperty& changed( std::function<void( const T& newVal )>&& fn ) {
		mBindedData.onValueChangeCb = std::move( fn );
		return *this;
	}

  protected:
	T mValue{};
	UIDataBind<T> mBindedData;
};

}} // namespace EE::UI
