#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "updater.h"

int main (int argc, char *argv[])
{
	int64_t new_version;
	int ret = -1;

	if (access("/usr/data/VERSION", F_OK)) {
		creat("/usr/data/VERSION", 0666);
		system("echo 0 > /usr/data/VERSION");
	}

	new_version = updater_check_update("/etc/ota_res/recovery.conf", NULL);
	printf("updater_check_update ret: %lld\n", new_version);

	if (new_version > 0) {
		printf("has update, the version is %lld, starting recovery \n", new_version);
		if (access("/usr/data/VERSION_NEW", F_OK)) {
			creat("/usr/data/VERSION_NEW", 0666);
			system("echo 0 > /usr/data/VERSION_NEW");
		}

		ret = updater_save_version("/usr/data/VERSION_NEW", new_version);
		if (ret)
			goto error;

		system("cp -f /usr/data/VERSION /usr/data/VERSION_OLD");
#ifdef MMC_DEVICE
		ret = system("getpackage");
		if (ret)
			goto error;
#endif

		return 0;			// Update required
	} else if (0 == new_version) {
		printf("There is no new version, no update required.\n");
		return 1;			// No update required
	}

error:
	return ret;
}
