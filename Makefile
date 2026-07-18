# make kern    generate ./kern (bach-370-chorales scores with modulations) from
#              annotations/bach-modulations.json -- never committed, always derived.
# make clean   remove the generated ./kern directory

.PHONY: kern clean

kern:
	$(MAKE) -C chorale-search
	./chorale-search/build/add-modulations \
		annotations/bach-modulations.json \
		bach-370-chorales/kern \
		kern/bach-370-chorales

clean:
	rm -rf kern
