.PHONY: build

build: 
	pebble build


install: build
	pebble install --phone $(PHONE_IP)


