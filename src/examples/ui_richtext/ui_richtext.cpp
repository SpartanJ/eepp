#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char** ) {
	UIApplication app( { 800, 600, "eepp - UIRichText Example" } );
	app.getUI()->loadLayoutFromString( R"xml(
	<vbox layout_width="match_parent" layout_height="match_parent">
		<ScrollView lw="mp" lh="mp">
			<vbox layout_width="match_parent" layout_height="wrap_content" padding="8dp">
				<RichText font-size="12dp"
					color="white">Welcome to the <span color="#FFD700" font-style="bold">UIRichText</span> example!
					This component supports <span color="#00FF00" font-style="italic">styled text</span>,
					<span color="#00BFFF" font-style="shadow">shadows</span>,
					and <span color="#FF4500" text-stroke-width="1dp" text-stroke-color="black">outlines</span> using <span font-family="monospace" color="#A9A9A9">HTML-like tags</span>.
				</RichText>
				<Image src="file://assets/icon/ee.png" margin="4dp" layout-gravity="center_horizontal" />
				<RichText font-size="12dp"
				color="#fefefe">We can also mix <span color="#FFD700" font-style="bold">contents</span> with more <span color="#00FF00" font-style="italic">text</span>!
				</RichText>
<h1>eepp - Entropia Engine++</h1>
<p><strong>eepp</strong> is an open source cross-platform game and application development
framework heavily focused on the development of rich graphical user interfaces.</p>
<p><a href="https://github.com/SpartanJ/eepp/actions?query=workflow%3ALinux"><img src="https://img.shields.io/github/actions/workflow/status/SpartanJ/eepp/eepp-linux-build-check.yml?branch=develop&amp;label=Linux" alt="Linux status" /></a>
<a href="https://github.com/SpartanJ/eepp/actions?query=workflow%3AWindows"><img src="https://img.shields.io/github/actions/workflow/status/SpartanJ/eepp/eepp-windows-build-check.yml?branch=develop&amp;label=Windows" alt="Windows status" /></a>
<a href="https://github.com/SpartanJ/eepp/actions?query=workflow%3AmacOS"><img src="https://img.shields.io/github/actions/workflow/status/SpartanJ/eepp/eepp-macos-build-check.yml?branch=develop&amp;label=macOS" alt="macOS status" /></a>
<a href="https://github.com/SpartanJ/eepp/actions?query=workflow%3AiOS"><img src="https://img.shields.io/github/actions/workflow/status/SpartanJ/eepp/eepp-ios-build-check.yml?branch=develop&amp;label=iOS" alt="iOS status" /></a>
<a href="https://github.com/SpartanJ/eepp/actions?query=workflow%3AAndroid"><img src="https://img.shields.io/github/actions/workflow/status/SpartanJ/eepp/eepp-android-build-check.yml?branch=develop&amp;label=Android" alt="Android status" /></a></p>
<h2>Features</h2>
<h3>Cross platform functionality</h3>
<ul>
<li><p>Official support for Linux, Windows, macOS, FreeBSD, Haiku, Android and iOS.</p>
</li>
<li><p>Exports to HTML5 using emscripten with some minor limitations.</p>
</li>
</ul>
<h3>UI Module</h3>
<ul>
<li><p>Base widgets to manage the app/game objects as nodes, with all basic input interaction events ( clicks, keypress, mouser over, focus, etc ).</p>
</li>
<li><p>Fully featured UI system, animation support, scaling, rotating, clipping, events, messages, etc.</p>
</li>
<li><p>Themes and skins/decorations support.</p>
</li>
<li><p>Pixel density support ( scaling of UI elements defined by the pixel density of the screen ).</p>
</li>
<li><p>All the basic widgets are implemented ( button, textbox, combobox, inputbox, menu, listbox, scrollbars, etc ).</p>
</li>
<li><p>Draw invalidation support. It can be used to make real apps, with low resource usage ( only redraws when is needed ).</p>
</li>
<li><p>Layout system similar to Android layouts ( LinearLayout, RelativeLayout, GridLayout ).</p>
</li>
<li><p>Advanced features as text selection, copy and paste, and key bindings.</p>
</li>
<li><p>Load and style layouts from XMLs</p>
</li>
<li><p>Styling with Cascading Style Sheets</p>
</li>
</ul>
<h3>Graphics Module</h3>
<ul>
<li><p>Renderers for OpenGL 2 ( fixed-pipeline ), OpenGL 3 ( programmable-pipeline ), OpenGL ES 2, OpenGL ES 1, and OpenGL Core Profile.</p>
</li>
<li><p>Batch Renderer ( all the rendering is automatically batched by the engine ).</p>
</li>
<li><p>Fonts support ( TrueType, BMFont and XNA Fonts ).</p>
</li>
<li><p>Frame Buffer support.</p>
</li>
<li><p>Shaders support ( with automatic fixed pipeline shaders to programmable conversor ).</p>
</li>
<li><p>Vertex Buffer Object support.</p>
</li>
<li><p>Particle System.</p>
</li>
<li><p>Extendable Console.</p>
</li>
<li><p>Animated Sprites.</p>
</li>
<li><p>Texture Atlas support ( automatic creation and update of the texture atlas, editor included ).</p>
</li>
<li><p>Clipping Masks ( stencil, scissors, planes )</p>
</li>
<li><p>Nine Patch resizable bitmaps support.</p>
</li>
<li><p>Primitives drawables.</p>
</li>
<li><p>Many image formats supported ( included rasterized SVG ), compressed textures support ( direct upload to the GPU  when possible ).</p>
</li>
</ul>
<h3>Window Module</h3>
<ul>
<li><p>Backend based module, this means that you can easily create a backend for the window/input handling.</p>
</li>
<li><p>Currently supports SDL 2 as backend.</p>
</li>
<li><p>Clipboard support.</p>
</li>
<li><p>Hardware cursors.</p>
</li>
<li><p>Display Manager</p>
</li>
<li><p>Joystick support.</p>
</li>
</ul>
<h3>Audio Module</h3>
<ul>
<li>OpenAL audio engine with extendable file format support. Read and write support for OGG and Wav, and read support for MP3 and FLAC.</li>
</ul>
<h3>System Module</h3>
<ul>
<li><p>Provides all the basics stuffs for the full multi-threading support of the library, file formats support for packing, clocks, resource manager, translator, and much more.</p>
</li>
<li><p>Virtual File System class ( abstract assets providers into a single virtual file system, abstracting zip files and local file system into one for transparent load of resources, similar to <a href="https://www.icculus.org/physfs/">PhysicsFS</a> ).</p>
</li>
</ul>
<h3>Core Module</h3>
<ul>
<li><p>Customizable Memory Manager. Used by default in debug mode to track memory leaks.</p>
</li>
<li><p>UTF8, UTF-16, UTF-32, Ansi, Wide Char support.</p>
</li>
<li><p>String class using UTF-32 chars internally.</p>
</li>
<li><p>Debug macros</p>
</li>
</ul>
<h3>Math Module</h3>
<ul>
<li><p>General purpose functions and templates ( vector, quad, polygon, etc ).</p>
</li>
<li><p>Interpolation classes with easing.</p>
</li>
<li><p>Some minor math utilities, include Mersenne Twister random number generator implementation, perlin noise and more.</p>
</li>
</ul>
<h3>Network Module</h3>
<ul>
<li><p>Web Requests with HTTP client, with <strong>TLS support</strong> ( provided by mbedtls or openssl ).</p>
</li>
<li><p>Asynchronous HTTP requests.</p>
</li>
<li><p>File Transfers with FTP client and FTPS client ( FTP with explicit TLS ).</p>
</li>
<li><p>TCP and UDP sockets.</p>
</li>
<li><p>HTTP Content-Encoding and Transfer-Encoding support.</p>
</li>
<li><p>HTTP Proxy Support.</p>
</li>
<li><p>HTTP Compressed response support.</p>
</li>
<li><p>Also HTTP resume/continue download support and automatic follow redirects.</p>
</li>
</ul>
<h3>Scene Module</h3>
<ul>
<li><p>Node based system for easy management of scenes.</p>
</li>
<li><p>Full control of node events ( clicks, mouse over, focus, etc ).</p>
</li>
<li><p>Event system.</p>
</li>
<li><p>Node Message system.</p>
</li>
<li><p>Programmable actions for nodes ( fade, rotate, move, scale, etc ).</p>
</li>
</ul>
<h3>Physics Module (optional)</h3>
<ul>
<li>Full OOP chipmunk physics wrapper.</li>
</ul>
<h3>Maps Module (optional)</h3>
<ul>
<li><p>Tiled Maps with software dynamic lights.</p>
</li>
<li><p>Full featured map editor.</p>
</li>
</ul>
<h3>Tools</h3>
<ul>
<li><p>Very simple UI Editor. Load layouts from an XML file and see the changes being made in real time.</p>
</li>
<li><p>Texture Atlas Editor. A very simple tool to allow the developer to create and edit texture atlases.</p>
</li>
<li><p>Map Editor: A advanced but simple map editor for the game engine. It lacks several features since I didn't have the time to work on it, this particular tool will probably die in favor of TMX map support in the near future ( but i'm not a fan of TMX maps, so there's no decision for the moment ).</p>
</li>
</ul>
<h3>General Features</h3>
<ul>
<li>Support for multi-threaded resource loading ( textures, sounds, fonts, etc ).</li>
</ul>
<h2>Documentation</h2>
<p>Documentation is located <a href="https://cdn.ensoft.dev/eepp-docs/index.html">here</a>. I'm currently working
on improving it. About 50% of the project is currently documented so still needs
a lot of work. Please check the code examples located in <code>src/examples</code> and you
can also check out the test ( <code>src/test</code> ) and tools ( <code>src/tools</code> ).</p>
<p>I'm putting my efforts on improving the documentation on the UI module since
currently is the most important and complex module but lacks of proper
documentation. If you have any question you can contact me anytime.</p>
<h2>Getting the code</h2>
<p>The repository uses git submodules so you'll need to clone the repository and
its submodules, in order to achieve this easily you can simply clone with:</p>
<p><code>git clone --recurse-submodules https://github.com/SpartanJ/eepp.git</code></p>
<h2>UI Screenshots</h2>
<h3>ecode - Code Editor</h3>
<p><a href="https://github.com/SpartanJ/ecode/">ecode</a> is a code editor inspired in <a href="https://github.com/rxi/lite">lite</a>.
It's using the newest pure CSS theme based on the default <a href="https://kde.org/plasma-desktop">Plasma</a>
dark theme: <a href="https://github.com/KDE/breeze">Breeze Dark</a>.</p>
<p><img src="https://cdn.ensoft.dev/eepp-demos/screenshots/ecode.png" alt="ecode - Code Editor" /></p>
<h3>UI Editor</h3>
<p>Editor that displays in real-time the changes on any layout and CSS to help speed up the development
of user interfaces. In the screenshot is displaying some of the default widgets available in eepp.</p>
<p><img src="https://cdn.ensoft.dev/eepp-demos/screenshots/uieditor.png" alt="UI Editor" /></p>
<h3>Texture Atlas Editor</h3>
<p>Small tool, used to create and edit texture atlases.</p>
<p><img src="https://cdn.ensoft.dev/eepp-demos/screenshots/taeditor.png" alt="Texture Atlas Editor with 1.5x pixel density" /></p>
<h3>Map Editor</h3>
<p>2D map editor using the default skinned theme (using a single texture atlas with 9-patch images).</p>
<p><img src="https://cdn.ensoft.dev/eepp-demos/screenshots/eepp1.png" alt="Map Editor" /></p>
<h2>UI Layout XML example</h2>
<p>It should look really familiar to any Android developer. This is a window with
the most basic widgets in a vertical linear layout display.</p>
<pre><code class="language-xml">&lt;window layout_width=&quot;300dp&quot; layout_height=&quot;300dp&quot; window-flags=&quot;default|maximize|shadow&quot;&gt;
  &lt;LinearLayout id=&quot;testlayout&quot; orientation=&quot;vertical&quot; layout_width=&quot;match_parent&quot; layout_height=&quot;match_parent&quot; layout_margin=&quot;8dp&quot;&gt;
    &lt;TextView text=&quot;Hello World!&quot; gravity=&quot;center&quot; layout_gravity=&quot;center_horizontal&quot; layout_width=&quot;match_parent&quot; layout_height=&quot;wrap_content&quot; backgroundColor=&quot;black&quot; /&gt;
    &lt;PushButton text=&quot;OK!&quot; textSize=&quot;16dp&quot; icon=&quot;ok&quot; gravity=&quot;center&quot; layout_gravity=&quot;center_horizontal&quot; layout_width=&quot;match_parent&quot; layout_height=&quot;wrap_content&quot; /&gt;
    &lt;Image src=&quot;thecircle&quot; layout_width=&quot;match_parent&quot; layout_height=&quot;32dp&quot; flags=&quot;clip&quot; /&gt;
    &lt;Sprite src=&quot;gn&quot; /&gt;
    &lt;TextInput text=&quot;test&quot; layout_width=&quot;match_parent&quot; layout_height=&quot;wrap_content&quot; /&gt;
    &lt;DropDownList layout_width=&quot;match_parent&quot; layout_height=&quot;wrap_content&quot; selectedIndex=&quot;0&quot;&gt;
      &lt;item&gt;Test Item&lt;/item&gt;
      &lt;item&gt;@string/test_item&lt;/item&gt;
    &lt;/DropDownList&gt;
    &lt;ListBox layout_width=&quot;match_parent&quot; layout_height=&quot;match_parent&quot; layout_weight=&quot;1&quot;&gt;
      &lt;item&gt;Hello!&lt;/item&gt;
      &lt;item&gt;World!&lt;/item&gt;
    &lt;/ListBox&gt;
  &lt;/LinearLayout&gt;
&lt;/window&gt;
</code></pre>
<p><strong>UI introduction can be found <a href="https://cdn.ensoft.dev/eepp-docs/page_uiintroduction.html">here</a></strong>.</p>
<h2>UI Widgets with C++ example</h2>
<p>How does it look with real code?</p>
<pre><code class="language-cpp">UITextView::New()-&gt;setText( &quot;Text  on  test  1&quot; )
         -&gt;setCharacterSize( 12 )
         -&gt;setLayoutMargin( Rect( 10, 10, 10, 10 ) )
         -&gt;setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
         -&gt;setParent( layout );
</code></pre>
<h2>UI Styling</h2>
<p>Element styling can be done with a custom implementation of Cascading Style
Sheets, most common CSS2 rules are available, plus several CSS3 rules (some
examples: <a href="https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Animations/">animations</a>,
<a href="https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Transitions">transitions</a>,
<a href="https://developer.mozilla.org/en-US/docs/Web/CSS/Using_CSS_custom_properties">custom properties</a>,
<a href="https://developer.mozilla.org/en-US/docs/Web/CSS/Media_Queries/Using_media_queries">media queries</a>,
<a href="https://developer.mozilla.org/en-US/docs/Web/CSS/@font-face">@font-face at rule</a>,
<a href="https://developer.mozilla.org/en-US/docs/Web/CSS/:root">:root element</a>).
Here is a small example on how the CSS looks like:</p>
<pre><code class="language-css">@font-face {
  font-family: &quot;OpenSans Regular&quot;;
  src: url(&quot;https://raw.githubusercontent.com/SpartanJ/eepp/develop/bin/assets/fonts/OpenSans-Regular.ttf&quot;);
}

@import url(&quot;assets/layouts/imported.css&quot;) screen and (min-width: 800px);

:root {
  --font-color: black;
  --background-input-color: rgba(255, 255, 255, 0.7);
  --border-color: black;
  --border-width: 1dp;
}

.screen TextView {
  color: var(--font-color);
}

.form {
  background-image: @drawable/back;
  background-repeat: no-repeat;
  background-size: cover;
}

.form .form_inputs {
  background-color: var(--non-existent, var(--background-input-color));
  margin-left: 100dp;
  margin-right: 100dp;
  padding-top: 72dp;
  padding-left: 57dp;
  padding-right: 57dp;
  padding-bottom: 115dp;
}

.screen TextView.input,
.screen TextInput.input {
  font-family: AkzidenzGroteskBQ-Cnd;
  layout-width: match_parent;
  layout-height: 80dp;
  border-color: var(--border-color);
  border-width: var(--border-width);
  color: var(--font-color);
  padding-left: 40dp;
  padding-right: 40dp;
  margin-bottom: 32dp;
  skin: none;
  hint-font-family: AkzidenzGroteskBQ-Cnd;
  hint-font-size: 46dp;
  hint-color: #818285;
  background-color: #FFFFFF00;
  transition: all 0.125s;
}

.screen TextInput.input:focus {
  background-color: #FFFFFF66;
  border-color: #796500;
}

.screen TextInput.input:hover {
  background-color: #FFFFFF66;
}

@media screen and (max-width: 1024px) {

.form .form_inputs {
  background-color: red;
}

}
</code></pre>
<p><strong>The complete CSS specification can be found in the docs: <a href="https://cdn.ensoft.dev/eepp-docs/page_cssspecification.html">here</a>.</strong></p>
<p><strong>You can also check how a pure CSS theme looks like in eepp: <a href="https://github.com/SpartanJ/eepp/blob/develop/bin/assets/ui/breeze.css">here</a>.</strong></p>
<h2>Live demos (using emscripten)</h2>
<p>Since eepp supports emscripten you can take a quick look on some of the examples, demos and tools that
the library currently provides. Please be aware that you'll find some differences based on the limitations
that emscripten have at the moment (no access to the file system, no custom cursors, etc) and also
demos are not optimized for size and they're bigger than they should be.
Note: Please use a modern browser with good WebGL and WASM support (Chrome/ium 70+ or Firefox 80+).</p>
<ul>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=ecode.js">ecode - Code Editor</a></strong></p>
</li>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-UIEditor.js">UI Editor</a></strong></p>
</li>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-ui-hello-world.js">UI Hello World</a></strong></p>
</li>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-TextureAtlasEditor.js">Texture Atlas Editor</a></strong></p>
</li>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-MapEditor.js">Map Editor</a></strong></p>
</li>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-fonts.js">Fonts example</a></strong></p>
</li>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-physics-demo.js">Physics module demo</a></strong></p>
</li>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-sprites.js">Sprites example</a></strong></p>
</li>
<li><p><strong><a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-test.js">Full Test</a></strong></p>
</li>
</ul>
<h3>7GUIs Examples</h3>
<p><a href="https://7guis.github.io/7guis/">7GUIs</a> is known as a &quot;GUI Programming Benchmark&quot; that it's used to
compare different GUI libraries and explore each library approach to GUI programming. All the 7 tasks
proposed in 7GUIs has been implemented for eepp. The tasks are very good representative of what can
be achieved with eepp GUI and also are very useful to demonstrate how to implement different tasks
with the library.</p>
<p>The 7GUIs are composed by the following tasks:</p>
<ul>
<li><p><a href="https://7guis.github.io/7guis/tasks#counter">Counter</a>: <a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-7guis-counter.js">Demo</a> and <a href="https://github.com/SpartanJ/eepp/blob/develop/src/examples/7guis/counter/counter.cpp">code implementation</a>.</p>
</li>
<li><p><a href="https://7guis.github.io/7guis/tasks#temp">Temperature Converter</a>: <a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-7guis-temperature-converter.js">Demo</a> and <a href="https://github.com/SpartanJ/eepp/blob/develop/src/examples/7guis/temperature_converter/temperature_converter.cpp">code implementation</a>.</p>
</li>
<li><p><a href="https://7guis.github.io/7guis/tasks#flight">Flight Booker</a>: <a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-7guis-flight-booker.js">Demo</a> and <a href="https://github.com/SpartanJ/eepp/blob/develop/src/examples/7guis/flight_booker/flight_booker.cpp">code implementation</a>.</p>
</li>
<li><p><a href="https://7guis.github.io/7guis/tasks#timer">Timer</a>: <a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-7guis-timer.js">Demo</a> and <a href="https://github.com/SpartanJ/eepp/blob/develop/src/examples/7guis/timer/timer.cpp">code implementation</a>.</p>
</li>
<li><p><a href="https://7guis.github.io/7guis/tasks#crud">CRUD</a>: <a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-7guis-crud.js">Demo</a> and <a href="https://github.com/SpartanJ/eepp/blob/develop/src/examples/7guis/crud/crud.cpp">code implementation</a>.</p>
</li>
<li><p><a href="https://7guis.github.io/7guis/tasks#circle">Circle Drawer</a>: <a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-7guis-circle-drawer.js">Demo</a> and <a href="https://github.com/SpartanJ/eepp/blob/develop/src/examples/7guis/circle_drawer/circle_drawer.cpp">code implementation</a>.</p>
</li>
<li><p><a href="https://7guis.github.io/7guis/tasks#cells">Cells</a>: <a href="https://cdn.ensoft.dev/eepp-demos/demo-fs.html?run=eepp-7guis-cells.js">Demo</a> and <a href="https://github.com/SpartanJ/eepp/tree/develop/src/examples/7guis/cells">code implementation</a>.</p>
</li>
</ul>
<h2>How to build it</h2>
<p>The library has only one external dependency. You will only need <strong>SDL2</strong> library with the headers
installed. Also <strong>premake5</strong> or <strong>premake4</strong> is needed to generate the Makefiles or project
files to build the library. I will assume that you know what you are doing and
skip the basics.</p>
<p>Notice: eepp uses <a href="https://icculus.org/mojoAL/">mojoAL</a> by default as an OpenAL drop-in replacement.
OpenAL is optionally available as an audio backend. If you want to use it, you
have the alternative to enable it. To enable it and disable the mojoAL drop-in replacemente, you
need to add the parameter <code>--without-mojoal</code> to any <code>premake</code> call
( ex: <code>premake5 --without-mojoal gmake</code> ).</p>
<h3>GNU/Linux</h3>
<p>In a Ubuntu system it would be something like ( also you will need gcc but it
will be installed anyways ):</p>
<p><code>sudo apt-get install premake5 libsdl2-2.0-0 libsdl2-dev</code></p>
<p>Clone the repository and run on the repository root directory:</p>
<p><code>premake5 gmake</code></p>
<p>or if you have premake4 installed you can run:</p>
<p><code>premake4 gmake</code></p>
<p>Then just build the library:</p>
<p>if <code>premake4</code> was used:</p>
<p><code>make -C make/linux config=release</code> (<code>config=debug</code> for debug build)</p>
<p>if <code>premake5</code> was used:</p>
<p><code>make -C make/linux config=release_x86_64</code> (<code>debug_x86_64</code> for debug build, or <code>release_arm64</code>/<code>debug_arm64</code> if building from arm64)</p>
<p>That's it. That will build the whole project.</p>
<h3>Windows</h3>
<p>You have two options: build with <a href="https://visualstudio.microsoft.com/">Visual Studio</a>
or with <a href="https://github.com/skeeto/w64devkit/releases/latest">MinGW</a>.
To be able to build the project with any of these options first you will need to
generate the project files with <a href="https://premake.github.io/download">premake4 or premake5</a>.
Then you will need to add the binary file to any of the executable paths defined
in <code>PATH</code> ( or add one, or use it from a local path ).
Download <em>Visual Studio</em> or <em>MinGW</em> files depending on your needs.</p>
<h4>Visual Studio</h4>
<p>You will need to use premake5 and run:</p>
<p><code>premake5.exe --windows-vc-build vs2022</code></p>
<p>Then the project files should be found in <code>make/windows/</code>. A complete solution
and all the project will be available. Having installed everything, you'll be
able to build the <em>Visual Studio</em> solution as any other project.</p>
<p>Using the command line argument <code>--windows-vc-build</code> will download the SDL2 dependency automatically
and add the paths to the build process to link against it without the need to download manually any
external dependency.</p>
<p>Then just build the solution in Visual Studio or run <code>MSBuild</code> manually in a
console:</p>
<p><code>&quot;%MSBUILD_PATH%\MSBuild.exe&quot; .\make\windows\eepp.sln -m</code></p>
<p>Where <code>%MSBUILD_PATH%</code> is the MSBuild.exe Visual Studio path, for example for
<em>VS2022 Community Edition</em> the path usually is:</p>
<p><code>C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\</code></p>
<h4>MinGW</h4>
<p>Windows MinGW builds are being produced and tested with <a href="https://github.com/skeeto/w64devkit/releases/latest">w64devkit</a> distribution.
MSYS is currently not officially supported given some issues found on the build process (but it's possible to build with some extra steps).</p>
<p>If you're using w64devkit you'll have to <a href="https://github.com/skeeto/w64devkit/releases/latest">download</a> it and extract it, we will assume that it's extracted at <code>C:\w64devkit</code>.</p>
<p>Execute <code>C:\w64devkit\w64devkit.exe</code> as an administrator (<code>right click</code> -&gt; <code>Run as administrator</code> ).</p>
<p>Then go to the <code>eepp</code> cloned repository directory and run:</p>
<p><code>premake5.exe --windows-mingw-build gmake</code></p>
<p><code>--windows-mingw-build</code> will automatically download and link external dependencies (SDL2).</p>
<p>Then just build the project located in <code>make/windows/</code> with <code>mingw32-make.exe</code> or any equivalent:</p>
<p><code>mingw32-make.exe -C make\\windows config=release_x86_64</code></p>
<p>To build a debug build run:</p>
<p><code>mingw32-make.exe -C make\\windows config=debug_x86_64</code></p>
<p>And then make sure to copy the <code>SDL2.dll</code> file located at <code>src/thirdparty/SDL2-2.XX.X/x86_64-w64-mingw32/bin/SDL2.dll</code> to <code>bin</code>.
If for some reason <code>eepp.dll</code> (or <code>eepp-debug.dll</code>) hasn't being copied automatically you can copy them from <code>libs/windows/x86_64/</code> to <code>bin</code>.</p>
<h3>macOS</h3>
<p>You will need the prebuild binaries and development libraries of
<a href="http://libsdl.org/download-2.0.php">SDL2</a>, OpenAL is included with the OS.
Install the SDL2 framework and you should be able to build the project.</p>
<p>You have two options to build the project: with <em>XCode</em> or with <em>Makefiles</em>.
To build with any of both options first you will also need to build the project
files with <a href="https://premake.github.io/download.html">premake4 or premake5</a>.</p>
<h4>Makefiles</h4>
<h5>Using premake5</h5>
<p>Generate the project:</p>
<p><code>premake5 --use-frameworks gmake</code></p>
<p>And build it:</p>
<p><code>make -C make/macosx config=release_x86_64</code> (or <code>config=debug_x86_64</code> for a debug build, or <code>release_arm64</code>/<code>debug_arm64</code> if building from arm64)</p>
<h5>Using premake4</h5>
<p>You can use the <code>projects/osx/make.sh</code> script, that generates the <em>Makefiles</em>
and builds the project.</p>
<h4>XCode</h4>
<p>Run:</p>
<p><code>premake5 --use-frameworks xcode4</code></p>
<p>And open the XCode project generated at <code>make/macosx/</code> or simply build from the
command line with:</p>
<p><code>xcodebuild -project make/macosx/project-name.xcodeproj</code></p>
<h3>Android</h3>
<p>There's a gradle project in <code>projects/android-project/</code>. It will build the
library with all the dependencies included. Use the example project as a base
for your project. Notice that there's a <code>eepp.mk</code> project file that builds the
library. That file can be used in you projects.</p>
<h3>iOS</h3>
<p>The project provides two files to build the library and the demos. You can use
any of them depending on your needs.
The files are located in <code>projects/ios</code>:</p>
<h4>gen-xcode4-proj.sh script</h4>
<p>This script can be used to generate the xcode projects and solution of all the
included projects in eepp (demos, tools, shared lib, static lib, etc). It will
also download and build the SDL2 fat static library in order to be able to
reference the library to the project. After building the projects sadly you'll
need to make some minor changes to any/all of the projects you wan't to build
or test, since the project generated lacks some minor configurations. After
you run this script you'll need to open the solution located in
<code>make/ios/eepp.xcworkspace</code>. To build the static libraries you'll not find any
problem (that will work out of the box). But to test some of the examples it's
required to:</p>
<h5>Add the Info.plist file</h5>
<p>Select (click on the project name) the project you want to test, for example
<code>eepp-empty-window</code>. You will se several tabs/options, go to <em>Build Settings</em>,
and locate the option <em>Info.plist</em> file, double click to edit and write:
<code>Info.plist</code>. This will indicate to read that file that is located in the same
directory than the project. The go to the tab <em>General</em> and complete the
<em>Bundle Identifier</em> with an identifier name of the app bundle that will be
generated, for this example you can use something like: <code>eepp-empty-window</code>.
That will allow you to build and run the project.</p>
<h5>Add resources to the project</h5>
<p>This <code>eepp-empty-window</code> demo does not use any assets/resources, but other demos
will need to load assets, and this assets need to be added to the project in order
to be available to the app bundle. For example, the project <code>eepp-ui-hello-world</code>,
will require you to add the <code>assets</code> folder into the project. What you need to do
is: select the project and go to the <em>Build Phases</em> tab, in <em>Copy Bundles Resources</em>
click in the plus icon (+), then go to <em>Add Other...</em> and locate and select the
<code>bin/assets/</code> folder and <em>Finish</em>. That should be enough.</p>
<h4>compile-all.sh script</h4>
<p>This script can be used to build the SDL2 and eepp as two fat static libraries
with arm64 and x86_64 architectures in it (arm64 for iPhone/iPad and x86_64 for
the simulators). To generate a release build pass <code>config=release_arm64</code> as a parameter
for the script (<code>sh compile-all.sh config=release_arm64</code>). The built files will be
located in <code>libs/ios/</code>, as <code>libSDL2.a</code> and <code>libeepp.a</code> (or <code>libeepp-debug.a</code> for
debug build). This two files can be integrated in your project.</p>
<h3>emscripten</h3>
<p>You will first need to <a href="https://emscripten.org/docs/getting_started/downloads.html">download and install emscripten</a>.
Then there's a script for building the <strong>emscripten</strong> project in
<code>projects/emscripten/make.sh</code>. Before running this script remember to set the
emsdk environment, it should be something like: <code>source /path/to/emsdk/emsdk_env.sh</code>.
That should be enough in <strong>GNU/Linux</strong> or <strong>macOS</strong> ( only tested this on GNU/Linux ).</p>
<h2>How to run the demos and tools?</h2>
<p>All the binaries are located at the <code>bin</code> directory after built. The binaries require two files:
the eepp library and the SDL2 library. The eepp library will be located in <code>libs/{OS}/</code>. The build
script will try to symlink the eepp library into <code>bin</code>, if that fails it should be copied or
symlinked manually. Regarding the SDL2 library is not provided in the repository, so in order to run
the demos you'll need to download the correct SDL2 library OS version and architecture.</p>
<h2>Author comment</h2>
<p>The library has been being developed for several years, it suffered many changes
since its beginnings, I'm making any changes that I find necessary to improve
it, so the API is still not totally stable (but close to be).
It's being used in several applications oriented to publicity campaigns mostly
developed for Android devices and Windows PCs.</p>
<p>I personally never had the time to use it to develop a complex game with the
library ( several frustrated projects ), but I made several UI oriented games
for clients.</p>
<p>The current project focus is on the UI module. And I'll continue working
putting my focus on this.</p>
<p>The plan is to provide an alternative UI toolkit fully hardware accelerated
similar to the Android toolkit but simpler ( as in easy to use ) and also
oriented to desktop apps.</p>
<p>Audio and Network modules were based the modules in SFML with several important
differences mentioned above.</p>
<p>I like to use what's well done and fits my needs, but since I have my personal
views on how to implement some things I prefer to take the code, to have full
control over it.</p>
<p>Also many ideas were/are taken from other projects. Some I can think about:
<em>cocos2d-x</em>, <em>raylib</em>, <em>Android SDK</em>, <em>libGDX</em>, <em>Godot</em>, <em>XNA</em>, <em>LÖVE</em>, and many
other projects.</p>
<p>If all this sounds interesting to you for some crazy reason, contact me and let
me know if I can help you to get into the library, and may be if you want, you
can contribute to it in the future. This project needs <em>contributors</em> more than
anything else.</p>
<p>The current state of the library is decent. In terms of features should be in a
similar position than the most used 2D game engines out there. But lacks of
course of the support+community that you can get from <em>Godot</em> or <em>cocos2d-x</em> to
mention a couple.</p>
<p>The main idea of this library is to focus on a better general approach to
develop heavily UI based apps and games than the other options, with cleaner
code and implementation.</p>
<p>The main reason I developed the library is for <em>fun</em> and to <em>learn</em> new
technologies. I love spending time working on the library, but I know there's
probably no real reason to develop something like this with the immense number
of similar alternatives.</p>
<p>Regarding the code quality: this project started very long time ago and suffered
many modifications over time. A good chunk of the code base still uses old C++
practices (for example: raw pointers, own implementation of thread, mutex, etc).
Some of these things can be &quot;modernized&quot;, but, others don't make much sense or
overhauling them would take too much time to justify the effort. I'm working on
&quot;modernizing&quot; some parts of the code, and new code usually tends to look more
modern.</p>
<h3>Plans/ideas for the future</h3>
<p>Keep improving the UI system, adding new widgets and layouts and improving the CSS support.</p>
<p>Simplify and improve the UI widgets skinning/theming support.</p>
<p>Improve/create documentation for the UI module.</p>
<p>Add more examples and some tools.</p>
<p>Add scripting support, but first I would like to stabilize the library, but I'm getting there.</p>
<p>Add 2D skeletal animations support ( probably Spine2D, shouldn't be much work to implement ).</p>
<p>Probably deprecate the Maps module, since I will focus my efforts on the UI system.</p>
<h2>Acknowledgements</h2>
<h3>Special thanks to</h3>
<ul>
<li><p>Sean Barrett for stb_image and all the <a href="https://github.com/nothings/stb">stb</a> libraries.</p>
</li>
<li><p>Sam Latinga for <a href="https://www.libsdl.org/">Simple DirectMedia Layer</a>.</p>
</li>
<li><p>Jonathan Dummer for the <a href="https://www.lonesock.net/soil.html">Simple OpenGL Image Library</a>.</p>
</li>
<li><p>Laurent Gomila for <a href="https://www.sfml-dev.org/">SFML</a></p>
</li>
<li><p>Yuri Kobets for <a href="https://github.com/litehtml/litehtml">litehtml</a></p>
</li>
<li><p>Michael R. P. Ragazzon for <a href="https://github.com/mikke89/RmlUi">RmlUI</a></p>
</li>
<li><p>rxi for <a href="https://github.com/rxi/lite">lite</a></p>
</li>
<li><p>Andreas Kling for <a href="https://github.com/SerenityOS/serenity">SerenityOS</a></p>
</li>
<li><p>Ryan C. Gordon for <a href="https://icculus.org/mojoAL/">mojoAL</a></p>
</li>
<li><p>David Reid for <a href="https://github.com/mackron/dr_libs">dr_libs</a></p>
</li>
<li><p>Lion (Lieff) for <a href="https://github.com/lieff/minimp3">minimp3</a> and more</p>
</li>
<li><p>Lewis Van Winkle for <a href="https://github.com/codeplea/pluscallback">PlusCallback</a></p>
</li>
<li><p>Dieter Baron and Thomas Klausner for <a href="https://libzip.org/">libzip</a></p>
</li>
<li><p>Jean-loup Gailly and Mark Adler for <a href="https://zlib.net/">zlib</a></p>
</li>
<li><p>Milan Ikits and Marcelo Magallon for <a href="http://glew.sourceforge.net/">GLEW</a></p>
</li>
<li><p>Mikko Mononen for <a href="https://github.com/memononen/nanosvg">nanosvg</a></p>
</li>
<li><p>Scott Lembcke for <a href="https://github.com/slembcke/Chipmunk2D">Chipmunk2D</a></p>
</li>
<li><p>Christophe Riccio for <a href="https://github.com/g-truc/glm">glm</a></p>
</li>
<li><p>Rich Geldreich for <a href="https://github.com/richgel999/imageresampler">imageresampler</a> and <a href="https://github.com/richgel999/jpeg-compressor">jpeg-compressor</a></p>
</li>
<li><p>Arseny Kapoulkine for <a href="https://github.com/zeux/pugixml">pugixml</a></p>
</li>
<li><p>Jason Perkins for <a href="https://premake.github.io/">premake</a></p>
</li>
<li><p>Martín Lucas Golini ( me ) and all the several contributors for <a href="https://github.com/SpartanJ/SOIL2">SOIL2</a> and <a href="https://github.com/SpartanJ/efsw">efsw</a></p>
</li>
<li><p>The Xiph open source community for <a href="https://xiph.org/ogg/">libogg</a> and <a href="https://xiph.org/vorbis/">libvorbis</a></p>
</li>
<li><p>The <a href="https://github.com/ARMmbed">ARMmbed</a> community for <a href="https://tls.mbed.org/">mbed TLS</a></p>
</li>
<li><p><a href="https://github.com/kcat">kcat</a> for <a href="http://kcat.strangesoft.net/openal.html">openal-soft</a></p>
</li>
<li><p>The <a href="https://www.freetype.org/freetype2/docsindex.html">FreeType Project</a></p>
</li>
<li><p>And a <strong>lot</strong> more people!</p>
</li>
</ul>
<h2>Code License</h2>
<p><a href="http://www.opensource.org/licenses/mit-license.php">MIT License</a></p>

			</vbox>
		</ScrollView>
	</vbox>
	)xml" );

	app.getUI()->on( Event::KeyUp, [&app]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 ) {
			UIWidgetInspector::create( app.getUI() );
		}
	} );

	return app.run();
}
