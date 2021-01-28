BUILD

docker build -t debuild .

RUN

docker run -it -v ${PWD}/packages:/build

** Once the run is successfull *.deb packages are in ${PWD}/packages

