#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <app/lib/fs.hpp>
#include <etl/span.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define PARTITION_NODE DT_NODELABEL(lfs1)

FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
static fs_mount_t *s_mp = &FS_FSTAB_ENTRY(PARTITION_NODE);

static void print_stats(fs::Filesystem &fs)
{
	auto sv = fs.statvfs();
	if (!sv) {
		LOG_ERR("statvfs: %s", sv.error().what());
		return;
	}
	LOG_PRINTK("  blocks: %lu total, %lu free (%lu byte blocks)\n", sv->blocks, sv->blocks_free,
		   sv->block_size);
}

int main()
{
	auto fs_res = fs::Filesystem::mount(s_mp);
	if (!fs_res) {
		LOG_ERR("mount: %s", fs_res.error().what());
		return 0;
	}
	auto &fs = *fs_res;

	LOG_PRINTK("=== stats before write ===\n");
	print_stats(fs);
	{
		auto f = fs.open_file("hello.txt", fs::Filesystem::OpenFlags::CreateRW);
		if (!f) {
			LOG_ERR("open: %s", f.error().what());
			return 0;
		}

		const char *msg = "Hello Zephyr!";
		auto wr = f->write(etl::span<const uint8_t>(reinterpret_cast<const uint8_t *>(msg),
							    strlen(msg)));
		if (!wr) {
			LOG_ERR("write: %s", wr.error().what());
			return 0;
		}
		LOG_PRINTK("written %zu bytes\n", *wr);
		f->sync();
	}
	LOG_PRINTK("=== stats after write ===\n");
	print_stats(fs);
	{
		auto f = fs.open_file("hello.txt", fs::Filesystem::OpenFlags::ReadOnly);
		if (!f) {
			LOG_ERR("open: %s", f.error().what());
			return 0;
		}

		uint8_t buf[64] = {};
		auto rd = f->read(etl::span<uint8_t>(buf, sizeof(buf)));
		if (!rd) {
			LOG_ERR("read: %s", rd.error().what());
			return 0;
		}

		buf[*rd] = '\0';
		LOG_PRINTK("read back (%zu bytes): %s\n", *rd, reinterpret_cast<const char *>(buf));
	}
	while (true) {
		k_sleep(K_MSEC(1000));
	}
	return 0;
}
