#ifndef EE_UI_UIBINDINGGROUP_HPP
#define EE_UI_UIBINDINGGROUP_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/ui/databinding/uidatabind.hpp>
#include <eepp/ui/databinding/uiproperty.hpp>
#include <eepp/ui/databinding/uivaluebinding.hpp>
#include <memory>

namespace EE { namespace UI {

/**
 * @brief Owns heterogeneous bindings and exposes aggregate form state.
 *
 * Dirty state compares live values with explicit per-binding baselines. The group owns bindings,
 * but never owns their widgets or model values. Disabled widgets are excluded from aggregate
 * validation. clear() and destruction disconnect all bindings.
 *
 * @code
 * UIBindingGroup form;
 * form += bindValue( name, nameInput, requiredName );
 * form += bindValue( port, portInput, validPort );
 * auto canSave = computedValue( form.validValue(), form.dirtyValue(),
 *                               []( bool valid, bool dirty ) { return valid && dirty; } );
 * @endcode
 */
class UIBindingGroup {
  private:
	template <typename T> struct PropertyEntry;

  public:
	/** @brief Identifies one invalid binding and its current validation result. */
	struct Error {
		/** Insertion index of the invalid binding in this group. */
		std::size_t index{ 0 };
		/** Bound widget when one is still connected, otherwise nullptr. */
		UIWidget* widget{ nullptr };
		/** Validation state owned by the binding and valid until the group changes. */
		const UIValueValidationResult* validation{ nullptr };
	};
	/** Inline-backed error collection sized for ordinary forms. */
	using Errors = SmallVector<Error, 4>;
	/** Inline-backed widget collection sized for ordinary forms. */
	using Widgets = SmallVector<UIWidget*, 4>;
	using Callback = std::function<void()>;

	UIBindingGroup() = default;
	UIBindingGroup( const UIBindingGroup& ) = delete;
	UIBindingGroup& operator=( const UIBindingGroup& ) = delete;
	UIBindingGroup( UIBindingGroup&& ) = delete;
	UIBindingGroup& operator=( UIBindingGroup&& ) = delete;
	~UIBindingGroup() = default;

	/**
	 * @brief Adds and takes ownership of a typed binding.
	 * @return This group, allowing several bindings to be added in one expression.
	 */
	template <typename T> UIBindingGroup& hold( UIValueBinding<T>&& binding ) {
		add( std::make_unique<ObservableEntry<T>>( std::move( binding ), this ) );
		return *this;
	}

	template <typename T> UIBindingGroup& operator+=( UIValueBinding<T>&& binding ) {
		return hold( std::move( binding ) );
	}

	/** @brief Adds and takes ownership of a legacy UIDataBind. */
	template <typename T> UIBindingGroup& hold( std::unique_ptr<UIDataBind<T>> binding ) {
		add( std::make_unique<RawEntry<T>>( std::move( binding ), this ) );
		return *this;
	}

	template <typename T> UIBindingGroup& operator+=( std::unique_ptr<UIDataBind<T>> binding ) {
		return hold( std::move( binding ) );
	}

	/**
	 * @brief Tracks a UIProperty without taking ownership of it.
	 *
	 * The entry expires safely if @p property is destroyed first. Its current value becomes the
	 * initial clean baseline.
	 */
	template <typename T> UIBindingGroup& hold( UIProperty<T>& property ) {
		add( std::make_unique<PropertyEntry<T>>( property, this ) );
		return *this;
	}

	/** @brief Convenience form of hold(UIProperty<T>&). */
	template <typename T> UIBindingGroup& operator+=( UIProperty<T>& property ) {
		return hold( property );
	}

	/**
	 * @return true when every binding attached to an enabled widget is valid.
	 *
	 * Disabled fields are intentionally ignored, which supports conditional form sections.
	 */
	bool isValid() const {
		for ( const auto& entry : mEntries )
			if ( !entry->isValid() )
				return false;
		return true;
	}

	/** @return true when at least one value differs from its current clean baseline. */
	bool isDirty() const {
		for ( const auto& entry : mEntries )
			if ( entry->isDirty() )
				return true;
		return false;
	}

	/** @return Observable aggregate validity, suitable for computed values and commands. */
	ObservableValue<bool>& validValue() { return mValid; }

	/** @return Observable aggregate dirty state, suitable for computed values and commands. */
	ObservableValue<bool>& dirtyValue() { return mDirty; }

