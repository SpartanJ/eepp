R"xml(
<window class="file_properties" id="file_properties" window-flags="default|shadow|modal|resizeable"
 window-title='@string(eefm_properties, "Properties")'>
 <vbox class="properties_body">
  <RelativeLayout lw="mp" lh="0" layout_weight="1">
   <TabWidget id="properties_tabs" lw="mp" lh="mp">
    <ScrollView id="properties_general_scroll" lw="mp" lh="mp" hscroll-mode="off">
     <vbox id="properties_general" class="properties_page">
      <hbox class="properties_row" margin-bottom="14dp">
       <hbox class="properties_label">
        <Image id="properties_icon" lw="48dp" lh="48dp" margin-left="64dp" />
       </hbox>
       <TextInput id="properties_name" />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_type, "Type:")' />
       <TextView class="properties_value" id="properties_type" />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_location, "Location:")' />
       <TextView class="properties_value" id="properties_location" />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_target, "Points to:")' />
       <TextView class="properties_value" id="properties_target" />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_original, "Original location:")' />
       <TextView class="properties_value" id="properties_original" />
      </hbox>
      <widget class="properties_separator" />
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_size, "Size:")' />
       <TextView class="properties_value" id="properties_size" />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_disk, "Space on disk:")' />
       <TextView class="properties_value" id="properties_disk" />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_contents, "Contents:")' />
       <TextView class="properties_value" id="properties_contents" />
      </hbox>
      <hbox class="properties_row">
       <widget class="properties_label" />
       <PushButton id="properties_calculate" text='@string(eefm_properties_calculate, "Calculate")' />
      </hbox>
      <hbox class="properties_row" visible="false">
       <widget class="properties_label" />
       <TextView class="properties_value" id="properties_totals" />
      </hbox>
      <!-- Selection cardinality determines this section before metadata or measurement arrives. -->
      <vbox id="properties_timestamps" lw="mp" lh="wc" visible="false">
       <widget class="properties_separator" />
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_created, "Created:")' />
       <TextView class="properties_value" id="properties_created" />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_modified, "Modified:")' />
       <TextView class="properties_value" id="properties_modified" />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_accessed, "Accessed:")' />
       <TextView class="properties_value" id="properties_accessed" />
      </hbox>
       <widget class="properties_separator" />
      </vbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_space, "Free space:")' />
       <TextView class="properties_value" id="properties_space" />
      </hbox>
     </vbox>
    </ScrollView>
    <ScrollView id="properties_permissions_scroll" lw="mp" lh="mp" hscroll-mode="off">
     <vbox id="properties_permissions" class="properties_page">
      <TextView class="properties_heading" text='@string(eefm_properties_access_permissions, "Access Permissions")' />
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_permissions_owner, "Owner:")' />
       <DropDownList id="properties_mode_0" selected-index="0">
        <item>@string(eefm_permissions_unchanged, "Leave unchanged")</item>
        <item>@string(eefm_permissions_no_access, "No access")</item>
        <item>@string(eefm_permissions_execute_only, "Execute only")</item>
        <item>@string(eefm_permissions_write_only, "Write only")</item>
        <item>@string(eefm_permissions_write_execute, "Write and execute")</item>
        <item>@string(eefm_permissions_read_only, "Read only")</item>
        <item>@string(eefm_permissions_read_execute, "Read and execute")</item>
        <item>@string(eefm_permissions_read_write, "Read and write")</item>
        <item>@string(eefm_permissions_read_write_execute, "Read, write and execute")</item>
       </DropDownList>
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_permissions_group, "Group:")' />
       <DropDownList id="properties_mode_1" selected-index="0">
        <item>@string(eefm_permissions_unchanged, "Leave unchanged")</item>
        <item>@string(eefm_permissions_no_access, "No access")</item>
        <item>@string(eefm_permissions_execute_only, "Execute only")</item>
        <item>@string(eefm_permissions_write_only, "Write only")</item>
        <item>@string(eefm_permissions_write_execute, "Write and execute")</item>
        <item>@string(eefm_permissions_read_only, "Read only")</item>
        <item>@string(eefm_permissions_read_execute, "Read and execute")</item>
        <item>@string(eefm_permissions_read_write, "Read and write")</item>
        <item>@string(eefm_permissions_read_write_execute, "Read, write and execute")</item>
       </DropDownList>
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_permissions_others, "Others:")' />
       <DropDownList id="properties_mode_2" selected-index="0">
        <item>@string(eefm_permissions_unchanged, "Leave unchanged")</item>
        <item>@string(eefm_permissions_no_access, "No access")</item>
        <item>@string(eefm_permissions_execute_only, "Execute only")</item>
        <item>@string(eefm_permissions_write_only, "Write only")</item>
        <item>@string(eefm_permissions_write_execute, "Write and execute")</item>
        <item>@string(eefm_permissions_read_only, "Read only")</item>
        <item>@string(eefm_permissions_read_execute, "Read and execute")</item>
        <item>@string(eefm_permissions_read_write, "Read and write")</item>
        <item>@string(eefm_permissions_read_write_execute, "Read, write and execute")</item>
       </DropDownList>
      </hbox>
      <hbox class="properties_row">
       <widget class="properties_label" />
       <PushButton id="properties_advanced_toggle" text='@string(eefm_properties_advanced, "Advanced Permissions...")' />
      </hbox>
      <vbox id="properties_advanced" lw="mp" lh="wc" visible="false">
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_permissions_setuid, "Set user ID:")' />
       <DropDownList id="properties_mode_3" selected-index="0">
        <item>@string(eefm_permissions_unchanged, "Leave unchanged")</item>
        <item>@string(eefm_permissions_off, "Off")</item>
        <item>@string(eefm_permissions_on, "On")</item>
       </DropDownList>
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_permissions_setgid, "Set group ID:")' />
       <DropDownList id="properties_mode_4" selected-index="0">
        <item>@string(eefm_permissions_unchanged, "Leave unchanged")</item>
        <item>@string(eefm_permissions_off, "Off")</item>
        <item>@string(eefm_permissions_on, "On")</item>
       </DropDownList>
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_permissions_sticky, "Sticky bit:")' />
       <DropDownList id="properties_mode_5" selected-index="0">
        <item>@string(eefm_permissions_unchanged, "Leave unchanged")</item>
        <item>@string(eefm_permissions_off, "Off")</item>
        <item>@string(eefm_permissions_on, "On")</item>
       </DropDownList>
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_octal, "Octal mode:")' />
       <TextInput id="properties_octal" hint='@string(eefm_permissions_unchanged, "Leave unchanged")'
        tooltip='@string(eefm_properties_octal_hint, "An octal mode (0000–7777) overrides the access choices above.")' />
      </hbox>
      </vbox>
      <widget class="properties_separator" />
      <TextView class="properties_heading" text='@string(eefm_properties_ownership, "Ownership")' />
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_user, "User:")' />
       <TextInput id="properties_user" hint='@string(eefm_permissions_unchanged, "Leave unchanged")' />
      </hbox>
      <hbox class="properties_row">
       <TextView class="properties_label" text='@string(eefm_properties_label_group, "Group:")' />
       <TextInput id="properties_group" hint='@string(eefm_permissions_unchanged, "Leave unchanged")' />
      </hbox>
      <widget class="properties_separator" />
      <CheckBox id="properties_recursive" lw="mp" lh="wc"
       text='@string(eefm_properties_recursive, "Apply changes to all subfolders and their contents")' />
      <TextView id="properties_permission_hint" class="properties_hint" />
     </vbox>
    </ScrollView>
    <Tab text='@string(eefm_properties_general, "General")' owns="properties_general_scroll" />
    <Tab text='@string(eefm_properties_permissions, "Permissions")' owns="properties_permissions_scroll" />
   </TabWidget>
   <RelativeLayout id="properties_loading_overlay" lw="mp" lh="mp">
    <Loader id="properties_loader" lw="40dp" lh="40dp" outline-thickness="3dp"
     indeterminate="true" layout_gravity="center" />
   </RelativeLayout>
  </RelativeLayout>
  <TextView id="properties_status" lw="mp" lh="wc" max-height="56dp" word-wrap="true" margin-top="8dp" visible="false" />
  <hbox class="properties_actions">
   <widget lw="0" lh="1dp" layout_weight="1" />
   <PushButton id="properties_ok" text='@string(eefm_ok, "OK")' />
   <PushButton id="properties_cancel" text='@string(eefm_cancel, "Cancel")' />
   <PushButton id="properties_apply" text='@string(eefm_apply, "Apply")' />
  </hbox>
 </vbox>
</window>
)xml"
