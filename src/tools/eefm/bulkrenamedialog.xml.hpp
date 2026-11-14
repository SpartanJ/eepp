R"xml(
<window id="bulk_rename_dialog" window-flags="default|shadow|modal"
 window-title='@string(eefm_rename_items, "Rename Items")'>
 <vbox lw="mp" lh="mp" padding="12dp">
  <ScrollView lw="mp" lh="0" layout_weight="1" hscroll-mode="off">
   <vbox id="bulk_rename_form" lw="mp" lh="wc" enabled="false">
    <hbox lw="mp" lh="wc" margin-bottom="10dp">
     <TextView lw="wc" lh="wc" layout_gravity="center_vertical" margin-right="8dp"
      text='@string(eefm_rename_how, "How to rename:")' />
     <DropDownList id="bulk_rename_mode" lw="0" layout_weight="1" lh="wc" popup-to-root="true">
      <item>@string(eefm_rename_enumerate, "Enumerate")</item>
      <item>@string(eefm_rename_replace_text, "Replace text")</item>
      <item>@string(eefm_rename_add_text, "Add text")</item>
     </DropDownList>
    </hbox>
    <TextView id="bulk_rename_instruction" lw="mp" lh="wc" word-wrap="true" margin-bottom="8dp" />
    <vbox id="bulk_rename_enumerate" lw="mp" lh="wc">
     <hbox lw="mp" lh="wc" margin-bottom="8dp">
      <TextInput id="bulk_rename_template" lw="0" layout_weight="1" lh="wc" />
      <DropDownList id="bulk_rename_position" lw="wc" lh="wc" min-width="14em" margin-left="8dp" popup-to-root="true" selected-index="0">
       <item>@string(eefm_rename_replace_filename, "Replace filename")</item>
       <item>@string(eefm_rename_before_filename, "Before filename")</item>
       <item>@string(eefm_rename_after_filename, "After filename")</item>
      </DropDownList>
     </hbox>
     <hbox lw="mp" lh="wc">
      <TextView lw="0" layout_weight="1" lh="wc" word-wrap="true" layout_gravity="center_vertical" margin-right="8dp"
       text='@string(eefm_rename_starting_with, "# will be replaced by ascending numbers starting with:")' />
      <SpinBox id="bulk_rename_start" lw="100dp" lh="wc" min-value="0" max-value="1000000000"
       value="1" allow-floating-point="false" />
     </hbox>
    </vbox>
    <vbox id="bulk_rename_replace" lw="mp" lh="wc" visible="false">
     <TextView lw="mp" lh="wc" word-wrap="true"
      text='@string(eefm_rename_replacing, "Text to replace (literal, case-sensitive):")' />
     <TextInput id="bulk_rename_search" lw="mp" lh="wc" margin-bottom="6dp"
      hint='@string(eefm_rename_find_text, "Text to find")'
      tooltip='@string(eefm_rename_literal_hint, "Matches exact text, including case. Regular expression syntax is treated as ordinary text.")' />
     <TextView lw="mp" lh="wc" text='@string(eefm_rename_with, "With:")' />
     <TextInput id="bulk_rename_replacement" lw="mp" lh="wc"
      hint='@string(eefm_rename_replacement, "Replacement")' />
    </vbox>
    <vbox id="bulk_rename_add" lw="mp" lh="wc" visible="false">
     <TextInput id="bulk_rename_text" lw="mp" lh="wc" margin-bottom="6dp"
      hint='@string(eefm_rename_text_to_add, "Text to add")' />
     <DropDownList id="bulk_rename_add_position" lw="mp" lh="wc" popup-to-root="true" selected-index="0">
      <item>@string(eefm_rename_before_filename, "Before filename")</item>
      <item>@string(eefm_rename_after_filename, "After filename")</item>
     </DropDownList>
    </vbox>
    <TextView lw="mp" lh="wc" margin-top="12dp" margin-bottom="4dp"
     text='@string(eefm_rename_preview, "Preview:")' />
    <TextInput id="bulk_rename_preview" lw="mp" lh="wc" allow-editing="false" />
    <TextView id="bulk_rename_status" lw="mp" lh="wc" word-wrap="true" margin-top="8dp"
     text='@string(eefm_rename_loading, "Preparing filenames...")' />
   </vbox>
  </ScrollView>
  <hbox lw="mp" lh="wc" margin-top="12dp">
   <widget lw="0" lh="1dp" layout_weight="1" />
   <PushButton id="bulk_rename_accept" lw="wc" lh="wc" enabled="false"
    text='@string(eefm_rename_action, "Rename")' />
   <PushButton id="bulk_rename_cancel" lw="wc" lh="wc" margin-left="8dp"
    text='@string(eefm_cancel, "Cancel")' />
  </hbox>
 </vbox>
</window>
)xml"