	/** @return Invalid enabled bindings in insertion order. */
	Errors errors() const {
		Errors result;
		for ( std::size_t i = 0; i < mEntries.size(); ++i )
			if ( !mEntries[i]->isValid() )
				result.push_back( { i, mEntries[i]->widget(), &mEntries[i]->validation() } );
		return result;
	}

	/**
	 * @return Connected widgets grouped by binding insertion order.
	 *
	 * The order within a legacy multi-widget UIDataBind or UIProperty is unspecified.
	 */
	Widgets widgets() const {
		Widgets result;
		for ( const auto& entry : mEntries )
			entry->appendWidgets( result );
		return result;
	}

	/** @return The first invalid enabled widget in insertion order, or nullptr. */
	UIWidget* firstInvalidWidget() const {
		for ( const auto& entry : mEntries )
			if ( !entry->isValid() && entry->widget() )
				return entry->widget();
		return nullptr;
	}

	/** @brief Makes every binding's current value its new clean/reset baseline. */
	void markClean() {
		for ( auto& entry : mEntries )
			entry->markClean();
		notifyIfChanged();
	}

	/** @brief Restores every binding to the baseline recorded at insertion or markClean(). */
	void reset() {
		for ( auto& entry : mEntries )
			entry->reset();
		notifyIfChanged();
	}

	/** @brief Destroys all owned bindings and resets aggregate state. */
	void clear() {
		mEntries.clear();
		notifyIfChanged();
	}

	/** @return The number of bindings owned by the group. */
	std::size_t size() const { return mEntries.size(); }

	/**
	 * @brief Replaces the callback invoked after a relevant group event.
	 *
	 * A non-empty callback is invoked once immediately, then after value, validation,
	 * enabled-state, baseline, or membership changes. Use validValue()/dirtyValue() when only
	 * aggregate transitions matter, since those observables suppress equal values.
	 */
	void onChange( Callback callback ) {
		mCallback = std::move( callback );
		if ( mCallback )
			mCallback();
	}

  private:
	struct Entry {
		virtual ~Entry() = default;
		virtual bool isValid() const = 0;
		virtual bool isDirty() const = 0;
		virtual UIWidget* widget() const = 0;
		virtual void appendWidgets( Widgets& widgets ) const = 0;
		virtual const UIValueValidationResult& validation() const = 0;
		virtual void markClean() = 0;
		virtual void reset() = 0;
		ObservableValue<UIValueValidationResult>::Connection validationConnection;
		EventConnectionList enabledConnections;
	};

	template <typename T> struct ObservableEntry : Entry {
		ObservableEntry( UIValueBinding<T>&& binding, UIBindingGroup* group ) :
			binding( std::move( binding ) ), baseline( this->binding.value() ) {
			if ( auto validation = this->binding.validationState() )
				this->validationConnection = validation->observe(
					[group]( const UIValueValidationResult& ) { group->notifyIfChanged(); } );
			valueConnection =
				this->binding.observeValue( [group]( const T& ) { group->notifyIfChanged(); } );
			if ( auto widget = this->binding.widget() )
				this->enabledConnections += widget->connect(
					Event::OnEnabledChange, [group]( const Event* ) { group->notifyIfChanged(); } );
		}
		bool isValid() const override {
			auto widget = binding.widget();
			return !widget || !widget->isEnabled() || binding.isValid();
		}
		bool isDirty() const override { return baseline != binding.value(); }
		UIWidget* widget() const override { return binding.widget(); }
		void appendWidgets( Widgets& widgets ) const override {
			if ( auto boundWidget = widget() )
				widgets.push_back( boundWidget );
		}
		const UIValueValidationResult& validation() const override {
			static const UIValueValidationResult valid;
			auto state = binding.validationState();
			return state ? state->result() : valid;
		}
		void markClean() override { baseline = binding.value(); }
		void reset() override {
			if ( baseline )
				binding.setValue( *baseline );
		}
		UIValueBinding<T> binding;
		std::optional<T> baseline;
		typename ObservableValue<T>::Connection valueConnection;
	};

