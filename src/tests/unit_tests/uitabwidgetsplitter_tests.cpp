#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/uitabwidgetsplitter.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <nlohmann/json.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::UI::Tools;

class TestClient : public UITabWidgetSplitter::Client {
  public:
	int tabCreatedCount{ 0 };
	int focusChangeCount{ 0 };

	void onTabCreated( UITab*, UIWidget* ) override { tabCreatedCount++; }

	void onWidgetFocusChange( UIWidget* ) override { focusChangeCount++; }
};

UTEST( UITabWidgetSplitter, Serialization ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );

	// --- Step 1: Serialize single tab widget with a registered type ---
	UTEST_PRINT_STEP( "Serialize single tab widget with registered type" );
	{
		TestClient client;
		auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
		auto* tabWidget = splitter->createTabWidget( (Node*)app.getUI() );

		bool saved = false;
		splitter->registerWidgetType(
			"testwidget",
			{ [&]( UIWidget* ) {
				 saved = true;
				 return nlohmann::json{ { "custom_key", 42 } };
			 },
			  []( const nlohmann::json& ) -> WidgetLoadResult {
				  return { nullptr, nullptr, "" };
			  } } );

		auto* w = UIWidget::New();
		w->addClass( "testwidget" );
		splitter->createWidgetInTabWidget( tabWidget, w, "Test Tab" );

		nlohmann::json j = splitter->toJSON();

		EXPECT_TRUE( j["type"].is_string() );
		EXPECT_TRUE( j["type"] == "tabwidget" );
		EXPECT_EQ( j["files"].size(), 1UL );
		EXPECT_EQ( j["current_page"].get<int>(), 0 );
		EXPECT_TRUE( saved );

		std::string fileType = j["files"][0]["type"];
		EXPECT_TRUE( fileType == "testwidget" );
		EXPECT_EQ( j["files"][0]["custom_key"].get<int>(), 42 );
		std::string fileTitle = j["files"][0]["title"];
		EXPECT_TRUE( fileTitle == "Test Tab" );

		eeDelete( splitter );
	}

	// --- Step 2: Empty tab widget serializes empty ---
	UTEST_PRINT_STEP( "Empty tab widget serializes empty" );
	{
		TestClient client;
		auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
		splitter->createTabWidget( (Node*)app.getUI() );

		nlohmann::json j = splitter->toJSON();

		EXPECT_TRUE( j["type"].is_string() );
		EXPECT_TRUE( j["type"] == "tabwidget" );
		EXPECT_EQ( j["files"].size(), 0UL );

		eeDelete( splitter );
	}

	// --- Step 3: Unregistered widget type is skipped ---
	UTEST_PRINT_STEP( "Unregistered widget type is skipped" );
	{
		TestClient client;
		auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
		auto* tabWidget = splitter->createTabWidget( (Node*)app.getUI() );

		auto* ghost = UIWidget::New();
		ghost->addClass( "unknown_type" );
		splitter->createWidgetInTabWidget( tabWidget, ghost, "Ghost" );

		splitter->registerWidgetType(
			"known",
			{ []( UIWidget* ) { return nlohmann::json{ { "val", 1 } }; },
			  []( const nlohmann::json& ) -> WidgetLoadResult { return {}; } } );

		auto* reg = UIWidget::New();
		reg->addClass( "known" );
		splitter->createWidgetInTabWidget( tabWidget, reg, "Known" );

		nlohmann::json j = splitter->toJSON();
		EXPECT_EQ( j["files"].size(), 1UL );
		std::string regType = j["files"][0]["type"];
		EXPECT_TRUE( regType == "known" );

		eeDelete( splitter );
	}

	// --- Step 4: Serialize splitter tree ---
	UTEST_PRINT_STEP( "Serialize splitter tree" );
	{
		TestClient client;
		auto* splitter = UITabWidgetSplitter::New( &client, app.getUI() );
		auto* tabWidget = splitter->createTabWidget( (Node*)app.getUI() );

		splitter->registerWidgetType(
			"panewidget",
			{ []( UIWidget* ) { return nlohmann::json{ { "pane", true } }; },
			  []( const nlohmann::json& j ) -> WidgetLoadResult {
				  auto* w = UIWidget::New();
				  w->addClass( "panewidget" );
				  return { w, nullptr, j.value( "title", "" ) };
			  } } );

		auto* w1 = UIWidget::New();
		w1->addClass( "panewidget" );
		splitter->createWidgetInTabWidget( tabWidget, w1, "First" );

		splitter->splitTabWidget( SplitDirection::Right, tabWidget );
		auto* secondTab = splitter->getTabWidgets()[1];

		auto* w2 = UIWidget::New();
		w2->addClass( "panewidget" );
		splitter->createWidgetInTabWidget( secondTab, w2, "Second" );

		nlohmann::json j = splitter->toJSON();

		EXPECT_TRUE( j["type"].is_string() );
		EXPECT_TRUE( j["type"] == "splitter" );
		EXPECT_TRUE( j.contains( "first" ) );
		EXPECT_TRUE( j.contains( "last" ) );
		EXPECT_TRUE( j.contains( "orientation" ) );
		EXPECT_TRUE( j.contains( "split" ) );

		std::string firstType = j["first"]["type"];
		EXPECT_TRUE( firstType == "tabwidget" );
		std::string lastType = j["last"]["type"];
		EXPECT_TRUE( lastType == "tabwidget" );
		EXPECT_EQ( j["first"]["files"].size(), 1UL );
		EXPECT_EQ( j["last"]["files"].size(), 1UL );

		eeDelete( splitter );
	}

	// --- Step 5: Round-trip serialize/deserialize single tab widget ---
	UTEST_PRINT_STEP( "Round-trip serialize/deserialize single tab widget" );
	{
		TestClient client1;
		auto* splitter1 = UITabWidgetSplitter::New( &client1, app.getUI() );
		auto* tab1 = splitter1->createTabWidget( (Node*)app.getUI() );

		splitter1->registerWidgetType(
			"mywidget",
			{ []( UIWidget* ) { return nlohmann::json{ { "data", "hello" } }; },
			  []( const nlohmann::json& j ) -> WidgetLoadResult {
				  auto* w = UIWidget::New();
				  w->addClass( "mywidget" );
				  return { w, nullptr, j.value( "title", "" ) };
			  } } );

		auto* wa = UIWidget::New();
		wa->addClass( "mywidget" );
		splitter1->createWidgetInTabWidget( tab1, wa, "Tab A" );

		auto* wb = UIWidget::New();
		wb->addClass( "mywidget" );
		splitter1->createWidgetInTabWidget( tab1, wb, "Tab B" );

		tab1->setTabSelected( 1 );
		nlohmann::json saved = splitter1->toJSON();

		TestClient client2;
		auto* splitter2 = UITabWidgetSplitter::New( &client2, app.getUI() );
		auto* tab2 = splitter2->createTabWidget( (Node*)app.getUI() );

		splitter2->registerWidgetType(
			"mywidget",
			{ []( UIWidget* ) { return nlohmann::json::object(); },
			  []( const nlohmann::json& j ) -> WidgetLoadResult {
				  auto* w = UIWidget::New();
				  w->addClass( "mywidget" );
				  return { w, nullptr, j.value( "title", "" ) };
			  } } );

		splitter2->fromJSON( saved );

		EXPECT_EQ( splitter2->getTabWidgets().size(), 1UL );
		EXPECT_EQ( tab2->getTabCount(), (Uint32)2 );
		EXPECT_EQ( client2.tabCreatedCount, 2 );
		EXPECT_EQ( tab2->getTabSelectedIndex(), (Uint32)1 );

		eeDelete( splitter1 );
		eeDelete( splitter2 );
	}

	// --- Step 6: Round-trip with splitter ---
	UTEST_PRINT_STEP( "Round-trip with splitter" );
	{
		TestClient client1;
		auto* splitter1 = UITabWidgetSplitter::New( &client1, app.getUI() );
		auto* tab1 = splitter1->createTabWidget( (Node*)app.getUI() );

		auto onSave = []( UIWidget* ) { return nlohmann::json::object(); };
		auto onLoad = []( const nlohmann::json& j ) -> WidgetLoadResult {
			auto* w = UIWidget::New();
			w->addClass( "splitwidget" );
			return { w, nullptr, j.value( "title", "" ) };
		};
		splitter1->registerWidgetType( "splitwidget", { onSave, onLoad } );

		auto* w1 = UIWidget::New();
		w1->addClass( "splitwidget" );
		splitter1->createWidgetInTabWidget( tab1, w1, "Pane 1" );

		splitter1->splitTabWidget( SplitDirection::Bottom, tab1 );
		auto* secondTab = splitter1->getTabWidgets()[1];

		auto* w2 = UIWidget::New();
		w2->addClass( "splitwidget" );
		splitter1->createWidgetInTabWidget( secondTab, w2, "Pane 2" );

		nlohmann::json saved = splitter1->toJSON();

		TestClient client2;
		auto* splitter2 = UITabWidgetSplitter::New( &client2, app.getUI() );
		splitter2->createTabWidget( (Node*)app.getUI() );
		splitter2->registerWidgetType( "splitwidget", { onSave, onLoad } );

		splitter2->fromJSON( saved );

		EXPECT_EQ( splitter2->getTabWidgets().size(), 2UL );
		EXPECT_EQ( splitter2->getTabWidgets()[0]->getTabCount(), (Uint32)1 );
		EXPECT_EQ( splitter2->getTabWidgets()[1]->getTabCount(), (Uint32)1 );

		eeDelete( splitter1 );
		eeDelete( splitter2 );
	}

	// --- Step 7: Preserve split partition ---
	UTEST_PRINT_STEP( "Preserve split partition" );
	{
		TestClient client1;
		auto* splitter1 = UITabWidgetSplitter::New( &client1, app.getUI() );
		auto* tab1 = splitter1->createTabWidget( (Node*)app.getUI() );

		splitter1->registerWidgetType(
			"partwidget",
			{ []( UIWidget* ) { return nlohmann::json::object(); },
			  []( const nlohmann::json& j ) -> WidgetLoadResult {
				  auto* w = UIWidget::New();
				  w->addClass( "partwidget" );
				  return { w, nullptr, j.value( "title", "" ) };
			  } } );

		auto* w1 = UIWidget::New();
		w1->addClass( "partwidget" );
		splitter1->createWidgetInTabWidget( tab1, w1, "P1" );

		splitter1->splitTabWidget( SplitDirection::Bottom, tab1 );
		auto* secondTab = splitter1->getTabWidgets()[1];

		auto* w2 = UIWidget::New();
		w2->addClass( "partwidget" );
		splitter1->createWidgetInTabWidget( secondTab, w2, "P2" );

		UISplitter* spl = tab1->getParent()->asType<UISplitter>();
		spl->setSplitPartition( StyleSheetLength( "30%" ) );

		nlohmann::json saved = splitter1->toJSON();
		EXPECT_TRUE( saved.contains( "split" ) );
		EXPECT_TRUE( saved["split"].is_string() );

		TestClient client2;
		auto* splitter2 = UITabWidgetSplitter::New( &client2, app.getUI() );
		splitter2->createTabWidget( (Node*)app.getUI() );

		splitter2->registerWidgetType(
			"partwidget",
			{ []( UIWidget* ) { return nlohmann::json::object(); },
			  []( const nlohmann::json& j ) -> WidgetLoadResult {
				  auto* w = UIWidget::New();
				  w->addClass( "partwidget" );
				  return { w, nullptr, j.value( "title", "" ) };
			  } } );

		splitter2->fromJSON( saved );

		EXPECT_EQ( splitter2->getTabWidgets().size(), 2UL );

		eeDelete( splitter1 );
		eeDelete( splitter2 );
	}
}
