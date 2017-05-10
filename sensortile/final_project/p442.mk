# P442 Local targets

OPENOCD_CPU = stm32l4x
OPENOCD_SCRIPTS = ./scripts/

download: $(BUILDDIR)/$(PROJECT).bin                                            
	st-flash write $(BUILDDIR)/$(PROJECT).bin 0x8000000 > st-flash.log 2>&1    

etags:                                                                          
	find $(SRCPATHS) -type f -iname "*.[ch]" | xargs etags --append         
	find $(INCDIR) -type f -iname "*.[ch]" | xargs etags --append           
	find $(DINCDIR) -type f -iname "*.[ch]" | xargs etags --append          
	find $(UINCDIR) -type f -iname "*.[ch]" | xargs etags --append 

openocd:
	test -d build || mkdir -p build && cp gdbinit ./build/.gdbinit
	openocd -s ${OPENOCD_SCRIPTS} -f board/sensortile.cfg -c "${OPENOCD_CPU}.cpu configure -rtos ChibiOS"



