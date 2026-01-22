R"html(
<style>
<![CDATA[
TextInput#search_find,
TextInput#search_replace,
TextInput#locate_find,
TextInput#global_search_find,
TextInput#global_search_where,
TextInput.small_input,
.search_str {
	padding-top: 0;
	padding-bottom: 0;
	font-family: monospace;
}
#global_search_bar,
#locate_bar {
	padding-left: 4dp;
	padding-right: 4dp;
	padding-bottom: 3dp;
	margin-bottom: 2dp;
	margin-top: 2dp;
}
#search_bar {
	padding-left: 4dp;
	padding-right: 4dp;
	padding-bottom: 1dp;
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
	tint: var(--floating-icon);
	margin-top: 4dp;
	margin-right: 12dp;
	transition: all 0.15s;
	src: url("data:image/svg,<svg viewBox='0 0 24 24' width='24' height='24' fill='white'><path d='M2.13127 13.6308C1.9492 12.5349 1.95521 11.434 2.13216 10.3695C3.23337 10.3963 4.22374 9.86798 4.60865 8.93871C4.99357 8.00944 4.66685 6.93557 3.86926 6.17581C4.49685 5.29798 5.27105 4.51528 6.17471 3.86911C6.9345 4.66716 8.0087 4.99416 8.93822 4.60914C9.86774 4.22412 10.3961 3.23332 10.369 2.13176C11.4649 1.94969 12.5658 1.9557 13.6303 2.13265C13.6036 3.23385 14.1319 4.22422 15.0612 4.60914C15.9904 4.99406 17.0643 4.66733 17.8241 3.86975C18.7019 4.49734 19.4846 5.27153 20.1308 6.1752C19.3327 6.93499 19.0057 8.00919 19.3907 8.93871C19.7757 9.86823 20.7665 10.3966 21.8681 10.3695C22.0502 11.4654 22.0442 12.5663 21.8672 13.6308C20.766 13.6041 19.7756 14.1324 19.3907 15.0616C19.0058 15.9909 19.3325 17.0648 20.1301 17.8245C19.5025 18.7024 18.7283 19.4851 17.8247 20.1312C17.0649 19.3332 15.9907 19.0062 15.0612 19.3912C14.1316 19.7762 13.6033 20.767 13.6303 21.8686C12.5344 22.0507 11.4335 22.0447 10.3691 21.8677C10.3958 20.7665 9.86749 19.7761 8.93822 19.3912C8.00895 19.0063 6.93508 19.333 6.17532 20.1306C5.29749 19.503 4.51479 18.7288 3.86862 17.8252C4.66667 17.0654 4.99367 15.9912 4.60865 15.0616C4.22363 14.1321 3.23284 13.6038 2.13127 13.6308ZM11.9997 15.0002C13.6565 15.0002 14.9997 13.657 14.9997 12.0002C14.9997 10.3433 13.6565 9.00018 11.9997 9.00018C10.3428 9.00018 8.99969 10.3433 8.99969 12.0002C8.99969 13.657 10.3428 15.0002 11.9997 15.0002Z'></path></svg>");
	scale-type: expand;
}
#settings:hover {
	tint: var(--primary);
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
	padding: 0dp 6dp 0dp 6dp;
	opacity: 1;
	layout-gravity: center;
	layout-height: match_parent;
	font-size: 10dp;
}
#doc_info {
	color: var(--font);
	border-right-color: transparent;
}
.status_but + #doc_info {
	border-right-width: 1dp;
	border-right-color: var(--tab-line);
}
#search_find.error,
#search_replace.error {
	border-color: #ff4040;
}
TableView#locate_bar_table > tableview::row > tableview::cell:nth-child(2) > tableview::cell::text,
TableView#locate_bar_table > tableview::row > tableview::cell:nth-child(3) > tableview::cell::text {
	color: var(--font-hint);
}
TableView#locate_bar_table > tableview::row:selected > tableview::cell:nth-child(2) > tableview::cell::text,
TableView#locate_bar_table > tableview::row:selected > tableview::cell:nth-child(3) > tableview::cell::text {
	color: var(--list-row-active)
}
.search_tree treeview::cell {
	font-family: monospace;
}
.search_tree treeview::row:selected treeview::cell::text {
	color: var(--font);
}
.search_tree treeview::row:selected treeview::cell::expander {
	tint: var(--font);
}
.search_tree treeview::row:selected {
	background-color: var(--tab-hover);
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
	margin: 6dp 0dp 6dp 0dp;
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
	color: var(--font);
	tint: var(--font);
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
	color: var(--list-row-active);
	tint: var(--list-row-active);
}
#status_bar > .status_but:last-child {
	border-right-color: transparent;
}
#status_bar > #ai_assistant_but.status_but {
	padding: 0dp 4dp 0dp 6dp;
}
#status_bar > #ai_assistant_but.status_but > PushButton::icon {
	margin-right: 0dp;
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
	padding-right: 4dp;
	border-radius: 4dp;
	background-color: transparent;
	transition: background-color 100ms;
}
.settings_panel .advanced_options .title > * {
	color: var(--font);
	tint: var(--font);
}
.settings_panel .advanced_options .title:hover > * {
	color: var(--list-row-active);
	tint: var(--list-row-active);
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
.expand_status_bar_panel {
	icon: icon(arrow-up-s, 16dp);
}
.expand_status_bar_panel.expanded {
	icon: icon(arrow-down-s, 16dp);
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
	color: var(--list-row-active);
}
#build_output_issues TableView::row:selected TableView::cell.theme-error > TableView::cell::icon {
	tint: var(--list-row-active);
}
.texture-preview {
	border: 1dp solid var(--list-back);
}
.tab_widget_cont TabWidget {
	max-tab-width: 200dp;
}
.tab_widget_cont Tab > Tab::Text {
	text-overflow: ellipsis;
}
.tab_widget_cont Tab > Tab::close {
	opacity: 0;
}
.tab_widget_cont Tab:selected > Tab::close,
.tab_widget_cont Tab:hover > Tab::close {
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
.tab_widget_cont Tab > Tab::close {
	foreground-image: url("data:image/svg,<svg width='16' height='16' viewBox='0 0 16 16'><path fill='#ffffff' fill-rule='evenodd' d='M 2.3432061,13.657206 A 8.0002061,8.0002061 0 1 1 13.657206,2.3432061 8.0002061,8.0002061 0 0 1 2.3432061,13.657206 Z m 3.687,-8.6869999 a 0.75,0.75 0 0 0 -1.06,1.06 l 1.97,1.97 -1.97,1.97 a 0.75,0.75 0 1 0 1.06,1.0599999 l 1.97,-1.9699999 1.97,1.9699999 A 0.75,0.75 0 1 0 11.030206,9.9702061 l -1.9699999,-1.97 1.9699999,-1.97 a 0.75,0.75 0 1 0 -1.0599999,-1.06 l -1.97,1.97 z' /></svg>");
	foreground-tint: var(--tab-close);
	foreground-size: 10dp 10dp;
	foreground-position: center;
}
.tab_widget_cont Tab > Tab::close:hover {
	foreground-tint: var(--tab-close-hover);
}
.tab_widget_cont Tab.tab_modified > tab::close {
	foreground-image: url("data:image/svg,<svg viewBox='0 0 24 24' width='12' height='12' fill='#ffffff'><path d='M12 22C17.5228 22 22 17.5228 22 12C22 6.47715 17.5228 2 12 2C6.47715 2 2 6.47715 2 12C2 17.5228 6.47715 22 12 22Z'></path></svg>");
	foreground-tint: var(--primary);
	foreground-size: 6dp 6dp;
	foreground-position: center;
	opacity: 1;
}
.tab_widget_cont Tab.tab_modified > Tab::close:hover {
	foreground-image: url("data:image/svg,<svg width='16' height='16' viewBox='0 0 16 16'><path fill='#ffffff' fill-rule='evenodd' d='M 2.3432061,13.657206 A 8.0002061,8.0002061 0 1 1 13.657206,2.3432061 8.0002061,8.0002061 0 0 1 2.3432061,13.657206 Z m 3.687,-8.6869999 a 0.75,0.75 0 0 0 -1.06,1.06 l 1.97,1.97 -1.97,1.97 a 0.75,0.75 0 1 0 1.06,1.0599999 l 1.97,-1.9699999 1.97,1.9699999 A 0.75,0.75 0 1 0 11.030206,9.9702061 l -1.9699999,-1.97 1.9699999,-1.97 a 0.75,0.75 0 1 0 -1.0599999,-1.06 l -1.97,1.97 z' /></svg>");
	foreground-tint: var(--tab-close-hover);
	foreground-size: 10dp 10dp;
	foreground-position: center;
}
.tab_widget_cont Tab {
	text-decoration: none;
}
.tab_widget_cont Tab.tab_file_deleted {
	color: var(--theme-error);
	text-decoration: strikethrough;
}
.tab_widget_cont TabWidget::TabBar ScrollBarMini {
	opacity: 0;
	transition: opacity 0.15;
}
.tab_widget_cont TabWidget::TabBar:hover ScrollBarMini,
.tab_widget_cont TabWidget::TabBar ScrollBarMini.dragging,
.tab_widget_cont TabWidget::TabBar ScrollBarMini:focus-within {
	opacity: 1;
}
.notbold {
	font-style: normal;
}
.bold {
	font-style: bold;
}
.app_hint {
	background-color: var(--button-back);
	border-left-color: var(--button-border);
	border-top-color: var(--button-border);
	padding: 4dp 16dp 4dp 16dp;
	border-top-left-radius: 12dp;
	margin-bottom: 16dp;
	margin-right: 2dp;
	font-style: shadow;
	text-shadow-color: #323232e6;
}
#indent_tab_window ComboBox,
#indent_tab_window ListBox::item,
.indent_tab_listbox_item combobox::dropdownlist::listbox::item {
	font-family: monospace;
}
.git-stash-tooltip {
	text-align: left;
}
window::modaldialog.shadowbg {
	background-color: #00000066;
}
textview.dragged_cell {
	background-color: var(--button-back);
	border-radius: 4dp;
	padding: 4dp;
}
ImageViewer {
	display-options: name|pos|dimensions;
}
ImageViewer > TextView {
	x: 4dp;
	y: 24dp;
}
TabWidget::container > ImageViewer {
	background-color: var(--list-back);
}
TabWidget::container > ImageViewer > TextView {
	y: 4dp;
}
.audio_player {
	layout-width: 300dp;
}
.pseudo_anchor {
	tint: var(--floating-icon);
	cursor: arrow;
}
.pseudo_anchor:hover {
	tint: var(--primary);
	cursor: hand;
}

@media (prefers-color-scheme: light) {

.app_hint {
	text-shadow-color: #b4b4b4e6;
}

}
]]>
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
			<TreeViewFS id="project_view" lw="mp" lh="mp" />
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
				<vbox id="code_container" class="tab_widget_cont" lw="mp" lh="mp"></vbox>
				<TextView id="doc_info" enabled="false" />
				<RelativeLayout id="image_container" lw="mp" lh="mp" visible="false" enabled="false">
					<ImageViewer lw="mp" lh="mp" scaleType="fit_inside" gravity="center" enabled="true" lg="center" />
					<TextView id="image_close" lw="wc" lh="wc" text="&#xeb99;" lg="top|right" enabled="true" />
				</RelativeLayout>
			</RelativeLayout>
		</Splitter>
		<searchbar id="search_bar" lw="mp" lh="wc">
			<vbox lw="wc" lh="wc" margin-right="4dp" layout-gravity="center">
				<TextView lw="wc" lh="18dp" text='@string("find_text", "Find:")' margin-bottom="2dp" />
				<TextView lw="wc" lh="18dp" text='@string("replace_with_text", "Replace with:")' />
			</vbox>
			<vbox lw="0" lw8="1" lh="wc" margin-right="4dp" layout-gravity="center">
				<TextInput id="search_find" lw="mp" lh="18dp" padding="0" margin-bottom="2dp" />
				<TextInput id="search_replace" lw="mp" lh="18dp" padding="0" />
			</vbox>
			<vbox lw="wc" lh="wc" margin-right="4dp" layout-gravity="center">
				<CheckBox id="case_sensitive" lw="wc" lh="wc" text='@string(case_sensitive, "Case sensitive")' selected="false" />
				<CheckBox id="regex" lw="wc" lh="wc" text='@string(regular_expression, "Regular Expression")' selected="false" />
				<CheckBox id="lua_pattern" lw="wc" lh="wc" text='@string(lua_pattern, "Lua Pattern")' selected="false" />
			</vbox>
			<vbox lw="wc" lh="wc" margin-right="4dp" layout-gravity="center">
				<CheckBox id="whole_word" lw="wc" lh="wc" text='@string(match_whole_word, "Match Whole Word")' selected="false" />
				<CheckBox id="escape_sequence" lw="wc" lh="wc" text='@string(use_escape_sequences, "Use escape sequences")' selected="false" tooltip='@string(escape_sequence_tooltip, "Replace \\, \\t, \\n, \\r and \\uXXXX (Unicode characters) with the corresponding control")' />
			</vbox>
			<vbox lw="wc" lh="wc" layout-gravity="center">
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
						<CheckBox id="regex" text='@string(regular_expression, "Regular Expression")' selected="false" margin-right="8dp" />
						<CheckBox id="lua_pattern" text='@string(lua_pattern, "Lua Pattern")' selected="false" margin-right="8dp" />
						<CheckBox id="escape_sequence" text='@string(use_escape_sequences, "Use escape sequences")' margin-right="8dp" selected="false"
								  tooltip='@string(escape_sequence_tooltip, "Replace \\, \t, \n, \r and \uXXXX (Unicode characters) with the corresponding control")' />
						<CheckBox id="buffer_only_mode" text="@string(buffer_only_mode, Buffer Only Mode)" selected="false" tooltip="@string(buffer_only_mode_tooltip, Apply replacements to file buffers only.&#10;Changes won't be saved to disk until you explicitly save the files.)" visible="false" />
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
			<PushButton class="status_but" id="status_terminal_panel" text="@string(terminal, Terminal)" icon="icon(terminal, 11dp)" />
			<PushButton class="status_but" id="status_build_output" text="@string(build, Build)" icon="icon(symbol-property, 11dp)"  />
			<PushButton class="status_but" id="status_app_output" text="@string(app_output, App Output)" icon="icon(output, 11dp)"  />
			<Widget class="status_sep" lw="0" lw8="1" lh="1dp" />
		</statusbar>
	</vbox>
</Splitter>
<Image id="settings" lw="16dp" lh="16dp" lg="top|right" gravity="center" />
<TextView id="menu_hint" class="app_hint" lw="wc" lh="wc" lg="bottom|right" visible="false"
		  text='@string(menu_hold_shift_hint, "Hold \"Shift\" to keep menu open)"'
		  tooltip='@string(menu_hold_shift_hint_desc, "Keeping \"Shift\" clicked while changing any options it will keep the menu open.")' />
</MainLayout>
</vbox>
<vbox id="notification_center" lw="256dp" lh="wc"
	  lg="right|bottom" margin-right="22dp" margin-bottom="56dp">
</vbox>
)html"
