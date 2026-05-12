#include <drm/drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static const char *connector_type_name(uint32_t type)
{
	switch (type) {
	case DRM_MODE_CONNECTOR_Unknown:
		return "Unknown";
	case DRM_MODE_CONNECTOR_VGA:
		return "VGA";
	case DRM_MODE_CONNECTOR_DVII:
		return "DVI-I";
	case DRM_MODE_CONNECTOR_DVID:
		return "DVI-D";
	case DRM_MODE_CONNECTOR_DVIA:
		return "DVI-A";
	case DRM_MODE_CONNECTOR_Composite:
		return "Composite";
	case DRM_MODE_CONNECTOR_SVIDEO:
		return "SVIDEO";
	case DRM_MODE_CONNECTOR_LVDS:
		return "LVDS";
	case DRM_MODE_CONNECTOR_Component:
		return "Component";
	case DRM_MODE_CONNECTOR_9PinDIN:
		return "DIN";
	case DRM_MODE_CONNECTOR_DisplayPort:
		return "DP";
	case DRM_MODE_CONNECTOR_HDMIA:
		return "HDMI-A";
	case DRM_MODE_CONNECTOR_HDMIB:
		return "HDMI-B";
	case DRM_MODE_CONNECTOR_TV:
		return "TV";
	case DRM_MODE_CONNECTOR_eDP:
		return "eDP";
	case DRM_MODE_CONNECTOR_VIRTUAL:
		return "Virtual";
	case DRM_MODE_CONNECTOR_DSI:
		return "DSI";
	default:
		return "Other";
	}
}

static const char *connection_name(drmModeConnection connection)
{
	switch (connection) {
	case DRM_MODE_CONNECTED:
		return "connected";
	case DRM_MODE_DISCONNECTED:
		return "disconnected";
	case DRM_MODE_UNKNOWNCONNECTION:
	default:
		return "unknown";
	}
}

static void print_cap(int fd, uint64_t cap, const char *name)
{
	uint64_t value = 0;
	int ret = drmGetCap(fd, cap, &value);

	if (ret == 0)
		printf("%s=%llu\n", name, (unsigned long long)value);
	else
		printf("%s=error:%d errno=%d\n", name, ret, errno);
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "/dev/dri/card0";
	unsigned int hold_seconds = 0;
	struct stat st;
	int fd;
	drmVersionPtr version;
	char *busid;
	drm_magic_t magic = 0;
	drmModeResPtr res;
	int i;
	struct drm_mode_create_dumb create;
	struct drm_mode_map_dumb map;
	struct drm_mode_destroy_dumb destroy;
	void *map_ptr = MAP_FAILED;

	if (argc > 2) {
		char *end = NULL;
		unsigned long parsed;

		errno = 0;
		parsed = strtoul(argv[2], &end, 0);
		if (errno || !end || *end != '\0' || parsed > UINT32_MAX) {
			fprintf(stderr, "invalid hold seconds: %s\n", argv[2]);
			return 2;
		}
		hold_seconds = (unsigned int)parsed;
	}

	printf("drm_path=%s\n", path);
	if (stat(path, &st) == 0)
		printf("drm_path_mode=%o drm_rdev=%llu\n",
		       st.st_mode & 0777, (unsigned long long)st.st_rdev);

	fd = open(path, O_RDWR);
	printf("drm_open_result=%d\n", fd);
	if (fd < 0) {
		printf("drm_open_errno=%d\n", errno);
		return 1;
	}

	version = drmGetVersion(fd);
	if (version) {
		printf("drm_kernel_name=%.*s\n", version->name_len, version->name);
		printf("drm_kernel_desc=%.*s\n", version->desc_len, version->desc);
		printf("drm_kernel_date=%.*s\n", version->date_len, version->date);
		printf("drm_kernel_version=%d.%d.%d\n",
		       version->version_major, version->version_minor,
		       version->version_patchlevel);
		drmFreeVersion(version);
	}

	busid = drmGetBusid(fd);
	printf("drm_busid=%s\n", busid ? busid : "(null)");

	print_cap(fd, DRM_CAP_DUMB_BUFFER, "drm_cap_dumb_buffer");
	print_cap(fd, DRM_CAP_VBLANK_HIGH_CRTC, "drm_cap_vblank_high_crtc");
	print_cap(fd, DRM_CAP_DUMB_PREFERRED_DEPTH,
		  "drm_cap_dumb_preferred_depth");
	print_cap(fd, DRM_CAP_DUMB_PREFER_SHADOW,
		  "drm_cap_dumb_prefer_shadow");
	print_cap(fd, DRM_CAP_PRIME, "drm_cap_prime");
	print_cap(fd, DRM_CAP_TIMESTAMP_MONOTONIC,
		  "drm_cap_timestamp_monotonic");
#ifdef DRM_CAP_ADDFB2_MODIFIERS
	print_cap(fd, DRM_CAP_ADDFB2_MODIFIERS,
		  "drm_cap_addfb2_modifiers");
#endif
#ifdef DRM_CAP_SYNCOBJ
	print_cap(fd, DRM_CAP_SYNCOBJ, "drm_cap_syncobj");
#endif

	if (drmGetMagic(fd, &magic) == 0)
		printf("drm_magic=%u\n", magic);
	else
		printf("drm_magic_error=%d\n", errno);

#ifdef DRM_CLIENT_CAP_UNIVERSAL_PLANES
	errno = 0;
	printf("drm_set_client_cap_universal_planes=%d errno=%d\n",
	       drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1), errno);
