/*
 * Sigma Control API DUT (station/AP)
 * Copyright (c) 2010, Atheros Communications, Inc.
 * Copyright (c) 2011-2014, Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Licensed under the Clear BSD license. See README for more details.
 */

#include "sigma_dut.h"
#ifdef __linux__
#include <sys/stat.h>
#endif /* __linux__ */
#include "wpa_helpers.h"


static int cmd_ca_get_version(struct sigma_dut *dut, struct sigma_conn *conn,
			      struct sigma_cmd *cmd)
{
	const char *info;

	info = get_param(cmd, "TestInfo");
	if (info) {
		char buf[200];
		snprintf(buf, sizeof(buf), "NOTE CAPI:TestInfo:%s", info);
		wpa_command(get_main_ifname(), buf);
	}

	send_resp(dut, conn, SIGMA_COMPLETE, "version,1.0");
	return 0;
}


#ifdef __linux__

static void first_line(char *s)
{
	while (*s) {
		if (*s == '\r' || *s == '\n') {
			*s = '\0';
			return;
		}
		s++;
	}
}


static void get_ver(const char *cmd, char *buf, size_t buflen)
{
	FILE *f;
	char *pos;

	buf[0] = '\0';
	f = popen(cmd, "r");
	if (f == NULL)
		return;
	if (fgets(buf, buflen, f))
		first_line(buf);
	pclose(f);

	pos = strstr(buf, " v");
	if (pos == NULL)
		buf[0] = '\0';
	else
		memmove(buf, pos + 1, strlen(pos));
}

#endif /* __linux__ */


static int cmd_device_get_info(struct sigma_dut *dut, struct sigma_conn *conn,
			       struct sigma_cmd *cmd)
{
	const char *vendor = "Qualcomm Atheros";
	const char *model = "N/A";
	const char *version = "N/A";
#ifdef __linux__
	char model_buf[128];
	char ver_buf[128];
#endif /* __linux__ */
	char resp[200];

#ifdef __linux__
	{
		char path[128];
		struct stat s;
		FILE *f;
		char compat_ver[128];
		char wpa_supplicant_ver[128];
		char hostapd_ver[128];

		snprintf(path, sizeof(path), "/sys/class/net/%s/phy80211",
			 get_main_ifname());
		if (stat(path, &s) == 0) {
			ssize_t res;
			char *pos;
			snprintf(path, sizeof(path),
				 "/sys/class/net/%s/device/driver",
				 get_main_ifname());
			res = readlink(path, path, sizeof(path));
			if (res < 0)
				model = "Linux/";
			else {
				if (res >= (int) sizeof(path))
					res = sizeof(path) - 1;
				path[res] = '\0';
				pos = strrchr(path, '/');
				if (pos == NULL)
					pos = path;
				else
					pos++;
				snprintf(model_buf, sizeof(model_buf),
					 "Linux/%s", pos);
				model = model_buf;
			}
		} else
			model = "Linux";

		/* TODO: get version from wpa_supplicant (+ driver via wpa_s)
		 */

		f = fopen("/sys/module/compat/parameters/"
			  "backported_kernel_version", "r");
		if (f == NULL)
			f = fopen("/sys/module/compat/parameters/"
				  "compat_version", "r");
		if (f) {
			if (fgets(compat_ver, sizeof(compat_ver), f) == NULL)
				compat_ver[0] = '\0';
			else
				first_line(compat_ver);
			fclose(f);
		} else
			compat_ver[0] = '\0';

		get_ver("./hostapd -v 2>&1", hostapd_ver, sizeof(hostapd_ver));
		if (hostapd_ver[0] == '\0')
			get_ver("hostapd -v 2>&1", hostapd_ver,
				sizeof(hostapd_ver));
		get_ver("./wpa_supplicant -v", wpa_supplicant_ver,
			sizeof(wpa_supplicant_ver));
		if (wpa_supplicant_ver[0] == '\0')
			get_ver("wpa_supplicant -v", wpa_supplicant_ver,
				sizeof(wpa_supplicant_ver));

		snprintf(ver_buf, sizeof(ver_buf),
			 "drv=%s%s%s%s%s/sigma=" SIGMA_DUT_VER "%s%s",
			 compat_ver,
			 wpa_supplicant_ver[0] ? "/wpas=" : "",
			 wpa_supplicant_ver,
			 hostapd_ver[0] ? "/hapd=" : "",
			 hostapd_ver,
			 dut->version ? "@" : "",
			 dut->version ? dut->version : "");
		version = ver_buf;
	}
#endif /* __linux__ */

	if (dut->vendor_name)
		vendor = dut->vendor_name;
	if (dut->model_name)
		model = dut->model_name;
	if (dut->version_name)
		version = dut->version_name;
	snprintf(resp, sizeof(resp), "vendor,%s,model,%s,version,%s",
		 vendor, model, version);

	send_resp(dut, conn, SIGMA_COMPLETE, resp);
	return 0;
}


static int check_device_list_interfaces(struct sigma_cmd *cmd)
{
	if (get_param(cmd, "interfaceType") == NULL)
		return -1;
	return 0;
}


static int cmd_device_list_interfaces(struct sigma_dut *dut,
				      struct sigma_conn *conn,
				      struct sigma_cmd *cmd)
{
	const char *type;
	char resp[200];

	type = get_param(cmd, "interfaceType");
	if (type == NULL)
		return -1;
	sigma_dut_print(dut, DUT_MSG_DEBUG, "device_list_interfaces - "
			"interfaceType=%s", type);
	if (strcmp(type, "802.11") != 0)
		return -2;

	snprintf(resp, sizeof(resp), "interfaceType,802.11,"
		 "interfaceID,%s", get_main_ifname());
	send_resp(dut, conn, SIGMA_COMPLETE, resp);

	return 0;
}


void basic_register_cmds(void)
{
	sigma_dut_reg_cmd("ca_get_version", NULL, cmd_ca_get_version);
	sigma_dut_reg_cmd("device_get_info", NULL, cmd_device_get_info);
	sigma_dut_reg_cmd("device_list_interfaces",
			  check_device_list_interfaces,
			  cmd_device_list_interfaces);
}
