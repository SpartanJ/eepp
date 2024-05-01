R"html(
<style>
TextInput#search_find,
TextInput#search_replace,
TextInput#locate_find,
TextInput#global_search_find,
TextInput.small_input,
.search_str {
	padding-top: 0;
	padding-bottom: 0;
	font-family: monospace;
}
#search_bar,
#global_search_bar,
#locate_bar {
	padding-left: 4dp;
	padding-right: 4dp;
	padding-bottom: 3dp;
	margin-bottom: 2dp;
	margin-top: 2dp;
}
.close_button {
	width: 12dp;
	height: 12dp;
	border-radius: 6dp;
	background-color: var(--icon-back-hover);
	foreground-image: poly(line, var(--icon-line-hover), "0dp 0dp, 6dp 6dp"), poly(line, var(--icon-line-hover), "6dp 0dp, 0dp 6dp");
	foreground-position: 3dp 3dp, 3dp 3dp;
	transition: all 0.15s;
}
.close_button:hover {
	background-color: var(--icon-back-alert);
}
#settings {
	color: var(--floating-icon);
	font-family: icon;
	font-size: 16dp;
	margin-top: 4dp;
	margin-right: 12dp;
	transition: all 0.15s;
}
#settings:hover {
	color: var(--primary);
}
RelativeLayout > #doc_info {
	background-color: var(--back);
	margin-bottom: 22dp;
	margin-right: 22dp;
	border-radius: 8dp;
	padding: 6dp;
	opacity: 0.8;
	layout-gravity: bottom|right;
	layout-height: wrap_content;
	font-size: 1rem;
}
StatusBar > #doc_info {
	background-color: transparent;
	margin-bottom: 0dp;
	margin-right: 0dp;
	border-radius: 0dp;
	border: 0dp solid transparent;
	padding: 0dp 4dp 0dp 4dp;
	opacity: 1;
	layout-gravity: center;
	layout-height: match_parent;
	font-size: 10dp;
}
#doc_info {
	color: var(--font);
}
#search_find.error,
#search_replace.error {
	border-color: #ff4040;
}
TableView#locate_bar_table > tableview::row > tableview::cell:nth-child(2),
TableView#locate_bar_table > tableview::row > tableview::cell:nth-child(3) {
	color: var(--font-hint);
}
TableView#locate_bar_table > tableview::row:selected > tableview::cell:nth-child(2),
TableView#locate_bar_table > tableview::row:selected > tableview::cell:nth-child(3) {
	color: var(--font);
}
.search_tree treeview::cell {
	font-family: monospace;
}
#global_search_history {
	padding-top: 0dp;
	padding-bottom: 0dp;
}
.doc_alert {
	padding: 16dp;
	border-width: 2dp;
	border-radius: 4dp;
	border-color: var(--primary);
	background-color: var(--back);
	margin-right: 24dp;
	margin-top: 24dp;
	cursor: arrow;
}
#image_container {
	background-color: #00000066;
}
#image_close {
	color: var(--floating-icon);
	font-family: icon;
	font-size: 22dp;
	margin-top: 32dp;
	margin-right: 22dp;
}
#global_search_layout {
	background-color: var(--back);
}
#global_search_layout > .status_box,
#global_search_layout > .replace_box {
	padding: 4dp;
}
.notification {
	background-color: var(--button-back);
	border-color: var(--button-border);
	border-width: 1dp;
	border-radius: 8dp;
	color: var(--font);
	padding: 4dp;
	min-height: 48dp;
	margin-bottom: 8dp;
	opacity: 0.8;
}
#panel tab::icon {
	margin-left: 0dp;
	margin-right: 0dp;
}
#panel tab:selected {
	border: none;
	background-color: transparent;
	background-image: rectangle(solid, var(--primary), 0º, 1dp);
	background-size: 80% 1dp;
	background-position-x: center;
	background-position-y: 90%;
	border-left-width: 0dp;
	border-right-width: 0dp;
	border-top-width: 0dp;
	border-top-left-radius: 0dp;
	border-top-right-radius: 0dp;
}
#check_for_updates .check_at_startup {
	margin: 6dp 0dp 6p 0dp;
}
#project_view_empty {
	padding-top: 8dp;
}
#project_view_empty > * {
	margin-left: 8dp;
	margin-right: 8dp;
}
#status_bar {
	background-color: var(--list-back);
	min-height: 16dp;
}
#status_bar > .status_but {
	padding: 0dp 5dp 0dp 4dp;
	background-color: var(--list-back);
	border-radius: 0dp;
	border-left-color: transparent;
	border-top-color: transparent;
	border-bottom-color: transparent;
	border-right-color: var(--tab-line);
	font-size: 10dp;
	layout-height: match_parent;
	layout-gravity: top;
}
#status_bar > .status_but:hover {
	background-color: var(--item-hover);
}
#status_bar > .status_but.selected {
	background-color: var(--primary);
}
.vertical_bar {
	background-color: var(--list-back);
}
.vertical_bar PushButton {
	padding: 2dp;
	background-color: var(--list-back);
	border-radius: 0dp;
	border-width: 0dp;
}
.vertical_bar PushButton:hover {
	background-color: var(--item-hover);
}
.vertical_bar PushButton.selected {
	background-color: var(--primary);
}
#panel > tabwidget::container > * {
	background-color: var(--list-back);
}
.settings_panel {
	padding: 4dp;
}
.settings_panel .title {
	font-size:17dp;
}
.settings_panel .subtitle {
	font-size:15dp;
	margin-top: 12dp;
}
.settings_panel > .subtitle {
	margin-top: 4dp;
}
.settings_panel .advance_opt {
	font-size:15dp;
}
.settings_panel .separator {
	margin-top: 4dp;
	background-color: var(--font-hint);
}
.settings_panel PushButton {
	padding-top: 2dp;
	padding-bottom: 2dp;
}
.settings_panel .build_step,
.settings_panel .os_select {
	background-color: var(--list-back);
	border-width: 1dp;
	border-color: var(--button-border);
	padding: 4dp;
	margin-top: 4dp;
}
.settings_panel .build_step > LinearLayout > LinearLayout {
	margin-bottom: 3dp;
}
.settings_panel .build_step TextInput {
	padding: 2dp 4dp 2dp 4dp;
}
.settings_panel .add_build_step {
	margin-top: 4dp;
}
.settings_panel .build_steps {
	margin-bottom: 4dp;
}
.settings_panel .build_step:first-child .move_up,
.settings_panel .build_step:first-child .remove_item {
	enabled: false;
}
.settings_panel .build_step:last-child .move_down {
	enabled: false;
}
.settings_panel .build_step > .details {
	margin-top: 2dp;
}
.settings_panel .build_step > .header > PushButton {
	margin-left: 2dp;
}
.settings_panel .build_step > .header > .remove_item:disabled,
.settings_panel .build_step > .header > .move_up:disabled,
.settings_panel .build_step > .header > .move_down:disabled {
	tint: var(--icon);
}
.settings_panel .os_select > CheckBox {
	margin-right: 8dp;
}
.settings_panel .save_cont > PushButton {
	margin: 8dp 0dp 0dp 4dp;
	padding: 4dp 16dp 4dp 16dp;
}
.settings_panel .build_types > DropDownList,
.settings_panel .output_parser > DropDownList,
.settings_panel .span {
	margin-top: 4dp;
}
.settings_panel #build_type_list,
.settings_panel #build_type_add {
	margin-right: 4dp;
}
.settings_panel .build_types_cont {
	row-valign: center;
}
.settings_panel .inner_box {
	visible: false;
}
.settings_panel .advanced_options {
	margin-top: 12dp;
	border-radius: 4dp;
	padding: 4dp;
	background-color: rgba(0,0,0, 0.1);
}
.settings_panel .advanced_options .title {
	padding-bottom: 4dp;
	border-radius: 4dp;
	background-color: transparent;
	transition: background-color 100ms;
}
.settings_panel .advanced_options .title:hover {
	background-color: var(--primary);
}
.settings_panel .output_parser .output_parser_rules {
	background-color: var(--list-back);
}
.settings_panel .output_parser .output_parser_rules {
	margin-top: 4dp;
}
.settings_panel .output_parser .output_parser_rules > TextView {
	padding-left: 4dp;
	min-height: 24dp;
	layout-gravity: center;
}
.settings_panel .output_parser .output_parser_rules > TextView:first-child {
	border-width: 1dp;
	border-right-color: var(--button-border);
}
.settings_panel .output_parser .output_parser_rules > PushButton {
	layout-gravity: center;
}
.settings_panel .details_but {
	icon: icon(arrow-up-s, 10dp);
	inner-widget-orientation: widgettextboxicon;
}
.settings_panel .details_but.contracted {
	icon: icon(arrow-down-s, 10dp);
}
.settings_panel > .advanced_options > .title > Image {
	icon: icon(arrow-down-s, 24dp);
}
.settings_panel > .advanced_options > .title > Image.expanded {
	icon: icon(arrow-up-s, 24dp);
}
.settings_panel > .advanced_options > LinearLayout.inner_box.visible {
	visible: true;
}
.settings_panel .buttons_box {
	margin-left:4dp;
	layout-gravity: center_horizontal|top;
}
.settings_panel .buttons_box > * {
	margin-bottom: 4dp;
}
.settings_panel .stack_margins > * {
	margin-right:4dp;
}
.settings_panel TableView {
	margin-top: 4dp;
}
.custom_output_parser_cont > * {
	margin-bottom: 4dp;
	padding-top: 4dp;
}
.custom_output_parser_cont > .capture_positions_cont {
	border: 1dp solid var(--tab-line);
	padding: 4dp;
}
.custom_output_parser_cont > .capture_positions_cont > * {
	padding: 2dp;
}
.theme-error > tableview::cell::text,
.theme-error > treeview::cell::text,
.theme-error > listview::cell::text,
.error {
	color: var(--theme-error);
}