	template <typename T> struct RawEntry : Entry {
		RawEntry( std::unique_ptr<UIDataBind<T>> binding, UIBindingGroup* group ) :
			binding( std::move( binding ) ), baseline( this->binding->get() ) {
			this->validationConnection = this->binding->validationState().observe(
				[group]( const UIValueValidationResult& ) { group->notifyIfChanged(); } );
			auto previous = std::move( this->binding->onValueChangeCb );
			this->binding->onValueChangeCb = [group,
											  previous = std::move( previous )]( const T& value ) {
				if ( previous )
					previous( value );
				group->notifyIfChanged();
			};
			for ( auto widget : this->binding->getWidgets() )
				this->enabledConnections += widget->connect(
					Event::OnEnabledChange, [group]( const Event* ) { group->notifyIfChanged(); } );
		}
		bool isValid() const override {
			if ( binding->isValid() )
				return true;
			if ( auto emitter = binding->getValidationEmitter() )
				return !emitter->isEnabled();
			for ( auto widget : binding->getWidgets() )
				if ( widget && widget->isEnabled() )
					return false;
			return true;
		}
		bool isDirty() const override { return baseline != binding->get(); }
		UIWidget* widget() const override {
			if ( auto emitter = binding->getValidationEmitter(); emitter && emitter->isEnabled() )
				return emitter;
			for ( auto widget : binding->getWidgets() )
				if ( widget && widget->isEnabled() )
					return widget;
			return nullptr;
		}
		void appendWidgets( Widgets& widgets ) const override {
			widgets.insert( widgets.end(), binding->getWidgets().begin(),
							binding->getWidgets().end() );
		}
		const UIValueValidationResult& validation() const override {
			return binding->validationState().result();
		}
		void markClean() override { baseline = binding->get(); }
		void reset() override { binding->set( baseline ); }
		std::unique_ptr<UIDataBind<T>> binding;
		T baseline;
	};

	template <typename T> struct PropertyEntry : Entry {
		PropertyEntry( UIProperty<T>& property, UIBindingGroup* group ) :
			handle( property.weakHandle() ),
			baseline( handle.get() ),
			currentValidation( handle.validation() ) {
			this->validationConnection =
				handle.observeValidation( [this, group]( const UIValueValidationResult& result ) {
					currentValidation = result;
					group->notifyIfChanged();
				} );
			valueConnection = handle.observe( [group]( const T& ) { group->notifyIfChanged(); } );
			lifetimeConnection =
				handle.observeLifetime( [group]( const bool& ) { group->notifyIfChanged(); } );
			handle.forEachWidget( [this, group]( UIWidget* widget ) {
				this->enabledConnections += widget->connect(
					Event::OnEnabledChange, [group]( const Event* ) { group->notifyIfChanged(); } );
			} );
		}
		bool isValid() const override {
			if ( !handle )
				return true;
			if ( currentValidation.valid )
				return true;
			if ( auto emitter = handle.validationEmitter() )
				return !emitter->isEnabled();
			return handle.firstEnabledWidget() == nullptr;
		}
		bool isDirty() const override {
			auto current = handle.get();
			return baseline && current && *baseline != *current;
		}
		UIWidget* widget() const override {
			if ( auto emitter = handle.validationEmitter(); emitter && emitter->isEnabled() )
				return emitter;
			return handle.firstEnabledWidget();
		}
		void appendWidgets( Widgets& widgets ) const override { handle.appendWidgets( widgets ); }
		const UIValueValidationResult& validation() const override { return currentValidation; }
		void markClean() override { baseline = handle.get(); }
		void reset() override {
			if ( baseline )
				handle.set( *baseline );
		}
		typename UIProperty<T>::WeakHandle handle;
		std::optional<T> baseline;
		UIValueValidationResult currentValidation;
		typename UIProperty<T>::Connection valueConnection;
		ObservableValue<bool>::Connection lifetimeConnection;
	};

	void add( std::unique_ptr<Entry> entry ) {
		mEntries.emplace_back( std::move( entry ) );
		notifyIfChanged();
	}

	void notifyIfChanged() {
		bool valid = true;
		bool dirty = false;
		for ( const auto& entry : mEntries ) {
			if ( valid && !entry->isValid() )
				valid = false;
			if ( !dirty && entry->isDirty() )
				dirty = true;
			if ( !valid && dirty )
				break;
		}
		if ( valid != mLastValid ) {
			mLastValid = valid;
			mValid = valid;
		}
		if ( dirty != mLastDirty ) {
			mLastDirty = dirty;
			mDirty = dirty;
		}
		if ( mCallback )
			mCallback();
	}

	// Most forms contain only a few fields; keep their entry ownership entirely inline.
	SmallVector<std::unique_ptr<Entry>, 4> mEntries;
	Callback mCallback;
	bool mLastValid{ true };
	bool mLastDirty{ false };
	ObservableValue<bool> mValid{ true };
	ObservableValue<bool> mDirty{ false };
};

}} // namespace EE::UI

#endif
