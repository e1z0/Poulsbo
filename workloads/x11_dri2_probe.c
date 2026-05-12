#include <xcb/dri2.h>
#include <xcb/xcb.h>

#include <drm/drm.h>
#include <xf86drm.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void print_reply_error(const char *what, xcb_generic_error_t *error)
{
	if (!error)
		return;

	printf("%s_error_code=%u\n", what, error->error_code);
	free(error);
}

static void print_cap(int fd, uint64_t cap, const char *name)
{
	uint64_t value = 0;
	int ret = drmGetCap(fd, cap, &value);

	if (ret == 0)
		printf("%s=%llu\n", name, (unsigned long long)value);
	else
		printf("%s=error:%d\n", name, ret);
}

int main(void)
{
	int screen_no = 0;
	xcb_connection_t *conn;
	const xcb_setup_t *setup;
	xcb_screen_iterator_t screen_iter;
	xcb_screen_t *screen;
	const xcb_query_extension_reply_t *ext;
	xcb_generic_error_t *error = NULL;
	xcb_dri2_query_version_reply_t *version_reply;
	xcb_dri2_connect_reply_t *connect_reply;
	xcb_dri2_authenticate_reply_t *auth_reply;
	int driver_len;
	int device_len;
	const char *driver_name;
	const char *device_name;
	int fd = -1;
	drm_magic_t magic = 0;
	drmVersionPtr version = NULL;
	struct stat st;

	printf("display_env=%s\n", getenv("DISPLAY") ? getenv("DISPLAY") : "(null)");
	printf("xauthority_env=%s\n",
	       getenv("XAUTHORITY") ? getenv("XAUTHORITY") : "(null)");

	conn = xcb_connect(NULL, &screen_no);
	if (xcb_connection_has_error(conn)) {
		printf("xcb_connect_error=%d\n", xcb_connection_has_error(conn));
		return 1;
	}

	setup = xcb_get_setup(conn);
	screen_iter = xcb_setup_roots_iterator(setup);
	for (int i = 0; i < screen_no && screen_iter.rem; i++)
		xcb_screen_next(&screen_iter);
	screen = screen_iter.data;
	if (!screen) {
		printf("xcb_screen_error=no-screen\n");
		xcb_disconnect(conn);
		return 1;
	}

	printf("xcb_screen_root=0x%08x\n", screen->root);
	printf("xcb_screen_size=%ux%u\n", screen->width_in_pixels,
	       screen->height_in_pixels);

	ext = xcb_get_extension_data(conn, &xcb_dri2_id);
	printf("dri2_extension_present=%u\n", ext ? ext->present : 0);
	if (!ext || !ext->present) {
		xcb_disconnect(conn);
		return 0;
	}

	version_reply = xcb_dri2_query_version_reply(
		conn,
		xcb_dri2_query_version(conn, XCB_DRI2_MAJOR_VERSION,
					 XCB_DRI2_MINOR_VERSION),
		&error);
	if (!version_reply) {
		print_reply_error("dri2_query_version", error);
		xcb_disconnect(conn);
		return 0;
	}
	printf("dri2_version=%u.%u\n", version_reply->major_version,
	       version_reply->minor_version);
	free(version_reply);
	error = NULL;

	connect_reply = xcb_dri2_connect_reply(
		conn,
		xcb_dri2_connect(conn, screen->root, XCB_DRI2_DRIVER_TYPE_DRI),
		&error);
	if (!connect_reply) {
		print_reply_error("dri2_connect", error);
		xcb_disconnect(conn);
		return 0;
	}

	driver_len = xcb_dri2_connect_driver_name_length(connect_reply);
	device_len = xcb_dri2_connect_device_name_length(connect_reply);
	driver_name = xcb_dri2_connect_driver_name(connect_reply);
	device_name = xcb_dri2_connect_device_name(connect_reply);

	printf("dri2_driver_name_len=%d\n", driver_len);
	printf("dri2_driver_name=%.*s\n", driver_len, driver_name);
	printf("dri2_device_name_len=%d\n", device_len);
	printf("dri2_device_name=%.*s\n", device_len, device_name);

	if (device_len > 0) {
		char *path = calloc((size_t)device_len + 1, 1);

		if (!path) {
			free(connect_reply);
			xcb_disconnect(conn);
			return 1;
		}

		memcpy(path, device_name, (size_t)device_len);
		fd = open(path, O_RDWR);
		printf("drm_open_path=%s\n", path);
		printf("drm_open_result=%d\n", fd);
		printf("drm_open_errno=%d\n", fd >= 0 ? 0 : errno);

		if (fd >= 0) {
			if (fstat(fd, &st) == 0)
				printf("drm_mode=%o drm_rdev=%llu\n",
				       st.st_mode & 0777,
				       (unsigned long long)st.st_rdev);

			version = drmGetVersion(fd);
			if (version) {
				printf("drm_kernel_name=%.*s\n", version->name_len,
				       version->name);
				printf("drm_kernel_desc=%.*s\n", version->desc_len,
				       version->desc);
				printf("drm_kernel_version=%d.%d.%d\n",
				       version->version_major,
				       version->version_minor,
				       version->version_patchlevel);
				drmFreeVersion(version);
			}

			if (drmGetMagic(fd, &magic) == 0)
				printf("drm_magic=%u\n", magic);
			else
				printf("drm_magic_error=%d\n", errno);

			print_cap(fd, DRM_CAP_DUMB_BUFFER, "drm_cap_dumb_buffer");
			print_cap(fd, DRM_CAP_PRIME, "drm_cap_prime");
			print_cap(fd, DRM_CAP_TIMESTAMP_MONOTONIC,
				  "drm_cap_timestamp_monotonic");
			print_cap(fd, DRM_CAP_SYNCOBJ, "drm_cap_syncobj");
		}

		if (magic != 0) {
			auth_reply = xcb_dri2_authenticate_reply(
				conn,
				xcb_dri2_authenticate(conn, screen->root, magic),
				&error);
			if (!auth_reply) {
				print_reply_error("dri2_authenticate", error);
			} else {
				printf("dri2_authenticated=%u\n",
				       auth_reply->authenticated);
				free(auth_reply);
			}
			error = NULL;
		}

		if (fd >= 0)
			close(fd);
		free(path);
	}

	free(connect_reply);
	xcb_disconnect(conn);
	return 0;
}