.theme-warning > tableview::cell::text,
.theme-warning > treeview::cell::text,
.theme-warning > listview::cell::text,
.warning {
	color: var(--theme-warning);
}

.theme-success > tableview::cell::text,
.theme-success > treeview::cell::text,
.theme-success > listview::cell::text,
.success {
	color: var(--theme-success);
}

.theme-none > tableview::cell::text,
.theme-none > treeview::cell::text,
.theme-none > listview::cell::text,
.none {
	color: #d48838;
}

Anchor.success:hover,
Anchor.error:hover {
	color: var(--primary);
}
.status_build_output_cont > SelectButton {
	font-size: 11dp;
	padding: 2dp 8dp 2dp 8dp;
	border-radius: 0dp;
	border-width: 1dp;
	border-color: transparent;
	transition: all 0.2;
}
.status_build_output_cont > SelectButton:hover {
	border-width: 1dp;
	border-color: var(--primary);
}
#build_output_issues TableView::cell.theme-warning > TableView::cell::icon {
	tint: var(--theme-warning);
}
#build_output_issues TableView::cell.theme-error > TableView::cell::icon {
	tint: var(--theme-error);
}
#build_output_issues TableView::row:selected TableView::cell.theme-error > TableView::cell::text {
	color: var(--font);
}
#build_output_issues TableView::row:selected TableView::cell.theme-error > TableView::cell::icon {
	tint: var(--font);
}
.texture-preview {
	border: 1dp solid var(--list-back);
}
#code_container TabWidget {
	max-tab-width: 200dp;
}
#code_container Tab > Tab::Text {
	text-overflow: ellipsis;
}
#code_container Tab > Tab::close {
	opacity: 0;
}
#code_container Tab:selected > Tab::close,
#code_container Tab:hover > Tab::close {
	opacity: 1;
}
#project_view ScrollBar {
	opacity: 0;
	transition: opacity 0.15;
}
#project_view:hover ScrollBar,
#project_view ScrollBar.dragging,
#project_view ScrollBar:focus-within {
	opacity: 1;
}
#code_container TabWidget::TabBar ScrollBarMini {
	opacity: 0;
	transition: opacity 0.15;
}
#code_container TabWidget::TabBar:hover ScrollBarMini,
#code_container TabWidget::TabBar ScrollBarMini.dragging,
#code_container TabWidget::TabBar ScrollBarMini:focus-within {
	opacity: 1;
}
.notbold {
	font-style: normal;
}
.bold {
	font-style: bold;
}
</style>
)html"

