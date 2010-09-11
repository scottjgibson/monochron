make clean
make
copy MultiChron.hex distribution
copy MultiChron.eep distribution
make --makefile=MakeFileDC clean
make --makefile=MakeFileDC
copy MultiChronB.hex distribution
copy MultiChronB.eep distribution