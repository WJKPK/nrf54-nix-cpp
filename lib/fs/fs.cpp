#include <app/lib/fs.hpp>

#include <cerrno>
#include <cstring>

#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(app_fs, CONFIG_LOG_DEFAULT_LEVEL);

namespace fs
{

const char *FsError::what() const noexcept
{
	switch (-code) {
	case ENOENT:
		return "ENOENT: no such file";
	case EEXIST:
		return "EEXIST: already exists";
	case EACCES:
		return "EACCES: permission denied";
	case ENOSPC:
		return "ENOSPC: no space left";
	case EBUSY:
		return "EBUSY: resource busy";
	case EIO:
		return "EIO: I/O error";
	case EBADF:
		return "EBADF: bad file descriptor";
	case EINVAL:
		return "EINVAL: invalid argument";
	default:
		return "unknown FS error";
	}
}

File::File(File &&other) noexcept : file_(other.file_), valid_(other.valid_)
{
	memset(&other.file_, 0, sizeof(other.file_));
	other.valid_ = false;
}

File::~File() noexcept
{
	close_internal();
}

Result<std::size_t> File::read(etl::span<uint8_t> buf) noexcept
{
	if (!valid_) {
		return etl::unexpected(FsError(-EBADF));
	}
	const ssize_t rc = fs_read(&file_, buf.data(), buf.size());
	if (rc < 0) {
		LOG_ERR("fs_read failed: %d", rc);
		return etl::unexpected(FsError(static_cast<int>(rc)));
	}
	return static_cast<std::size_t>(rc);
}

Result<std::size_t> File::write(etl::span<const uint8_t> buf) noexcept
{
	if (!valid_) {
		return etl::unexpected(FsError(-EBADF));
	}
	const ssize_t rc = fs_write(&file_, buf.data(), buf.size());
	if (rc < 0) {
		LOG_ERR("fs_write failed: %d", rc);
		return etl::unexpected(FsError(static_cast<int>(rc)));
	}
	return static_cast<std::size_t>(rc);
}

Status File::seek(off_t offset, Whence whence) noexcept
{
	if (!valid_) {
		return etl::unexpected(FsError(-EBADF));
	}
	const int rc = fs_seek(&file_, offset, static_cast<int>(whence));
	if (rc < 0) {
		LOG_ERR("fs_seek failed: %d", rc);
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

Result<off_t> File::tell() noexcept
{
	if (!valid_) {
		return etl::unexpected(FsError(-EBADF));
	}
	const off_t pos = fs_tell(&file_);
	if (pos < 0) {
		return etl::unexpected(FsError(static_cast<int>(pos)));
	}
	return pos;
}

Status File::truncate(off_t length) noexcept
{
	if (!valid_) {
		return etl::unexpected(FsError(-EBADF));
	}
	const int rc = fs_truncate(&file_, length);
	if (rc < 0) {
		LOG_ERR("fs_truncate failed: %d", rc);
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

Status File::sync() noexcept
{
	if (!valid_) {
		return etl::unexpected(FsError(-EBADF));
	}
	const int rc = fs_sync(&file_);
	if (rc < 0) {
		LOG_ERR("fs_sync failed: %d", rc);
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

Status File::close() noexcept
{
	if (!valid_) {
		return etl::monostate{};
	}
	const int rc = close_internal();
	if (rc < 0) {
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

bool File::is_valid() const noexcept
{
	return valid_;
}

File::File(fs_file_t f) noexcept : file_(f), valid_(true)
{
}

int File::close_internal() noexcept
{
	if (!valid_) {
		return 0;
	}
	valid_ = false;
	const int rc = fs_close(&file_);
	if (rc < 0) {
		LOG_ERR("fs_close failed: %d", rc);
	}
	return rc;
}

Dir::Dir(Dir &&other) noexcept : dir_(other.dir_), valid_(other.valid_)
{
	memset(&other.dir_, 0, sizeof(other.dir_));
	other.valid_ = false;
}

Dir::~Dir() noexcept
{
	close_internal();
}

Result<etl::optional<DirEntry>> Dir::read_next() noexcept
{
	if (!valid_) {
		return etl::unexpected(FsError(-EBADF));
	}
	struct fs_dirent entry{};
	const int rc = fs_readdir(&dir_, &entry);
	if (rc < 0) {
		LOG_ERR("fs_readdir failed: %d", rc);
		return etl::unexpected(FsError(rc));
	}
	if (entry.name[0] == '\0') {
		return etl::optional<DirEntry>(etl::nullopt);
	}
	DirEntry de;
	de.type = (entry.type == FS_DIR_ENTRY_DIR) ? DirEntry::Type::Dir : DirEntry::Type::File;
	de.name = entry.name;
	de.size = entry.size;
	return etl::optional<DirEntry>(de);
}

Status Dir::close() noexcept
{
	if (!valid_) {
		return etl::monostate{};
	}
	const int rc = close_internal();
	if (rc < 0) {
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

bool Dir::is_valid() const noexcept
{
	return valid_;
}

Dir::Dir(fs_dir_t d) noexcept : dir_(d), valid_(true)
{
}

int Dir::close_internal() noexcept
{
	if (!valid_) {
		return 0;
	}
	valid_ = false;
	const int rc = fs_closedir(&dir_);
	if (rc < 0) {
		LOG_ERR("fs_closedir failed: %d", rc);
	}
	return rc;
}

Filesystem::Filesystem(Filesystem &&other) noexcept
	: mountpoint_(other.mountpoint_), mounted_(other.mounted_)
{
	other.mounted_ = false;
}

Filesystem::~Filesystem() noexcept
{
	unmount_internal();
}

Result<Filesystem> Filesystem::mount(fs_mount_t *mp, unsigned int flash_area_id, bool wipe) noexcept
{
	if (!mp) {
		return etl::unexpected(FsError(-EINVAL));
	}
	if (wipe) {
		const int rc = erase_flash_area(flash_area_id);
		if (rc < 0) {
			return etl::unexpected(FsError(rc));
		}
	}
	const int rc = fs_mount(mp);
	if (rc == -EBUSY) {
		LOG_INF("%s already mounted", mp->mnt_point);
		return Filesystem(mp);
	}
	if (rc < 0) {
		LOG_ERR("fs_mount failed at %s: %d", mp->mnt_point, rc);
		return etl::unexpected(FsError(rc));
	}
	LOG_INF("%s mounted", mp->mnt_point);
	return Filesystem(mp);
}

Status Filesystem::unmount() noexcept
{
	if (!mounted_) {
		return etl::monostate{};
	}
	const int rc = unmount_internal();
	if (rc < 0) {
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

Result<StatVfs> Filesystem::statvfs() const noexcept
{
	if (!mounted_) {
		return etl::unexpected(FsError(-EBADF));
	}
	struct fs_statvfs raw{};
	const int rc = fs_statvfs(mountpoint_->mnt_point, &raw);
	if (rc < 0) {
		LOG_ERR("fs_statvfs failed: %d", rc);
		return etl::unexpected(FsError(rc));
	}
	return StatVfs{raw.f_bsize, raw.f_frsize, raw.f_blocks, raw.f_bfree};
}

Result<DirEntry> Filesystem::stat(etl::string_view rel_path) const noexcept
{
	if (!mounted_) {
		return etl::unexpected(FsError(-EBADF));
	}
	Path abs = make_abs_path(rel_path);
	struct fs_dirent entry{};
	const int rc = fs_stat(abs.c_str(), &entry);
	if (rc < 0) {
		return etl::unexpected(FsError(rc));
	}
	DirEntry de;
	de.type = (entry.type == FS_DIR_ENTRY_DIR) ? DirEntry::Type::Dir : DirEntry::Type::File;
	de.name = entry.name;
	de.size = entry.size;
	return de;
}

Status Filesystem::unlink(etl::string_view rel_path) noexcept
{
	if (!mounted_) {
		return etl::unexpected(FsError(-EBADF));
	}
	Path abs = make_abs_path(rel_path);
	const int rc = fs_unlink(abs.c_str());
	if (rc < 0) {
		LOG_ERR("fs_unlink %s failed: %d", abs.c_str(), rc);
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

Status Filesystem::mkdir(etl::string_view rel_path) noexcept
{
	if (!mounted_) {
		return etl::unexpected(FsError(-EBADF));
	}
	Path abs = make_abs_path(rel_path);
	const int rc = fs_mkdir(abs.c_str());
	if (rc < 0) {
		LOG_ERR("fs_mkdir %s failed: %d", abs.c_str(), rc);
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

Status Filesystem::rename(etl::string_view from, etl::string_view to) noexcept
{
	if (!mounted_) {
		return etl::unexpected(FsError(-EBADF));
	}
	Path abs_from = make_abs_path(from);
	Path abs_to = make_abs_path(to);
	const int rc = fs_rename(abs_from.c_str(), abs_to.c_str());
	if (rc < 0) {
		LOG_ERR("fs_rename failed: %d", rc);
		return etl::unexpected(FsError(rc));
	}
	return etl::monostate{};
}

Result<File> Filesystem::open_file(etl::string_view rel_path, OpenFlags flags) noexcept
{
	if (!mounted_) {
		return etl::unexpected(FsError(-EBADF));
	}
	Path abs = make_abs_path(rel_path);
	fs_file_t f{};
	fs_file_t_init(&f);
	const int rc = fs_open(&f, abs.c_str(), static_cast<int>(flags));
	if (rc < 0) {
		LOG_ERR("fs_open %s failed: %d", abs.c_str(), rc);
		return etl::unexpected(FsError(rc));
	}
	return File(f);
}

Result<Dir> Filesystem::open_dir(etl::string_view rel_path) noexcept
{
	if (!mounted_) {
		return etl::unexpected(FsError(-EBADF));
	}
	Path abs = rel_path.empty() ? Path(mountpoint_->mnt_point) : make_abs_path(rel_path);
	fs_dir_t d{};
	fs_dir_t_init(&d);
	const int rc = fs_opendir(&d, abs.c_str());
	if (rc < 0) {
		LOG_ERR("fs_opendir %s failed: %d", abs.c_str(), rc);
		return etl::unexpected(FsError(rc));
	}
	return Dir(d);
}

bool Filesystem::is_mounted() const noexcept
{
	return mounted_;
}

const char *Filesystem::mount_point() const noexcept
{
	return mounted_ ? mountpoint_->mnt_point : "";
}

Filesystem::Filesystem(fs_mount_t *mp) noexcept : mountpoint_(mp), mounted_(true)
{
}

int Filesystem::unmount_internal() noexcept
{
	if (!mounted_) {
		return 0;
	}
	mounted_ = false;
	const int rc = fs_unmount(mountpoint_);
	LOG_INF("%s unmount: %d", mountpoint_->mnt_point, rc);
	return rc;
}

Path Filesystem::make_abs_path(etl::string_view rel) const noexcept
{
	Path p(mountpoint_->mnt_point);
	if (!rel.empty()) {
		p += '/';
		p.append(rel.begin(), rel.end());
	}
	return p;
}

int Filesystem::erase_flash_area(unsigned int id) noexcept
{
	const struct flash_area *pfa = nullptr;
	int rc = flash_area_open(id, &pfa);
	if (rc < 0) {
		LOG_ERR("flash_area_open %u failed: %d", id, rc);
		return rc;
	}
	rc = flash_area_flatten(pfa, 0, pfa->fa_size);
	if (rc < 0) {
		LOG_ERR("flash_area_flatten failed: %d", rc);
	} else {
		LOG_INF("Flash area erased");
	}
	flash_area_close(pfa);
	return rc;
}

} // namespace fs