#endif
#ifdef DRM_CLIENT_CAP_ATOMIC
	errno = 0;
	printf("drm_set_client_cap_atomic=%d errno=%d\n",
	       drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1), errno);
#endif

	res = drmModeGetResources(fd);
	if (!res) {
		printf("drm_mode_get_resources=error errno=%d\n", errno);
		close(fd);
		return 0;
	}

	printf("drm_min_size=%dx%d\n", res->min_width, res->min_height);
	printf("drm_max_size=%dx%d\n", res->max_width, res->max_height);
	printf("drm_counts crtcs=%d encoders=%d connectors=%d fbs=%d\n",
	       res->count_crtcs, res->count_encoders, res->count_connectors,
	       res->count_fbs);

	for (i = 0; i < res->count_connectors; i++) {
		drmModeConnectorPtr conn = drmModeGetConnector(fd,
						     res->connectors[i]);

		if (!conn) {
			printf("connector[%d]_error=%d\n", i, errno);
			continue;
		}

		printf("connector[%d]_id=%u\n", i, conn->connector_id);
		printf("connector[%d]_type=%s\n", i,
		       connector_type_name(conn->connector_type));
		printf("connector[%d]_type_id=%u\n", i, conn->connector_type_id);
		printf("connector[%d]_connection=%s\n", i,
		       connection_name(conn->connection));
		printf("connector[%d]_mm=%ux%u\n", i, conn->mmWidth, conn->mmHeight);
		printf("connector[%d]_modes=%d\n", i, conn->count_modes);
		printf("connector[%d]_encoders=%d\n", i, conn->count_encoders);
		printf("connector[%d]_props=%d\n", i, conn->count_props);
		printf("connector[%d]_encoder_id=%u\n", i, conn->encoder_id);

		if (conn->count_modes > 0) {
			drmModeModeInfo *mode = &conn->modes[0];

			printf("connector[%d]_mode0=%s %dx%d@%d\n", i,
			       mode->name, mode->hdisplay, mode->vdisplay,
			       mode->vrefresh);
		}

		drmModeFreeConnector(conn);
	}

#ifdef DRM_CLIENT_CAP_UNIVERSAL_PLANES
	{
		drmModePlaneResPtr planes = drmModeGetPlaneResources(fd);

		if (planes) {
			printf("drm_plane_count=%u\n", planes->count_planes);
			drmModeFreePlaneResources(planes);
		} else {
			printf("drm_plane_count_error=%d\n", errno);
		}
	}
#endif

	memset(&create, 0, sizeof(create));
	create.width = 64;
	create.height = 64;
	create.bpp = 32;
	errno = 0;
	printf("drm_create_dumb=%d errno=%d\n",
	       drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create), errno);
	if (create.handle) {
		printf("drm_create_dumb_handle=%u\n", create.handle);
		printf("drm_create_dumb_pitch=%u\n", create.pitch);
		printf("drm_create_dumb_size=%llu\n",
		       (unsigned long long)create.size);

		memset(&map, 0, sizeof(map));
		map.handle = create.handle;
		errno = 0;
		printf("drm_map_dumb=%d errno=%d\n",
		       drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map), errno);
		if (map.offset) {
			printf("drm_map_dumb_offset=%llu\n",
			       (unsigned long long)map.offset);
			map_ptr = mmap(NULL, (size_t)create.size,
				       PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				       (off_t)map.offset);
			if (map_ptr == MAP_FAILED) {
				printf("drm_mmap_error=%d\n", errno);
			} else {
				memset(map_ptr, 0x5a, (size_t)create.size);
				printf("drm_mmap_write=ok\n");
				munmap(map_ptr, (size_t)create.size);
			}
		}

		{
			int prime_fd = -1;
			errno = 0;
			printf("drm_prime_handle_to_fd=%d errno=%d\n",
			       drmPrimeHandleToFD(fd, create.handle, 0, &prime_fd),
			       errno);
			if (prime_fd >= 0) {
				printf("drm_prime_fd=%d\n", prime_fd);
				close(prime_fd);
			}
		}

		if (hold_seconds) {
			printf("drm_hold_seconds=%u\n", hold_seconds);
			fflush(stdout);
			sleep(hold_seconds);
			printf("drm_hold_complete=1\n");
		}

		memset(&destroy, 0, sizeof(destroy));
		destroy.handle = create.handle;
		errno = 0;
		printf("drm_destroy_dumb=%d errno=%d\n",
		       drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy),
		       errno);
	}

	drmModeFreeResources(res);
	close(fd);
	return 0;
}
