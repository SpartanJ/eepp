R"xml(
<style>
#quick_preview {
	background-color: var(--back);
	border-left-width: 1dp;
	border-left-color: var(--button-border);
}
.preview_header { padding: 4dp 6dp; }
.preview_title { font-style: bold; }
#preview_name { padding: 4dp 8dp 8dp 8dp; text-selection: true; }
#preview_message {
	padding: 8dp;
	color: var(--font-hint);
	text-align: center;
}
#preview_editor {
	font-family: monospace;
	font-size: 11dp;
	background-color: var(--list-back);
	line-wrap-mode: word;
	line-wrap-type: viewport;
}
#preview_audio .audio_player { layout-width: match_parent; margin: 6dp; }
#preview_audio .time_progress { font-size: 10dp; margin-left: 2dp; margin-right: 2dp; }
#preview_audio .audio_btn { layout-width: 20dp; }
</style>
<vbox id="quick_preview" layout_width="match_parent" layout_height="match_parent" min-width="160dp">
	<hbox class="preview_header" layout_width="match_parent" layout_height="wrap_content">
		<TextView class="preview_title" layout_width="0" layout_weight="1" layout_height="wrap_content"
			layout_gravity="center_vertical" text='@string(eefm_quick_preview, "Quick Preview")' />
		<PushButton id="preview_close" class="tool" icon="action-close"
			tooltip='@string(eefm_close_preview, "Close Preview")' />
	</hbox>
	<TextView id="preview_name" layout_width="match_parent" layout_height="wrap_content" wordwrap="true" />
	<RelativeLayout id="preview_content" layout_width="match_parent" layout_height="0" layout_weight="1" clip="content-box">
		<TextView id="preview_message" layout_width="match_parent" layout_height="wrap_content"
			layout_gravity="center" wordwrap="true" />
		<Loader id="preview_loader" layout_width="32dp" layout_height="32dp"
			layout_gravity="center" indeterminate="true" visible="false" enabled="false" />
	</RelativeLayout>
</vbox>
)xml"