R"html(
<vbox id="" lw="mp" lh="mp">
<MenuBar id="main_menubar" visible="false">
	<Menu id="menubar_file" text="@string(file, File)" nomenu="true" />
	<Menu id="menubar_edit" text="@string(edit, Edit)" nomenu="true" />
	<Menu id="menubar_view" text="@string(view, View)" nomenu="true" />
	<Menu id="menubar_document" text="@string(document, Document)" nomenu="true" />
	<Menu id="menubar_terminal" text="@string(terminal, Terminal)" nomenu="true" />
	<Menu id="menubar_project" text="@string(project, Project)" nomenu="true" />
	<Menu id="menubar_tools" text="@string(tools, Tools)" nomenu="true" />
	<Menu id="menubar_window" text="@string(window, Window)" nomenu="true" />
	<Menu id="menubar_help" text="@string(help, Help)" nomenu="true" />
</MenuBar>
<MainLayout id="main_layout" lw="mp" lh="0" lw8="1">
<Splitter id="project_splitter" lw="mp" lh="mp">
	<TabWidget id="panel" tabbar-hide-on-single-tab="true" tabbar-allow-rearrange="true" min-tab-width="32dp" max-tab-width="32dp">
		<RelativeLayout id="project_view_cont" lw="mp" lh="mp">
			<TreeView id="project_view" lw="mp" lh="mp" />
			<vbox id="project_view_empty" lg="top|center_horizontal" lw="mp" lh="wc">
				<TextView text-align="center" lw="mp" lg="center" text='@string(you_have_not_yet_opened_a_folder, "You have not yet opened a folder.")' word-wrap="true"  />
				<PushButton lw="mp" lg="center" id="open_folder" text='@string(open_folder, "Open Folder")' margin-top="4dp" />
				<PushButton lw="mp" lg="center" id="open_recent_folder" text='@string(open_recent_folder_ellipsis, "Open Recent Folder...")' margin-top="4dp" />
			</vbox>
		</RelativeLayout>
		<Tab id="treeview_tab" text='@string("project", "Project")' owns="project_view_cont" text-as-fallback="true" icon="icon(folder-open, 12dp)" />
	</TabWidget>
	<vbox>
		<Splitter id="main_splitter" lw="mp" lh="0" lw8="1" orientation="vertical">
			<RelativeLayout id="main_splitter_cont">
				<vbox id="code_container" lw="mp" lh="mp"></vbox>
				<TextView id="doc_info" enabled="false" />
				<RelativeLayout id="image_container" lw="mp" lh="mp" visible="false" enabled="false">
					<Image lw="mp" lh="mp" scaleType="fit_inside" gravity="center" enabled="false" lg="center" />
					<TextView id="image_close" lw="wc" lh="wc" text="&#xeb99;" lg="top|right" enabled="false" />
					<Loader id="image_loader" lw="64dp" lh="64dp" outline-thickness="6dp" lg="center" visible="false" />
				</RelativeLayout>
				<vbox id="notification_center" lw="256dp" lh="wc"
					  lg="right|bottom" margin-right="22dp" margin-bottom="56dp">
				</vbox>
			</RelativeLayout>
		</Splitter>
		<searchbar id="search_bar" lw="mp" lh="wc">
			<vbox lw="wc" lh="wc" margin-right="4dp">
				<TextView lw="wc" lh="18dp" text='@string("find_text", "Find:")' margin-bottom="2dp" />
				<TextView lw="wc" lh="18dp" text='@string("replace_with_text", "Replace with:")' />
			</vbox>
			<vbox lw="0" lw8="1" lh="wc" margin-right="4dp">
				<TextInput id="search_find" lw="mp" lh="18dp" padding="0" margin-bottom="2dp" />
				<TextInput id="search_replace" lw="mp" lh="18dp" padding="0" />
			</vbox>
			<vbox lw="wc" lh="wc" margin-right="4dp">
				<CheckBox id="case_sensitive" lw="wc" lh="wc" text='@string(case_sensitive, "Case sensitive")' selected="false" />
				<CheckBox id="lua_pattern" lw="wc" lh="wc" text='@string(lua_pattern, "Lua Pattern")' selected="false" />
			</vbox>
			<vbox lw="wc" lh="wc" margin-right="4dp">
				<CheckBox id="whole_word" lw="wc" lh="wc" text='@string(match_whole_word, "Match Whole Word")' selected="false" />
				<CheckBox id="escape_sequence" lw="wc" lh="wc" text='@string(use_escape_sequences, "Use escape sequences")' selected="false" tooltip='@string(escape_sequence_tooltip, "Replace \\, \\t, \\n, \\r and \\uXXXX (Unicode characters) with the corresponding control")' />
			</vbox>
			<vbox lw="wc" lh="wc">
				<hbox lw="wc" lh="wc" margin-bottom="2dp">
					<PushButton id="find_prev" lw="wc" lh="18dp" text='@string(previous, "Previous")' margin-right="4dp" />
					<PushButton id="find_next" lw="wc" lh="18dp" text='@string(next, "Next")' margin-right="4dp" />"
					<PushButton id="select_all" lw="wc" lh="18dp" text='@string(select_all, "Select All")' />
					<RelativeLayout lw="0" lw8="1" lh="18dp">
						<Widget id="searchbar_close" class="close_button" lw="wc" lh="wc" lg="center_vertical|right" margin-right="2dp" />
					</RelativeLayout>
				</hbox>
				<hbox lw="wc" lh="wc">
					<PushButton id="replace" lw="wc" lh="18dp" text='@string(replace, "Replace")' margin-right="4dp" />
					<PushButton id="replace_find" lw="wc" lh="18dp" text='@string(replace_and_find, "Replace & Find")' margin-right="4dp" />
					<PushButton id="replace_all" lw="wc" lh="18dp" text='@string(replace_all, "Replace All")' />
				</hbox>
			</vbox>
		</searchbar>
		<locatebar id="locate_bar" lw="mp" lh="wc" visible="false">
			<TextInput id="locate_find" lw="0" lw8="1" lh="18dp" padding="0" margin-bottom="2dp" margin-right="4dp" hint='@string(type_to_locate, "Type to Locate")' />
			<Widget id="locatebar_close" class="close_button" lw="wc" lh="wc" lg="center_vertical|right"/>
		</locatebar>
		<globalsearchbar id="global_search_bar" lw="mp" lh="wc">
			<hbox lw="mp" lh="wc">
				<vbox lw="wc" lh="wc">
					<TextView lh="18dp" text='@string(search_for, "Search for:")' margin-right="4dp" margin-bottom="2dp" />
					<TextView lh="18dp" text='@string(path_filters, "Path Filters:")' margin-right="4dp" />
				</vbox>
				<vbox lw="0" lw8="1" lh="wc">
					<TextInput id="global_search_find" lw="mp" lh="wc" lh="18dp" padding="0" margin-bottom="2dp" />
					<hbox lw="mp" lh="wc" margin-bottom="2dp">
						<TextInput id="global_search_where" lw="0" lw8="1" lh="wc" lh="18dp" padding="0" margin-right="4dp"
								   hint='@string(search_where_example, "e.g. *.ts, src/**/include, -src/**/exclude")' hint-display="focus" />
						<PushButton id="global_search_filters_menu_button" lw="wc" lh="mp" text="..." />
					</hbox>
					<StackLayout lw="mp" lh="wc" margin-bottom="2dp">
						<CheckBox id="case_sensitive" text='@string(case_sensitive, "Case sensitive")' selected="true" margin-right="8dp" />
						<CheckBox id="whole_word" text='@string(match_whole_word, "Match Whole Word")' selected="false" margin-right="8dp" />
						<CheckBox id="lua_pattern" text='@string(lua_pattern, "Lua Pattern")' selected="false" margin-right="8dp" />
						<CheckBox id="escape_sequence" text='@string(use_escape_sequences, "Use escape sequences")' margin-right="8dp" selected="false"
								  tooltip='@string(escape_sequence_tooltip, "Replace \\, \t, \n, \r and \uXXXX (Unicode characters) with the corresponding control")' />
					</StackLayout>
					<hbox lw="mp" lh="wc">
						<TextView text='@string(history, "History:")' margin-right="4dp" lh="18dp" focusable="false" />
						<DropDownList id="global_search_history" lw="0" lh="18dp" lw8="1" margin-right="4dp" />
						<PushButton id="global_search_clear_history" lw="wc" lh="18dp" text='@string(clear_history, "Clear History")' margin-right="4dp" />
						<PushButton id="global_search" lw="wc" lh="18dp" text='@string(search, "Search")' margin-right="4dp" />
						<PushButton id="global_search_replace" lw="wc" lh="18dp" text='@string(search_and_replace, "Search & Replace")' />
					</hbox>
				</vbox>
				<Widget id="global_searchbar_close" class="close_button" lw="wc" lh="wc" lg="top|right" margin-left="4dp" margin-top="4dp" />
			</hbox>
		</globalsearchbar>
		<statusbar lw="mp" lh="wc" id="status_bar">
			<PushButton class="status_but" id="status_locate_bar" text="@string(locate, Locate)" icon="icon(search-fuzzy, 11dp)" />
			<PushButton class="status_but" id="status_global_search_bar" text="@string(search, Search)" icon="icon(file-search, 11dp)" />
			<PushButton class="status_but" id="status_terminal" text="@string(terminal, Terminal)" icon="icon(terminal, 11dp)" />
			<PushButton class="status_but" id="status_build_output" text="@string(build, Build)" icon="icon(symbol-property, 11dp)"  />
			<PushButton class="status_but" id="status_app_output" text="@string(app_output, App Output)" icon="icon(output, 11dp)"  />
			<Widget class="status_sep" lw="0" lw8="1" lh="1dp" />
		</statusbar>
	</vbox>
</Splitter>
<TextView id="settings" text="&#xf0e9;" lg="top|right" />
</MainLayout>
</vbox>
)html"
