UI Introduction
===============


Introduction
------------
eepp UI is designed to be flexible and efficient. Inspired in
[Android Layouts structure](https://developer.android.com/guide/topics/ui/declaring-layout)
and [CSS standards](https://developer.mozilla.org/en-US/docs/Web/CSS).
It's still a work in progress so several features are still not stable, but
already provides a very solid foundation to create rich and interactive UIs.


Layout
------
A layout defines the structure for a user interface in your app.
For constructing layouts in eepp we have two options: instantiate layout
elements at runtime or declare UI elements in XML.

  * **Declare UI elements in XML.** eepp provides a straightforward XML
    vocabulary that corresponds to the UI Widget classes and subclasses.
    There are two kinds of widgets: normal widgets and layouts. The difference
    between this two is that layouts are the ones in charge of arranging their
    child in a particular way.

  * **Instantiate layout elements at runtime.** Your app can create Widget and
    Layout objects (and manipulate their properties) programmatically.

Declaring your UI in XML allows you to separate the presentation of your app
from the code that controls its behavior. Using XML files in conjunction with
CSS files also makes it easy to provide different layouts for different screen
sizes.

The framework gives you the flexibility to use either or both of these methods
to build your app's UI. For example, you can declare your app's default
layouts in XML, and then modify the layout at runtime.


Write the XML
-------------
Using eepp XML vocabulary, you can quickly design UI layouts and the screen
elements they contain, in the same way you create web pages in HTML — with a
series of nested elements.

The XML widget elements will be added recursively to the desired parent ( or
the root element of the UI ).
You can add additional layout objects or widgets as child elements to gradually
build a view hierarchy that defines your layout. For example, here's an XML
layout that uses a vertical EE::UI::UILinearLayout to hold a EE::UI::UITextView
and a EE::UI::UIPushButton:

```xml
<LinearLayout layout_width="match_parent"
              layout_height="match_parent"
              orientation="vertical" >
    <TextView id="text_view"
              layout_width="match_parent"
              layout_height="wrap_content"
              text="Hello, I am a TextView" />
    <PushButton id="button_view"
            layout_width="match_parent"
            layout_height="wrap_content"
            text="Hello, I am a Button" />
</LinearLayout>
```

After you've declared your layout in XML, save the file with the `.xml`
extension into a project accessible path and you're done.


Initializing the UI
-------------------
The library does not assume that the user is going to use the UI at all, so you
must initialize it manually. This usually comes right after the main
EE::Window::Window initialization.

The process consist in very few steps: Loading a default font. Instantiating the
EE::UI::UISceneNode. Setting the font as the default scene node. And finally,
adding this scene node to the EE::Scene::SceneManager.

For example:
```cpp
// ... after window creation
// Load a font to use as the default font in our UI.
FontTrueType* font =
    FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );

// Create a new scene node to add our widgets.
UISceneNode* uiSceneNode = UISceneNode::New();

// Set the default font used in the scene node (otherwise we won't have any font to create
// text views).
uiSceneNode->getUIThemeManager()->setDefaultFont( font );

// Add the new scene node to the scene manager.
SceneManager::instance()->add( uiSceneNode );
```

And that's all we need for a basic initialization.


Updating the UI
---------------
Updating the UI consist in two simple steps: updating and drawing the UI scene
node contained by the EE::Scene::SceneManager.

So we just need to do in our main loop:
```cpp
// Update the UI scene
SceneManager::instance()->update();

// Check if the UI has been invalidated ( needs redraw )
if ( SceneManager::instance()->getUISceneNode()->invalidated() ) {
    // Clear the window.
    win->clear();

    // Redraw the UI scene.
    SceneManager::instance()->draw();

    // Display the scene to the screen.
    win->display();
} else {
    Sys::sleep( Milliseconds( 8 ) );
}
```

This example only redraws the scene when the EE::UI::UISceneNode indicates
that changes has been detected that needs to redraw the scene. You can redraw
the scene on every update or only when is needed. There are several options
regarding this topic, since each scene node can be drawn into a separated
frame buffer, in order to be able to control when to redraw a scene.


Load the XML Layout Resource
----------------------------
XML layout resources can be loaded from any file system path,
EE::UI::Pack accessible path or a hard-coded string.

Layout resources will be appended to the EE::Scene::Node (the most basic element
in a EE::Scene::SceneNode) specified by the user, if no node is specified the
root scene node will be used.

Following the [Write the XML](#write-the-xml) layout and the
[Initializing the UI](#initializing-the-ui) code:
```cpp
uiSceneNode->loadLayoutFromString( R"xml(
            <LinearLayout layout_width="match_parent"
                          layout_height="match_parent"
                          orientation="vertical">
                <TextView id="text_view"
                          layout_width="match_parent"
                          layout_height="wrap_content"
                          text="Hello, I am a TextView" />
                <PushButton id="button_view"
                        layout_width="match_parent"
                        layout_height="wrap_content"
                        text="Hello, I am a PushButton" />
            </LinearLayout>
        )xml" );
```

This will load and create the widgets into the root element of the UI scene
node.


Cascading Style Sheets
----------------------
[CSS (Cascading Style Sheets)](https://developer.mozilla.org/en-US/docs/Web/CSS)
is the code you use to style the UI. It's very heavily based on the
[CSS 2.1](https://www.w3.org/TR/CSS21/) standards with a good amount of CSS 3
features. Some of the main current differences are: eepp CSS properties don't
support inheritance (except for
[Custom properties](https://developer.mozilla.org/en-US/docs/Web/CSS/Using_CSS_custom_properties),
and `*` is supported), eepp also adds new properties oriented to decoration
related and layout control stuffs, and CSS layout properties differ from the
standard since we use a different layout model. But you can learn how style the
UI following the CSS standards and then reading about the specific eepp UI
features.
Important CSS3 features that are currently supported:

  * [Transitions](https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Transitions)

  * [Custom properties](https://developer.mozilla.org/en-US/docs/Web/CSS/Using_CSS_custom_properties)

  * [Media queries](https://developer.mozilla.org/en-US/docs/Web/CSS/Media_Queries/Using_media_queries)

  * [@font-face at rule](https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face)

  * [:root element](https://developer.mozilla.org/en-US/docs/Web/CSS/:root)

  * [Most of the background properties](https://developer.mozilla.org/en-US/docs/Web/CSS/background)


Write the CSS
-------------
Following [Load the XML Layout Resource](#load-the-xml-layout-resource) we are
going to write a very simple style to our UI. In order to that we can create a
new CSS file (with `.css` extension) in our file system and save it in an
accesible path to our project, or we can simply write the CSS in a string in our
code.

This is how our CSS could look like:
```css
* {
    font-size: 22dp;
}

TextView {
    background-color: white;
    color: black;
    text-align: center;
}

PushButton {
    background-color: red;
    color: white;
}
```

The only difference we can spot in this example with the CSS standard is that
eepp UI support a new CSS unit called "dp", representing a [Device-independent
pixel](https://en.wikipedia.org/wiki/Device-independent_pixel), that allow us to
keep our layout consistent between different screen densities. This concept was
taken from the [Android pixel density implementation](https://developer.android.com/guide/practices/screens_support.html#density).

Loading the CSS Resource
------------------------
Following [Load the XML Layout Resource](#load-the-xml-layout-resource) and
[Write the CSS](#write-the-css) we are going to load our CSS from a string:

```cpp
// We can also use "combineStyleSheet" to combine the new CSS with any previously set CSS.
uiSceneNode->setStyleSheet( R"css(
    * {
        font-size: 22dp;
    }

    TextView {
        background-color: white;
        color: black;
        text-align: center;
    }

    PushButton {
        background-color: red;
        color: white;
    }
)css" );
```

To load a CSS from a file we need to first parse it with the
EE::UI::CSS::StyleSheetParser.

It will look like:
```cpp
CSS::StyleSheetParser parser;
parser.loadFromFile( "path/to/our/stylesheet.css" );
uiSceneNode->setStyleSheet( parser.getStyleSheet() );
```

Example
-------
For a complete example of this introduction you can look into:
[src/examples/ui_hello_world/ui_hello_world.cpp](https://github.com/SpartanJ/eepp/blob/develop/src/examples/ui_hello_world/ui_hello_world.cpp).
