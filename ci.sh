#!/bin/sh
if [ "$TRAVIS_OS_NAME" = "osx" ]
then
	./cdcxbld/cibuild-mac.sh
else
	./cdcxbld/cibuild.sh
fi
