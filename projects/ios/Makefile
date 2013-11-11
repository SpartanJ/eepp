IPHONE_IP:=
PROJECTNAME:=eepp
APPFOLDER:=$(PROJECTNAME).app
INSTALLFOLDER:=$(PROJECTNAME).app

install: 
ifeq ($(IPHONE_IP),)
	echo "Please set IPHONE_IP"
else
	ssh root@$(IPHONE_IP) 'rm -fr /Applications/$(INSTALLFOLDER)'
	scp -r $(APPFOLDER) root@$(IPHONE_IP):/Applications/$(INSTALLFOLDER)
	echo "Application $(INSTALLFOLDER) installed"
	ssh mobile@$(IPHONE_IP) 'uicache'
endif

uninstall:
ifeq ($(IPHONE_IP),)
	echo "Please set IPHONE_IP"
else
	ssh root@$(IPHONE_IP) 'rm -fr /Applications/$(INSTALLFOLDER)'
	echo "Application $(INSTALLFOLDER) uninstalled"
endif

.PHONY: install uninstall
