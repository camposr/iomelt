#include "iomelt.h"

#ifdef __APPLE__
#include <sys/mount.h>
#else
#include <sys/sysmacros.h>
#endif

static int is_network_fs(const char *fstype)
{
	const char *net_types[] = {
		"nfs", "nfs4", "cifs", "smb", "smb2", "smb3",
		"fuse.sshfs", "fuse.glusterfs", "ceph",
		"afpfs", "davfs", "ncpfs", NULL
	};
	int i;
	for (i = 0; net_types[i]; i++)
		if (strcasecmp(fstype, net_types[i]) == 0) return 1;
	return 0;
}

#ifndef __APPLE__

/* strip partition suffix: sda1->sda, nvme0n1p1->nvme0n1, mmcblk0p1->mmcblk0 */
static void base_device(const char *devpath, char *base, size_t len)
{
	const char *name = strrchr(devpath, '/');
	name = name ? name + 1 : devpath;
	snprintf(base, len, "%s", name);

	if (strncmp(base, "nvme", 4) == 0 || strncmp(base, "mmcblk", 6) == 0)
	{
		/* strip trailing p[digits] */
		char *p = strrchr(base, 'p');
		if (p && p > base)
		{
			char *end;
			strtol(p + 1, &end, 10);
			if (*end == '\0') *p = '\0';
		}
	}
	else
	{
		/* strip trailing digits: sda1->sda, hdb2->hdb */
		size_t l = strlen(base);
		while (l > 0 && isdigit((unsigned char)base[l - 1])) l--;
		base[l] = '\0';
	}
}

/* read a single-line sysfs attribute into buf, strip trailing whitespace */
static int read_sysfs(const char *basedev, const char *attr,
                      char *buf, size_t len)
{
	char path[512];
	FILE *f;
	size_t i;

	snprintf(path, sizeof(path), "/sys/block/%s/%s", basedev, attr);
	if (!(f = fopen(path, "r"))) return -1;
	if (!fgets(buf, len, f)) { fclose(f); return -1; }
	fclose(f);

	buf[strcspn(buf, "\n")] = '\0';
	i = strlen(buf);
	while (i > 0 && isspace((unsigned char)buf[i - 1])) buf[--i] = '\0';
	return 0;
}

int getDriveInfo(const char *path, driveInfo *info)
{
	struct stat st;
	FILE *f;
	char line[1024];
	unsigned int maj, min;
	int found = 0;
	char base[256];

	memset(info, 0, sizeof(*info));
	snprintf(info->type, sizeof(info->type), "Unknown");

	if (stat(path, &st) != 0) return -1;

	/* find matching entry in /proc/self/mountinfo by device number */
	if (!(f = fopen("/proc/self/mountinfo", "r"))) return -1;

	while (fgets(line, sizeof(line), f) && !found)
	{
		char *dash;
		unsigned int lmaj, lmin;

		if (sscanf(line, "%*d %*d %u:%u", &lmaj, &lmin) != 2)
			continue;
		if (makedev(lmaj, lmin) != st.st_dev)
			continue;

		/* optional fields end at " - "; fstype and source follow */
		dash = strstr(line, " - ");
		if (!dash) continue;

		if (sscanf(dash + 3, "%63s %255s",
		           info->fstype, info->device) == 2)
			found = 1;
	}
	fclose(f);
	if (!found) return -1;

	/* handle network filesystems */
	if (is_network_fs(info->fstype))
	{
		snprintf(info->type,  sizeof(info->type),  "%s", info->fstype);
		snprintf(info->model, sizeof(info->model), "Remote filesystem");
		info->is_remote = 1;
		return 0;
	}

	/* local device: derive base device name and query sysfs */
	base_device(info->device, base, sizeof(base));

	read_sysfs(base, "device/model",  info->model,  sizeof(info->model));
	read_sysfs(base, "device/vendor", info->vendor, sizeof(info->vendor));

	if (strncmp(base, "nvme", 4) == 0)
	{
		snprintf(info->type, sizeof(info->type), "NVMe");
	}
	else
	{
		char rot[8];
		if (read_sysfs(base, "queue/rotational", rot, sizeof(rot)) == 0)
			snprintf(info->type, sizeof(info->type),
			         atoi(rot) ? "HDD" : "SSD");
	}

	return 0;
}

#else /* __APPLE__ */

int getDriveInfo(const char *path, driveInfo *info)
{
	struct statfs sfs;
	char cmd[512], line[512];
	FILE *p;

	memset(info, 0, sizeof(*info));
	snprintf(info->type, sizeof(info->type), "Unknown");

	if (statfs(path, &sfs) != 0) return -1;

	snprintf(info->device, sizeof(info->device), "%s", sfs.f_mntfromname);
	snprintf(info->fstype, sizeof(info->fstype), "%s", sfs.f_fstypename);

	if (is_network_fs(sfs.f_fstypename))
	{
		snprintf(info->type,  sizeof(info->type),  "%s", sfs.f_fstypename);
		snprintf(info->model, sizeof(info->model), "Remote filesystem");
		info->is_remote = 1;
		return 0;
	}

	/* local device: query diskutil for model and type */
	snprintf(cmd, sizeof(cmd),
	         "diskutil info %s 2>/dev/null", sfs.f_mntfromname);
	if (!(p = popen(cmd, "r"))) return -1;

	while (fgets(line, sizeof(line), p))
	{
		char *val;

		if ((val = strstr(line, "Device / Media Name:")))
		{
			val = strchr(val, ':') + 1;
			while (*val == ' ') val++;
			val[strcspn(val, "\n")] = '\0';
			snprintf(info->model, sizeof(info->model), "%s", val);
		}
		else if ((val = strstr(line, "Protocol:")))
		{
			val = strchr(val, ':') + 1;
			while (*val == ' ') val++;
			val[strcspn(val, "\n")] = '\0';
			snprintf(info->type, sizeof(info->type), "%s", val);
		}
		else if ((val = strstr(line, "Solid State:")))
		{
			/* fallback if Protocol is not present */
			if (strstr(val, "Yes") && info->type[0] == '\0')
				snprintf(info->type, sizeof(info->type), "SSD");
		}
	}
	pclose(p);
	return 0;
}

#endif /* __APPLE__ */
