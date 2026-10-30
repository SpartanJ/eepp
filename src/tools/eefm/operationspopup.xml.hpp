R"xml(
<window id="operations_popup" window-flags="undecorated|ephemeral|shareopacity">
 <vbox lw="mp" lh="mp" padding="12dp">
  <hbox lw="mp" lh="wc" margin-bottom="8dp">
   <TextView lw="0" layout_weight="1" lh="wc" layout_gravity="center_vertical"
    font-style="bold" text='@string(eefm_operations, "Operations")' />
   <PushButton id="operations_close" class="tool" lw="wc" lh="wc" icon="action-close"
    tooltip='@string(eefm_close, "Close")' />
  </hbox>
  <ScrollView lw="mp" lh="0" layout_weight="1" hscroll-mode="off" vscroll-mode="auto">
   <vbox id="operations_rows" lw="mp" lh="wc" />
  </ScrollView>
 </vbox>
</window>
)xml"
