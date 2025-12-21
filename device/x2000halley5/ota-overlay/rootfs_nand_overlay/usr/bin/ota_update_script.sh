#!/bin/sh
update
if [ "$?" -eq 0 ]		# Update required
then
	echo "There is a new version available for update."
	read -p "Would you like to upgrade? [Y/n]" UPDATE_REQUEST_FLAG
	UPDATE_REQUEST_FLAG=$(echo "$UPDATE_REQUEST_FLAG" | tr '[:upper:]' '[:lower:]')
	if [[ $UPDATE_REQUEST_FLAG == 'y' ]]
	then
		cp /etc/wpa_supplicant.conf /usr/data
		recovery
	fi
fi

echo "OTA upgrade aborted."
