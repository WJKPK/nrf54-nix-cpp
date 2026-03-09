#pragma once

#include <cstddef>
#include <cstdint>

#include <etl/array.h>
#include <etl/expected.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/string.h>
#include <zephyr/fs/fs.h>

namespace fs
{

inline constexpr std::size_t kMaxPathLen = 255;

struct FsError {
	int code;
	constexpr explicit FsError(int rc) noexcept : code(rc)
	{
	}
	const char *what() const noexcept;
};

template <typename T> using Result = etl::expected<T, FsError>;
using Status = Result<etl::monostate>;
using Path = etl::string<kMaxPathLen>;

class Filesystem;

class File
{
      public:
	File(const File &) = delete;
	File &operator=(const File &) = delete;

	File(File &&other) noexcept;
	~File() noexcept;

	[[nodiscard]] Result<std::size_t> read(etl::span<uint8_t> buf) noexcept;
	[[nodiscard]] Result<std::size_t> write(etl::span<const uint8_t> buf) noexcept;

	enum class Whence : int {
		Set = FS_SEEK_SET,
		Current = FS_SEEK_CUR,
		End = FS_SEEK_END,
	};

	Status seek(off_t offset, Whence whence = Whence::Set) noexcept;
	[[nodiscard]] Result<off_t> tell() noexcept;
	Status truncate(off_t length) noexcept;
	Status sync() noexcept;
	Status close() noexcept;
	bool is_valid() const noexcept;

      private:
	friend class Filesystem;

	explicit File(fs_file_t f) noexcept;
	int close_internal() noexcept;

	fs_file_t file_{};
	bool valid_{false};
};

struct DirEntry {
	enum class Type {
		File,
		Dir
	};
	Type type{};
	etl::string<kMaxPathLen> name{};
	std::size_t size{};
};

class Dir
{
      public:
	Dir(const Dir &) = delete;
	Dir &operator=(const Dir &) = delete;

	Dir(Dir &&other) noexcept;
	~Dir() noexcept;

	[[nodiscard]] Result<etl::optional<DirEntry>> read_next() noexcept;
	Status close() noexcept;
	bool is_valid() const noexcept;

      private:
	friend class Filesystem;

	explicit Dir(fs_dir_t d) noexcept;
	int close_internal() noexcept;

	fs_dir_t dir_{};
	bool valid_{false};
};

struct StatVfs {
	unsigned long block_size;
	unsigned long fragment_size;
	unsigned long blocks;
	unsigned long blocks_free;
};

class Filesystem
{
      public:
	Filesystem(const Filesystem &) = delete;
	Filesystem &operator=(const Filesystem &) = delete;

	Filesystem(Filesystem &&other) noexcept;
	~Filesystem() noexcept;

	enum class OpenFlags : int {
		ReadOnly = FS_O_READ,
		WriteOnly = FS_O_WRITE,
		ReadWrite = FS_O_RDWR,
		Create = FS_O_CREATE,
		Append = FS_O_APPEND,
		CreateRW = FS_O_CREATE | FS_O_RDWR,
	};

	friend OpenFlags operator|(OpenFlags a, OpenFlags b)
	{
		return static_cast<OpenFlags>(static_cast<int>(a) | static_cast<int>(b));
	}

	[[nodiscard]] static Result<Filesystem>
	mount(fs_mount_t *mp, unsigned int flash_area_id = 0, bool wipe = false) noexcept;

	Status unmount() noexcept;
	[[nodiscard]] Result<StatVfs> statvfs() const noexcept;
	[[nodiscard]] Result<DirEntry> stat(etl::string_view rel_path) const noexcept;
	Status unlink(etl::string_view rel_path) noexcept;
	Status mkdir(etl::string_view rel_path) noexcept;
	Status rename(etl::string_view from, etl::string_view to) noexcept;
	[[nodiscard]] Result<File> open_file(etl::string_view rel_path,
					     OpenFlags flags = OpenFlags::CreateRW) noexcept;
	[[nodiscard]] Result<Dir> open_dir(etl::string_view rel_path = "") noexcept;

	bool is_mounted() const noexcept;
	const char *mount_point() const noexcept;

      private:
	explicit Filesystem(fs_mount_t *mp) noexcept;

	int unmount_internal() noexcept;
	Path make_abs_path(etl::string_view rel) const noexcept;
	static int erase_flash_area(unsigned int id) noexcept;

	fs_mount_t *mountpoint_{nullptr};
	bool mounted_{false};
};

} // namespace fs
