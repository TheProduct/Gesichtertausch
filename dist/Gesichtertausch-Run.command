#! /bin/sh

TYPE=-a
SLEEP_DURATION=5
APP_NAME=/Users/eidotech/Documents/gesichtertausch/Gesichtertausch.app
OPTIONS=""

if [ "-a" = "$1" ]
then
	echo "### running application: "$APP_NAME
else
	echo "### running script: "$APP_NAME
fi

while true;
	do
	if [ "-a" = "$TYPE" ]
    then
       open $APP_NAME
    else
       $APP_NAME $4
	fi
	sleep $SLEEP_DURATION; 
done
