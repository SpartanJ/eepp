#ifndef EE_UISCENENODE_HPP
#define EE_UISCENENODE_HPP

#include <eepp/scene/scenenode.hpp>
#include <eepp/system/translator.hpp>
#include <eepp/ui/css/stylesheet.hpp>

namespace EE { namespace UI {

class UIWidget;
class UIWindow;

class EE_API UISceneNode : public SceneNode {
	public:
		static UISceneNode * New( EE::Window::Window * window = NULL );

		explicit UISceneNode( EE::Window::Window * window = NULL );

		virtual Node * setSize( const Sizef& size );

		virtual Node * setSize( const Float& Width, const Float& Height );

		const Sizef& getSize();

		virtual const Sizef& getRealSize();

		void setTranslator( Translator translator );

		Translator& getTranslator();

		String getTranslatorString( const std::string& str );

		String getTranslatorString( const std::string& str, const String& defaultValue );

		UIWidget * loadLayoutFromFile( const std::string& layoutPath, Node * parent = NULL );

		UIWidget * loadLayoutFromString( const std::string& layoutString, Node * parent = NULL );

		UIWidget * loadLayoutFromMemory( const void * buffer, Int32 bufferSize, Node * parent = NULL );

		UIWidget * loadLayoutFromStream( IOStream& stream, Node * parent = NULL );

		UIWidget * loadLayoutFromPack( Pack * pack, const std::string& FilePackPath, Node * parent = NULL );

		UIWidget * loadLayoutNodes( pugi::xml_node node, Node * parent );

		void setStyleSheet( const CSS::StyleSheet& styleSheet );

		void combineStyleSheet( const CSS::StyleSheet& styleSheet );

		CSS::StyleSheet& getStyleSheet();

		bool hasStyleSheet();

		const bool& isLoading() const;
	protected:
		friend class EE::UI::UIWindow;
		Sizef				mDpSize;
		Uint32				mFlags;
		Translator			mTranslator;
		std::list<UIWindow*> mWindowsList;
		CSS::StyleSheet mStyleSheet;
		bool				mIsLoading;

		virtual void resizeControl( EE::Window::Window * win );

		void				setActiveWindow( UIWindow * window );

		void				setFocusLastWindow( UIWindow * window  );

		void				windowAdd( UIWindow * win );

		void				windowRemove( UIWindow * win );

		bool				windowExists( UIWindow * win );

		virtual void setInternalSize(const Sizef& size );

		void reloadStyle();
};

}}

#endif
