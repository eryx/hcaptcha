
LINKCOLOR="\033[34;1m"
BINCOLOR="\033[37;1m"
ENDCOLOR="\033[0m"

ifndef V
	QUIET_BUILD = @printf '    %b %b\n' $(LINKCOLOR)BUILD$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR);
endif


all: hcaptcha-server 
	@echo ""
	@echo "hcaptcha build complete ..."
	@echo ""


hcaptcha-server:
	$(QUIET_BUILD)go build -ldflags "-w -s" -o ./hcaptcha-server ./hcaptcha-server.go$(CCLINK)


clean:
	rm -f ./hcaptcha-server
