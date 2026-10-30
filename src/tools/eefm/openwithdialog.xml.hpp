R"xml(
<window id="open_with_dialog" window-flags="default|shadow|modal|resizeable"
 window-title='@string(eefm_open_with, "Open With")'>
 <vbox lw="mp" lh="mp" padding="12dp">
  <TextView id="open_with_type" lw="mp" lh="wc" word-wrap="true"
   text='@string(eefm_choose_application_for_files, "Choose an application for the selected files:")' />
  <ListView id="open_with_applications" class="application_list" lw="mp" lh="0" layout_weight="1"
   row-height="34dp" icon-size="24dp" selection-kind="single"
   vscroll-mode="auto" hscroll-mode="off" margin="8dp 0" />
  <CheckBox id="open_with_always" lw="mp" lh="wc" word-wrap="true" enabled="false"
   text='@string(eefm_always_use_application, "Always use this application for this file type")' />
  <PushButton id="open_with_default" lw="wc" lh="wc" enabled="false" visible="false"
   text='@string(eefm_change_default_application, "Change default app...")' />
  <TextView id="open_with_hint" lw="mp" lh="wc" word-wrap="true" margin-top="4dp"
   color="var(--font-hint)" />
  <TextView id="open_with_status" lw="mp" lh="wc" word-wrap="true" margin-top="8dp"
   text='@string(eefm_checking_file_type, "Checking file type...")' />
  <hbox lw="mp" lh="wc" margin-top="12dp">
   <PushButton id="open_with_command" lw="wc" lh="wc"
    text='@string(eefm_custom_command, "Custom Command...")' />
   <widget lw="0" lh="1dp" layout_weight="1" />
   <PushButton id="open_with_open" lw="wc" lh="wc"
    text='@string(eefm_open, "Open")' />
   <PushButton id="open_with_cancel" lw="wc" lh="wc" margin-left="8dp"
    text='@string(eefm_cancel, "Cancel")' />
  </hbox>
 </vbox>
</window>
)xml"
